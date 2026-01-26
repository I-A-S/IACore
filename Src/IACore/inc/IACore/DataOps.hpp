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
    static auto hash_fnv1a(Ref<String> string) -> u32;
    static auto hash_fnv1a(Ref<Span<const u8>> data) -> u32;

    static auto hash_xxhash(Ref<String> string, const u32 seed = 0) -> u32;
    static auto hash_xxhash(Ref<Span<const u8>> data, const u32 seed = 0) -> u32;

    static auto crc32(Ref<Span<const u8>> data) -> u32;

    static auto detect_compression(const Span<const u8> data) -> CompressionType;

    static auto gzip_inflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>;
    static auto gzip_deflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>;

    static auto zlib_inflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>;
    static auto zlib_deflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>;

    static auto zstd_inflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>;
    static auto zstd_deflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>;
  };
} // namespace IACore