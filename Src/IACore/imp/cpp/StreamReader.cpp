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

#include <IACore/StreamReader.hpp>
#include <IACore/FileOps.hpp>
#include <IACore/Logger.hpp>

namespace IACore
{
    StreamReader::StreamReader(IN CONST FilePath &path) : m_storageType(EStorageType::OWNING_MMAP)
    {
        const auto t = FileOps::MapFile(path, m_dataSize);
        if (!t)
        {
            Logger::Error("Failed to memory map file {}", path.string());
            return;
        }
        m_data = *t;
    }

    StreamReader::StreamReader(IN Vector<UINT8> &&data)
        : m_owningVector(IA_MOVE(data)), m_storageType(EStorageType::OWNING_VECTOR)
    {
        m_data = m_owningVector.data();
        m_dataSize = m_owningVector.size();
    }

    StreamReader::StreamReader(IN Span<CONST UINT8> data)
        : m_data(data.data()), m_dataSize(data.size()), m_storageType(EStorageType::NON_OWNING)
    {
    }

    StreamReader::~StreamReader()
    {
        switch (m_storageType)
        {
        case EStorageType::OWNING_MMAP:
            FileOps::UnmapFile(m_data);
            break;

        case EStorageType::NON_OWNING:
        case EStorageType::OWNING_VECTOR:
            break;
        }
    }
} // namespace IACore