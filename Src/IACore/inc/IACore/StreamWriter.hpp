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

#pragma once

#include <IACore/PCH.hpp>

namespace IACore
{

  class StreamWriter
  {
public:
    enum class StorageType
    {
      NonOwning,
      OwningFile,
      OwningVector,
    };

    static auto create_from_file(Ref<Path> path) -> Result<StreamWriter>;

    StreamWriter();
    explicit StreamWriter(const Span<u8> data);

    StreamWriter(ForwardRef<StreamWriter> other);
    auto operator=(ForwardRef<StreamWriter> other) -> MutRef<StreamWriter>;

    StreamWriter(Ref<StreamWriter>) = delete;
    auto operator=(Ref<StreamWriter>) -> MutRef<StreamWriter> = delete;

    ~StreamWriter();

    auto write(const u8 byte, const usize count) -> Result<void>;
    auto write(const void *buffer, const usize size) -> Result<void>;

    template<typename T> auto write(Ref<T> value) -> Result<void>;

    [[nodiscard]] auto data() const -> const u8 *
    {
      return m_buffer;
    }

    [[nodiscard]] auto cursor() const -> usize
    {
      return m_cursor;
    }

    auto flush() -> Result<void>;

private:
    Mut<u8 *> m_buffer = nullptr;
    Mut<usize> m_cursor = 0;
    Mut<usize> m_capacity = 0;
    Mut<Path> m_file_path;
    Mut<Vec<u8>> m_owning_vector;
    Mut<StorageType> m_storage_type = StorageType::OwningVector;

private:
    auto flush_to_disk() -> Result<void>;
  };

  template<typename T> inline auto StreamWriter::write(Ref<T> value) -> Result<void>
  {
    return write(&value, sizeof(T));
  }

} // namespace IACore