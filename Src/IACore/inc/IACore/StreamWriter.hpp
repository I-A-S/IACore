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
    class StreamWriter
    {
        enum class EStorageType
        {
            NON_OWNING,
            OWNING_FILE,
            OWNING_VECTOR,
        };

      public:
        BOOL Write(IN UINT8 byte, IN SIZE_T count);
        BOOL Write(IN PCVOID buffer, IN SIZE_T size);
        template<typename T> BOOL Write(IN CONST T &value);

        PCUINT8 Data() CONST
        {
            return m_buffer;
        }

        SIZE_T Cursor() CONST
        {
            return m_cursor;
        }

      public:
        StreamWriter();
        explicit StreamWriter(IN Span<UINT8> data);
        explicit StreamWriter(IN CONST FilePath &path);
        ~StreamWriter();

      private:
        PUINT8 m_buffer{};
        SIZE_T m_cursor{};
        SIZE_T m_capacity{};
        FilePath m_filePath{};
        Vector<UINT8> m_owningVector;
        CONST EStorageType m_storageType;
    };

    template<typename T> BOOL StreamWriter::Write(IN CONST T &value)
    {
        return Write(&value, sizeof(T));
    }
} // namespace IACore