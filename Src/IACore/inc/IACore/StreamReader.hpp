// IACore-OSS; The Core Library for All IA Open Source Projects
// Copyright (C) 2025 IAS (ias@iasoft.dev)
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
    class StreamReader
    {
        enum class EStorageType
        {
            NON_OWNING,
            OWNING_MMAP,
            OWNING_VECTOR,
        };

      public:
        INLINE Expected<VOID, String> Read(IN PVOID buffer, IN SIZE_T size);
        template<typename T> NO_DISCARD("Check for EOF") Expected<T, String> Read();

        VOID Skip(SIZE_T amount)
        {
            m_cursor = std::min(m_cursor + amount, m_dataSize);
        }

        VOID Seek(SIZE_T pos)
        {
            m_cursor = (pos > m_dataSize) ? m_dataSize : pos;
        }

        SIZE_T Cursor() CONST
        {
            return m_cursor;
        }

        SIZE_T Size() CONST
        {
            return m_dataSize;
        }

        SIZE_T Remaining() CONST
        {
            return m_dataSize - m_cursor;
        }

        BOOL IsEOF() CONST
        {
            return m_cursor >= m_dataSize;
        }

      public:
        StreamReader(IN CONST FilePath &path);
        explicit StreamReader(IN Vector<UINT8> &&data);
        explicit StreamReader(IN Span<CONST UINT8> data);
        ~StreamReader();

      private:
        PCUINT8 m_data{};
        SIZE_T m_cursor{};
        SIZE_T m_dataSize{};
        Vector<UINT8> m_owningVector;
        CONST EStorageType m_storageType;
    };

    Expected<VOID, String> StreamReader::Read(IN PVOID buffer, IN SIZE_T size)
    {
        if B_UNLIKELY ((m_cursor + size > m_dataSize))
            return MakeUnexpected(String("Unexpected EOF while reading"));

        std::memcpy(buffer, &m_data[m_cursor], size);
        m_cursor += size;

        return {};
    }

    template<typename T> NO_DISCARD("Check for EOF") Expected<T, String> StreamReader::Read()
    {
        constexpr SIZE_T size = sizeof(T);

        if B_UNLIKELY ((m_cursor + size > m_dataSize))
            return MakeUnexpected(String("Unexpected EOF while reading"));

        T value;
        std::memcpy(&value, &m_data[m_cursor], size);
        m_cursor += size;

        return value;
    }
} // namespace IACore