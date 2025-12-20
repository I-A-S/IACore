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
    class DataOps
    {
      public:
        enum class CompressionType
        {
            None,
            Gzip,
            Zlib
        };

      public:
        STATIC UINT32 Hash_FNV1A(IN CONST String &string);
        STATIC UINT32 Hash_FNV1A(IN Span<CONST UINT8> data);

        STATIC UINT32 Hash_xxHash(IN CONST String &string);
        STATIC UINT32 Hash_xxHash(IN Span<CONST UINT8> data);

        STATIC UINT32 CRC32(IN Span<CONST UINT8> data);

        STATIC CompressionType DetectCompression(IN Span<CONST UINT8> data);

        STATIC Expected<Vector<UINT8>, String> GZipInflate(IN Span<CONST UINT8> data);
        STATIC Expected<Vector<UINT8>, String> GZipDeflate(IN Span<CONST UINT8> data);

        STATIC Expected<Vector<UINT8>, String> ZlibInflate(IN Span<CONST UINT8> data);
        STATIC Expected<Vector<UINT8>, String> ZlibDeflate(IN Span<CONST UINT8> data);

        STATIC Expected<Vector<UINT8>, String> ZstdInflate(IN Span<CONST UINT8> data);
        STATIC Expected<Vector<UINT8>, String> ZstdDeflate(IN Span<CONST UINT8> data);
    };
} // namespace IACore