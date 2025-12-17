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

#include <IACore/StreamWriter.hpp>
#include <IACore/Logger.hpp>

namespace IACore
{
    StreamWriter::StreamWriter() : m_storageType(EStorageType::OWNING_VECTOR)
    {
        m_owningVector.resize(m_capacity = 256);
        m_buffer = m_owningVector.data();
    }

    StreamWriter::StreamWriter(IN Span<UINT8> data)
        : m_buffer(data.data()), m_capacity(data.size()), m_storageType(EStorageType::NON_OWNING)
    {
    }

    StreamWriter::StreamWriter(IN CONST FilePath &path) : m_filePath(path), m_storageType(EStorageType::OWNING_FILE)
    {
        IA_RELEASE_ASSERT(!path.empty());
        const auto f = fopen(m_filePath.string().c_str(), "wb");
        if (!f)
        {
            Logger::Error("Failed to open file for writing {}", m_filePath.string().c_str());
            return;
        }
        fputc(0, f);
        fclose(f);

        m_owningVector.resize(m_capacity = 256);
        m_buffer = m_owningVector.data();
    }

    StreamWriter::~StreamWriter()
    {
        switch (m_storageType)
        {
        case EStorageType::OWNING_FILE: {
            IA_RELEASE_ASSERT(!m_filePath.empty());
            const auto f = fopen(m_filePath.string().c_str(), "wb");
            if (!f)
            {
                Logger::Error("Failed to open file for writing {}", m_filePath.string().c_str());
                return;
            }
            fwrite(m_owningVector.data(), 1, m_owningVector.size(), f);
            fclose(f);
        }
        break;

        case EStorageType::OWNING_VECTOR:
        case EStorageType::NON_OWNING:
            break;
        }
    }

#define HANDLE_OUT_OF_CAPACITY(_size)                                                                                  \
    if B_UNLIKELY ((m_cursor + _size) > m_capacity)                                                                    \
    {                                                                                                                  \
        if (m_storageType == EStorageType::NON_OWNING)                                                                 \
            return false;                                                                                              \
        m_owningVector.resize(m_capacity + (_size << 1));                                                              \
        m_capacity = m_owningVector.size();                                                                            \
        m_buffer = m_owningVector.data();                                                                              \
    }

    BOOL StreamWriter::Write(IN UINT8 byte, IN SIZE_T count)
    {
        HANDLE_OUT_OF_CAPACITY(count);
        memset(&m_buffer[m_cursor], byte, count);
        m_cursor += count;
        return true;
    }

    BOOL StreamWriter::Write(IN PCVOID buffer, IN SIZE_T size)
    {
        HANDLE_OUT_OF_CAPACITY(size);
        memcpy(&m_buffer[m_cursor], buffer, size);
        m_cursor += size;
        return true;
    }
} // namespace IACore