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

#include <IACore/FileOps.hpp>
#include <IACore/StreamReader.hpp>

namespace IACore
{
  auto StreamReader::create_from_file(Ref<Path> path) -> Result<StreamReader>
  {
    Mut<usize> size = 0;

    const u8 *ptr = AU_TRY(FileOps::map_file(path, size));

    Mut<StreamReader> reader(Span<const u8>(ptr, size));
    reader.m_storage_type = StorageType::OwningMmap;

    return reader;
  }

  StreamReader::StreamReader(ForwardRef<Vec<u8>> data)
      : m_owning_vector(std::move(data)), m_storage_type(StorageType::OwningVector)
  {
    m_data = m_owning_vector.data();
    m_data_size = m_owning_vector.size();
  }

  StreamReader::StreamReader(const Span<const u8> data)
      : m_data(data.data()), m_data_size(data.size()), m_storage_type(StorageType::NonOwning)
  {
  }

  StreamReader::StreamReader(ForwardRef<StreamReader> other)
      : m_data(other.m_data), m_cursor(other.m_cursor), m_data_size(other.m_data_size),
        m_owning_vector(std::move(other.m_owning_vector)), m_storage_type(other.m_storage_type)
  {
    other.m_storage_type = StorageType::NonOwning;
    other.m_data = {};
    other.m_data_size = 0;

    if (m_storage_type == StorageType::OwningVector)
    {
      m_data = m_owning_vector.data();
    }
  }

  auto StreamReader::operator=(ForwardRef<StreamReader> other) -> MutRef<StreamReader>
  {
    if (this != &other)
    {
      if (m_storage_type == StorageType::OwningMmap)
      {
        FileOps::unmap_file(m_data);
      }

      m_data = other.m_data;
      m_cursor = other.m_cursor;
      m_data_size = other.m_data_size;
      m_owning_vector = std::move(other.m_owning_vector);
      m_storage_type = other.m_storage_type;

      if (m_storage_type == StorageType::OwningVector)
      {
        m_data = m_owning_vector.data();
      }

      other.m_storage_type = StorageType::NonOwning;
      other.m_data = {};
      other.m_data_size = 0;
    }
    return *this;
  }

  StreamReader::~StreamReader()
  {
    if (m_storage_type == StorageType::OwningMmap)
    {
      FileOps::unmap_file(m_data);
    }
  }
} // namespace IACore