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
#include <algorithm>
#include <cstring>

namespace IACore
{
  class StreamReader
  {
public:
    enum class StorageType
    {
      NonOwning,
      OwningMmap,
      OwningVector,
    };

    static auto create_from_file(Ref<Path> path) -> Result<StreamReader>;

    explicit StreamReader(ForwardRef<Vec<u8>> data);
    explicit StreamReader(const Span<const u8> data);
    ~StreamReader();

    StreamReader(ForwardRef<StreamReader> other);
    auto operator=(ForwardRef<StreamReader> other) -> MutRef<StreamReader>;

    StreamReader(Ref<StreamReader>) = delete;
    auto operator=(Ref<StreamReader>) -> MutRef<StreamReader> = delete;

    auto read(Mut<void *> buffer, const usize size) -> Result<void>;

    template<typename T>
    [[nodiscard("Check for EOF")]]
    auto read() -> Result<T>;

    auto skip(const usize amount) -> void
    {
      m_cursor = std::min(m_cursor + amount, m_data_size);
    }

    auto seek(const usize pos) -> void
    {
      m_cursor = (pos > m_data_size) ? m_data_size : pos;
    }

    [[nodiscard]] auto cursor() const -> usize
    {
      return m_cursor;
    }

    [[nodiscard]] auto size() const -> usize
    {
      return m_data_size;
    }

    [[nodiscard]] auto remaining() const -> usize
    {
      return m_data_size - m_cursor;
    }

    [[nodiscard]] auto is_eof() const -> bool
    {
      return m_cursor >= m_data_size;
    }

private:
    Mut<const u8 *> m_data = nullptr;
    Mut<usize> m_cursor = 0;
    Mut<usize> m_data_size = 0;
    Mut<Vec<u8>> m_owning_vector;
    Mut<StorageType> m_storage_type = StorageType::NonOwning;
  };

  inline auto StreamReader::read(Mut<void *> buffer, const usize size) -> Result<void>
  {
    if (m_cursor + size > m_data_size) [[unlikely]]
    {
      return fail("Unexpected EOF while reading");
    }

    std::memcpy(buffer, &m_data[m_cursor], size);
    m_cursor += size;

    return {};
  }

  template<typename T>
  [[nodiscard("Check for EOF")]]
  inline auto StreamReader::read() -> Result<T>
  {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable to read via memcpy");

    constexpr const usize SIZE = sizeof(T);

    if (m_cursor + SIZE > m_data_size) [[unlikely]]
    {
      return fail("Unexpected EOF while reading");
    }

    Mut<T> value;
    std::memcpy(&value, &m_data[m_cursor], SIZE);
    m_cursor += SIZE;

    return value;
  }

} // namespace IACore