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

#include <IACore/DataOps.hpp>
#include <IACore/Platform.hpp>

#include <bit>
#include <cstring>
#include <zlib.h>
#include <zstd.h>

#if IA_ARCH_X64
#  include <immintrin.h>
#endif

#if IA_ARCH_ARM64
#  include <arm_acle.h>
#endif

namespace IACore
{
  template<typename T> [[nodiscard]] inline auto read_unaligned(const u8 *ptr) -> T
  {
    Mut<T> v;
    std::memcpy(&v, ptr, sizeof(T));
    return v;
  }

  struct Crc32Tables
  {
    Mut<u32> table[8][256] = {};

    consteval Crc32Tables()
    {
      constexpr const u32 T = 0x82F63B78;

      for (Mut<u32> i = 0; i < 256; i++)
      {
        Mut<u32> crc = i;
        for (Mut<i32> j = 0; j < 8; j++)
        {
          crc = (crc >> 1) ^ ((crc & 1) ? T : 0);
        }
        table[0][i] = crc;
      }

      for (Mut<i32> i = 0; i < 256; i++)
      {
        for (Mut<i32> slice = 1; slice < 8; slice++)
        {
          const u32 prev = table[slice - 1][i];
          table[slice][i] = (prev >> 8) ^ table[0][prev & 0xFF];
        }
      }
    }
  };

  static constexpr const Crc32Tables CRC32_TABLES{};

#if IA_ARCH_X64
  inline auto crc32_x64_hw(Ref<Span<const u8>> data) -> u32
  {
    Mut<const u8 *> p = data.data();

    Mut<u32> crc = 0xFFFFFFFF;
    Mut<usize> len = data.size();

    while (len >= 8)
    {
      const u64 chunk = read_unaligned<u64>(p);
      crc = static_cast<u32>(_mm_crc32_u64(static_cast<u64>(crc), chunk));
      p += 8;
      len -= 8;
    }

    while (len--)
    {
      crc = _mm_crc32_u8(crc, *p++);
    }

    return ~crc;
  }
#endif

#if IA_ARCH_ARM64
  __attribute__((target("+crc"))) inline auto crc32_arm64_hw(Ref<Span<const u8>> data) -> u32
  {
    Mut<const u8 *> p = data.data();

    Mut<u32> crc = 0xFFFFFFFF;
    Mut<usize> len = data.size();

    while (len >= 8)
    {
      const u64 chunk = read_unaligned<u64>(p);
      crc = __crc32cd(crc, chunk);
      p += 8;
      len -= 8;
    }

    while (len--)
    {
      crc = __crc32cb(crc, *p++);
    }

    return ~crc;
  }
#endif

  inline auto crc32_software_slice8(Ref<Span<const u8>> data) -> u32
  {
    Mut<const u8 *> p = data.data();
    Mut<u32> crc = 0xFFFFFFFF;
    Mut<usize> len = data.size();

    while (len >= 8)
    {
      const u32 term1 = crc ^ read_unaligned<u32>(p);
      const u32 term2 = read_unaligned<u32>(p + 4);

      crc = CRC32_TABLES.table[7][term1 & 0xFF] ^ CRC32_TABLES.table[6][(term1 >> 8) & 0xFF] ^
            CRC32_TABLES.table[5][(term1 >> 16) & 0xFF] ^ CRC32_TABLES.table[4][(term1 >> 24)] ^
            CRC32_TABLES.table[3][term2 & 0xFF] ^ CRC32_TABLES.table[2][(term2 >> 8) & 0xFF] ^
            CRC32_TABLES.table[1][(term2 >> 16) & 0xFF] ^ CRC32_TABLES.table[0][(term2 >> 24)];

      p += 8;
      len -= 8;
    }

    while (len--)
    {
      crc = (crc >> 8) ^ CRC32_TABLES.table[0][(crc ^ *p++) & 0xFF];
    }

    return ~crc;
  }

  auto DataOps::crc32(Ref<Span<const u8>> data) -> u32
  {
#if IA_ARCH_X64
    // IACore mandates AVX2 so no need to check
    return crc32_x64_hw(data);
#elif IA_ARCH_ARM64
    if (Platform::GetCapabilities().HardwareCRC32)
    {
      return crc32_arm64_hw(data);
    }
#endif
    return crc32_software_slice8(data);
  }

  constexpr const u32 XXH_PRIME32_1 = 0x9E3779B1U;
  constexpr const u32 XXH_PRIME32_2 = 0x85EBCA77U;
  constexpr const u32 XXH_PRIME32_3 = 0xC2B2AE3DU;
  constexpr const u32 XXH_PRIME32_4 = 0x27D4EB2FU;
  constexpr const u32 XXH_PRIME32_5 = 0x165667B1U;

  inline auto xxh32_round(Mut<u32> seed, const u32 input) -> u32
  {
    seed += input * XXH_PRIME32_2;
    seed = std::rotl(seed, 13);
    seed *= XXH_PRIME32_1;
    return seed;
  }

  auto DataOps::hash_xxhash(Ref<String> string, const u32 seed) -> u32
  {
    return hash_xxhash(Span<const u8>(reinterpret_cast<const u8 *>(string.data()), string.length()), seed);
  }

  auto DataOps::hash_xxhash(Ref<Span<const u8>> data, const u32 seed) -> u32
  {
    Mut<const u8 *> p = data.data();
    const u8 *b_end = p + data.size();
    Mut<u32> h32{};

    if (data.size() >= 16)
    {
      const u8 *limit = b_end - 16;

      Mut<u32> v1 = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
      Mut<u32> v2 = seed + XXH_PRIME32_2;
      Mut<u32> v3 = seed + 0;
      Mut<u32> v4 = seed - XXH_PRIME32_1;

      do
      {
        v1 = xxh32_round(v1, read_unaligned<u32>(p));
        p += 4;
        v2 = xxh32_round(v2, read_unaligned<u32>(p));
        p += 4;
        v3 = xxh32_round(v3, read_unaligned<u32>(p));
        p += 4;
        v4 = xxh32_round(v4, read_unaligned<u32>(p));
        p += 4;
      } while (p <= limit);

      h32 = std::rotl(v1, 1) + std::rotl(v2, 7) + std::rotl(v3, 12) + std::rotl(v4, 18);
    }
    else
    {
      h32 = seed + XXH_PRIME32_5;
    }

    h32 += static_cast<u32>(data.size());

    while (p + 4 <= b_end)
    {
      const u32 t = read_unaligned<u32>(p) * XXH_PRIME32_3;
      h32 += t;
      h32 = std::rotl(h32, 17) * XXH_PRIME32_4;
      p += 4;
    }

    while (p < b_end)
    {
      h32 += (*p++) * XXH_PRIME32_5;
      h32 = std::rotl(h32, 11) * XXH_PRIME32_1;
    }

    h32 ^= h32 >> 15;
    h32 *= XXH_PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= XXH_PRIME32_3;
    h32 ^= h32 >> 16;

    return h32;
  }

  constexpr const u32 FNV1A_32_PRIME = 0x01000193;
  constexpr const u32 FNV1A_32_OFFSET = 0x811c9dc5;

  auto DataOps::hash_fnv1a(Ref<String> string) -> u32
  {
    Mut<u32> hash = FNV1A_32_OFFSET;
    for (const char c : string)
    {
      hash ^= static_cast<u8>(c);
      hash *= FNV1A_32_PRIME;
    }
    return hash;
  }

  auto DataOps::hash_fnv1a(Ref<Span<const u8>> data) -> u32
  {
    Mut<u32> hash = FNV1A_32_OFFSET;
    const u8 *ptr = data.data();

    for (Mut<usize> i = 0; i < data.size(); ++i)
    {
      hash ^= ptr[i];
      hash *= FNV1A_32_PRIME;
    }
    return hash;
  }

  auto DataOps::detect_compression(const Span<const u8> data) -> CompressionType
  {
    if (data.size() < 2)
    {
      return CompressionType::None;
    }

    if (data[0] == 0x1F && data[1] == 0x8B)
    {
      return CompressionType::Gzip;
    }

    if (data[0] == 0x78 && (data[1] == 0x01 || data[1] == 0x9C || data[1] == 0xDA))
    {
      return CompressionType::Zlib;
    }

    return CompressionType::None;
  }

  auto DataOps::zlib_inflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>
  {
    Mut<z_stream> zs{};
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;

    if (inflateInit2(&zs, 15 + 32) != Z_OK)
    {
      return fail("Failed to initialize zlib inflate");
    }

    zs.next_in = const_cast<Bytef *>(data.data());
    zs.avail_in = static_cast<uInt>(data.size());

    Mut<Vec<u8>> out_buffer;
    const usize guess_size = data.size() < 1024 ? data.size() * 4 : data.size() * 2;
    out_buffer.resize(guess_size);

    zs.next_out = reinterpret_cast<Bytef *>(out_buffer.data());
    zs.avail_out = static_cast<uInt>(out_buffer.size());

    Mut<int> ret;
    do
    {
      if (zs.avail_out == 0)
      {
        const usize current_pos = zs.total_out;
        const usize new_size = out_buffer.size() * 2;
        out_buffer.resize(new_size);

        zs.next_out = reinterpret_cast<Bytef *>(out_buffer.data() + current_pos);
        zs.avail_out = static_cast<uInt>(new_size - current_pos);
      }

      ret = inflate(&zs, Z_NO_FLUSH);

    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END)
    {
      return fail("Failed to inflate: corrupt data or stream error");
    }

    out_buffer.resize(zs.total_out);

    return out_buffer;
  }

  auto DataOps::zlib_deflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>
  {
    Mut<z_stream> zs{};
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;

    if (deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK)
    {
      return fail("Failed to initialize zlib deflate");
    }

    zs.next_in = const_cast<Bytef *>(data.data());
    zs.avail_in = static_cast<uInt>(data.size());

    Mut<Vec<u8>> out_buffer;
    out_buffer.resize(deflateBound(&zs, static_cast<uLong>(data.size())));

    zs.next_out = reinterpret_cast<Bytef *>(out_buffer.data());
    zs.avail_out = static_cast<uInt>(out_buffer.size());

    const int ret = deflate(&zs, Z_FINISH);

    if (ret != Z_STREAM_END)
    {
      deflateEnd(&zs);
      return fail("Failed to deflate, ran out of buffer memory");
    }

    out_buffer.resize(zs.total_out);

    deflateEnd(&zs);
    return out_buffer;
  }

  auto DataOps::zstd_inflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>
  {
    const unsigned long long content_size = ZSTD_getFrameContentSize(data.data(), data.size());

    if (content_size == ZSTD_CONTENTSIZE_ERROR)
    {
      return fail("Failed to inflate: Not valid ZSTD compressed data");
    }

    if (content_size != ZSTD_CONTENTSIZE_UNKNOWN)
    {
      Mut<Vec<u8>> out_buffer;
      out_buffer.resize(static_cast<usize>(content_size));

      const usize d_size = ZSTD_decompress(out_buffer.data(), out_buffer.size(), data.data(), data.size());

      if (ZSTD_isError(d_size))
      {
        return fail("Failed to inflate: {}", ZSTD_getErrorName(d_size));
      }

      return out_buffer;
    }

    Mut<ZSTD_DCtx *> dctx = ZSTD_createDCtx();
    Mut<Vec<u8>> out_buffer;
    out_buffer.resize(data.size() * 2);

    Mut<ZSTD_inBuffer> input = {data.data(), data.size(), 0};
    Mut<ZSTD_outBuffer> output = {out_buffer.data(), out_buffer.size(), 0};

    Mut<usize> ret;
    do
    {
      ret = ZSTD_decompressStream(dctx, &output, &input);

      if (ZSTD_isError(ret))
      {
        ZSTD_freeDCtx(dctx);
        return fail("Failed to inflate: {}", ZSTD_getErrorName(ret));
      }

      if (output.pos == output.size)
      {
        const usize new_size = out_buffer.size() * 2;
        out_buffer.resize(new_size);
        output.dst = out_buffer.data();
        output.size = new_size;
      }

    } while (ret != 0);

    out_buffer.resize(output.pos);
    ZSTD_freeDCtx(dctx);

    return out_buffer;
  }

  auto DataOps::zstd_deflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>
  {
    const usize max_dst_size = ZSTD_compressBound(data.size());

    Mut<Vec<u8>> out_buffer;
    out_buffer.resize(max_dst_size);

    const usize compressed_size = ZSTD_compress(out_buffer.data(), max_dst_size, data.data(), data.size(), 3);

    if (ZSTD_isError(compressed_size))
    {
      return fail("Failed to deflate: {}", ZSTD_getErrorName(compressed_size));
    }

    out_buffer.resize(compressed_size);
    return out_buffer;
  }

  auto DataOps::gzip_deflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>
  {
    Mut<z_stream> zs{};
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;

    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    {
      return fail("Failed to initialize gzip deflate");
    }

    zs.next_in = const_cast<Bytef *>(data.data());
    zs.avail_in = static_cast<uInt>(data.size());

    Mut<Vec<u8>> out_buffer;

    out_buffer.resize(deflateBound(&zs, static_cast<uLong>(data.size())) + 1024);

    zs.next_out = reinterpret_cast<Bytef *>(out_buffer.data());
    zs.avail_out = static_cast<uInt>(out_buffer.size());

    const int ret = deflate(&zs, Z_FINISH);

    if (ret != Z_STREAM_END)
    {
      deflateEnd(&zs);
      return fail("Failed to deflate");
    }

    out_buffer.resize(zs.total_out);

    deflateEnd(&zs);
    return out_buffer;
  }

  auto DataOps::gzip_inflate(Ref<Span<const u8>> data) -> Result<Vec<u8>>
  {
    return zlib_inflate(data);
  }

} // namespace IACore