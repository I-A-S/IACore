// IACore-OSS; The Core Library for All IA Open Source Projects
// Copyright (C) 2026 IAS (ias@iasoft.dev)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <IACore/StreamWriter.hpp>

namespace IACore
{

  auto StreamWriter::create_from_file(Ref<Path> path) -> Result<StreamWriter>
  {
    Mut<FILE *> f = std::fopen(path.string().c_str(), "wb");
    if (!f)
    {
      return fail("Failed to open file for writing: {}", path.string());
    }
    std::fclose(f);

    Mut<StreamWriter> writer;
    writer.m_file_path = path;
    writer.m_storage_type = StorageType::OwningFile;

    return writer;
  }

  StreamWriter::StreamWriter() : m_storage_type(StorageType::OwningVector)
  {
    m_capacity = 256;
    m_owning_vector.resize(m_capacity);
    m_buffer = m_owning_vector.data();
  }

  StreamWriter::StreamWriter(const Span<u8> data)
      : m_buffer(data.data()), m_cursor(0), m_capacity(data.size()), m_storage_type(StorageType::NonOwning)
  {
  }

  StreamWriter::StreamWriter(ForwardRef<StreamWriter> other)
      : m_buffer(other.m_buffer), m_cursor(other.m_cursor), m_capacity(other.m_capacity),
        m_file_path(other.m_file_path), m_owning_vector(std::move(other.m_owning_vector)),
        m_storage_type(other.m_storage_type)
  {
    other.m_capacity = {};
    other.m_buffer = {};
    other.m_storage_type = StorageType::NonOwning;

    if (m_storage_type == StorageType::OwningVector)
      m_buffer = m_owning_vector.data();
  }

  auto StreamWriter::operator=(ForwardRef<StreamWriter> other) -> MutRef<StreamWriter>
  {
    if (this != &other)
    {
      if (m_storage_type == StorageType::OwningFile)
      {
        if (const Result<void> res = flush_to_disk(); !res)
        {
          std::fprintf(stderr, "[IACore] Data loss in StreamWriter move: %s\n", res.error().c_str());
        }
      }

      m_buffer = other.m_buffer;
      m_cursor = other.m_cursor;
      m_capacity = other.m_capacity;
      m_file_path = std::move(other.m_file_path);
      m_owning_vector = std::move(other.m_owning_vector);
      m_storage_type = other.m_storage_type;

      if (m_storage_type == StorageType::OwningVector)
        m_buffer = m_owning_vector.data();

      other.m_capacity = 0;
      other.m_cursor = 0;
      other.m_buffer = nullptr;
      other.m_storage_type = StorageType::NonOwning;
    }
    return *this;
  }

  StreamWriter::~StreamWriter()
  {
    if (m_storage_type == StorageType::OwningFile)
    {
      if (const Result<void> res = flush_to_disk(); !res)
      {
        std::fprintf(stderr, "[IACore] LOST DATA in ~StreamWriter: %s\n", res.error().c_str());
      }
    }
  }

  auto StreamWriter::flush() -> Result<void>
  {
    Mut<Result<void>> res = flush_to_disk();
    if (res.has_value())
    {
      m_storage_type = StorageType::OwningVector;
    }
    return res;
  }

  auto StreamWriter::flush_to_disk() -> Result<void>
  {
    if (m_storage_type != StorageType::OwningFile || m_file_path.empty())
    {
      return {};
    }

    Mut<FILE *> f = std::fopen(m_file_path.string().c_str(), "wb");
    if (!f)
    {
      return fail("Failed to open file for writing: {}", m_file_path.string());
    }

    const usize written = std::fwrite(m_buffer, 1, m_cursor, f);
    std::fclose(f);

    if (written != m_cursor)
    {
      return fail("Incomplete write: {} of {} bytes written", written, m_cursor);
    }
    return {};
  }

  auto StreamWriter::write(const u8 byte, const usize count) -> Result<void>
  {
    if (m_cursor + count > m_capacity)
    {
      if (m_storage_type == StorageType::NonOwning)
      {
        return fail("StreamWriter buffer overflow (NonOwning)");
      }

      const usize required = m_cursor + count;
      const usize double_cap = m_capacity * 2;
      const usize new_capacity = (double_cap > required) ? double_cap : required;

      m_owning_vector.resize(new_capacity);
      m_capacity = m_owning_vector.size();
      m_buffer = m_owning_vector.data();
    }

    std::memset(m_buffer + m_cursor, byte, count);
    m_cursor += count;
    return {};
  }

  auto StreamWriter::write(const void *buffer, const usize size) -> Result<void>
  {
    if (m_cursor + size > m_capacity)
    {
      if (m_storage_type == StorageType::NonOwning)
      {
        return fail("StreamWriter buffer overflow (NonOwning)");
      }

      const usize required = m_cursor + size;
      const usize double_cap = m_capacity * 2;
      const usize new_capacity = (double_cap > required) ? double_cap : required;

      m_owning_vector.resize(new_capacity);
      m_capacity = m_owning_vector.size();
      m_buffer = m_owning_vector.data();
    }

    std::memcpy(m_buffer + m_cursor, buffer, size);
    m_cursor += size;
    return {};
  }

} // namespace IACore