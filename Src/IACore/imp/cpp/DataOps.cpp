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

#include <IACore/DataOps.hpp>
#include <IACore/Platform.hpp>

#include <zlib.h>
#include <zstd.h>

namespace IACore
{
    template<typename T> INLINE T ReadUnaligned(IN PCUINT8 ptr)
    {
        T v;
        std::memcpy(&v, ptr, sizeof(T));
        return v;
    }

    struct CRC32Tables
    {
        UINT32 table[8][256] = {};

        CONSTEVAL CRC32Tables()
        {
            CONSTEXPR UINT32 t = 0x82F63B78;

            for (UINT32 i = 0; i < 256; i++)
            {
                UINT32 crc = i;
                for (int j = 0; j < 8; j++)
                    crc = (crc >> 1) ^ ((crc & 1) ? t : 0);
                table[0][i] = crc;
            }

            for (int i = 0; i < 256; i++)
            {
                for (int slice = 1; slice < 8; slice++)
                {
                    UINT32 prev = table[slice - 1][i];
                    table[slice][i] = (prev >> 8) ^ table[0][prev & 0xFF];
                }
            }
        }
    };

    STATIC CONSTEXPR CRC32Tables CRC32_TABLES{};

#if IA_ARCH_X64
    INLINE UINT32 CRC32_x64_HW(IN Span<CONST UINT8> data)
    {
        CONST UINT8 *p = data.data();

        UINT32 crc = 0xFFFFFFFF;
        SIZE_T len = data.size();

        while (len >= 8)
        {
            UINT64 chunk = ReadUnaligned<UINT64>(p);
            crc = (UINT32) _mm_crc32_u64((UINT64) crc, chunk);
            p += 8;
            len -= 8;
        }

        while (len--)
            crc = _mm_crc32_u8(crc, *p++);

        return ~crc;
    }
#endif

#if IA_ARCH_ARM64
    __attribute__((target("+crc"))) INLINE UINT32 CRC32_ARM64_HW(IN Span<CONST UINT8> data)
    {
        CONST UINT8 *p = data.data();

        UINT32 crc = 0xFFFFFFFF;
        SIZE_T len = data.size();

        while (len >= 8)
        {
            UINT64 chunk = ReadUnaligned<UINT64>(p);
            crc = __crc32cd(crc, chunk);
            p += 8;
            len -= 8;
        }

        while (len--)
            crc = __crc32cb(crc, *p++);

        return ~crc;
    }
#endif

    INLINE UINT32 CRC32_Software_Slice8(IN Span<CONST UINT8> data)
    {
        CONST UINT8 *p = data.data();
        UINT32 crc = 0xFFFFFFFF;
        SIZE_T len = data.size();

        while (len >= 8)
        {
            UINT32 term1 = crc ^ ReadUnaligned<UINT32>(p);
            UINT32 term2 = ReadUnaligned<UINT32>(p + 4);

            crc = CRC32_TABLES.table[7][term1 & 0xFF] ^ CRC32_TABLES.table[6][(term1 >> 8) & 0xFF] ^
                  CRC32_TABLES.table[5][(term1 >> 16) & 0xFF] ^ CRC32_TABLES.table[4][(term1 >> 24)] ^
                  CRC32_TABLES.table[3][term2 & 0xFF] ^ CRC32_TABLES.table[2][(term2 >> 8) & 0xFF] ^
                  CRC32_TABLES.table[1][(term2 >> 16) & 0xFF] ^ CRC32_TABLES.table[0][(term2 >> 24)];

            p += 8;
            len -= 8;
        }

        while (len--)
            crc = (crc >> 8) ^ CRC32_TABLES.table[0][(crc ^ *p++) & 0xFF];

        return ~crc;
    }

    UINT32 DataOps::CRC32(IN Span<CONST UINT8> data)
    {
#if IA_ARCH_X64
        // IACore mandates AVX2 so no need to check
        // for Platform::GetCapabilities().HardwareCRC32
        return CRC32_x64_HW(data);
#elif IA_ARCH_ARM64
        if (Platform::GetCapabilities().HardwareCRC32)
            return CRC32_ARM64_HW(data);
#endif
        return CRC32_Software_Slice8(data);
    }
} // namespace IACore

namespace IACore
{
    CONSTEXPR UINT32 XXH_PRIME32_1 = 0x9E3779B1U;
    CONSTEXPR UINT32 XXH_PRIME32_2 = 0x85EBCA6BU;
    CONSTEXPR UINT32 XXH_PRIME32_3 = 0xC2B2AE35U;
    CONSTEXPR UINT32 XXH_PRIME32_4 = 0x27D4EB2FU;
    CONSTEXPR UINT32 XXH_PRIME32_5 = 0x165667B1U;

    INLINE UINT32 XXH32_Round(IN UINT32 seed, IN UINT32 input)
    {
        seed += input * XXH_PRIME32_2;
        seed = std::rotl(seed, 13);
        seed *= XXH_PRIME32_1;
        return seed;
    }

    UINT32 DataOps::Hash_xxHash(IN CONST String &string)
    {
        return Hash_xxHash(Span<CONST UINT8>(reinterpret_cast<PCUINT8>(string.data()), string.size()));
    }

    UINT32 DataOps::Hash_xxHash(IN Span<CONST UINT8> data)
    {
        CONST UINT8 *p = data.data();
        CONST UINT8 *CONST bEnd = p + data.size();
        UINT32 h32{};

        if (data.size() >= 16)
        {
            const UINT8 *const limit = bEnd - 16;

            UINT32 v1 = XXH_PRIME32_1 + XXH_PRIME32_2;
            UINT32 v2 = XXH_PRIME32_2;
            UINT32 v3 = 0;
            UINT32 v4 = -XXH_PRIME32_1;

            do
            {
                v1 = XXH32_Round(v1, ReadUnaligned<UINT32>(p));
                p += 4;
                v2 = XXH32_Round(v2, ReadUnaligned<UINT32>(p));
                p += 4;
                v3 = XXH32_Round(v3, ReadUnaligned<UINT32>(p));
                p += 4;
                v4 = XXH32_Round(v4, ReadUnaligned<UINT32>(p));
                p += 4;
            } while (p <= limit);

            h32 = std::rotl(v1, 1) + std::rotl(v2, 7) + std::rotl(v3, 12) + std::rotl(v4, 18);
        }
        else
            h32 = XXH_PRIME32_5;

        h32 += (UINT32) data.size();

        while (p + 4 <= bEnd)
        {
            h32 += ReadUnaligned<UINT32>(p) * XXH_PRIME32_3;
            h32 = std::rotl(h32, 17) * XXH_PRIME32_4;
            p += 4;
        }

        while (p < bEnd)
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
} // namespace IACore

namespace IACore
{
    // FNV-1a 32-bit Constants
    CONSTEXPR UINT32 FNV1A_32_PRIME = 0x01000193;
    CONSTEXPR UINT32 FNV1A_32_OFFSET = 0x811c9dc5;

    UINT32 DataOps::Hash_FNV1A(IN CONST String &string)
    {
        UINT32 hash = FNV1A_32_OFFSET;
        for (char c : string)
        {
            hash ^= static_cast<uint8_t>(c);
            hash *= FNV1A_32_PRIME;
        }
        return hash;
    }

    UINT32 DataOps::Hash_FNV1A(IN Span<CONST UINT8> data)
    {
        UINT32 hash = FNV1A_32_OFFSET;
        const uint8_t *ptr = static_cast<const uint8_t *>(data.data());

        for (size_t i = 0; i < data.size(); ++i)
        {
            hash ^= ptr[i];
            hash *= FNV1A_32_PRIME;
        }
        return hash;
    }
} // namespace IACore

namespace IACore
{
    DataOps::CompressionType DataOps::DetectCompression(IN Span<CONST UINT8> data)
    {
        if (data.size() < 2)
            return CompressionType::None;

        // Check for GZIP Magic Number (0x1F 0x8B)
        if (data[0] == 0x1F && data[1] == 0x8B)
            return CompressionType::Gzip;

        // Check for ZLIB Magic Number (starts with 0x78)
        // 0x78 = Deflate compression with 32k window size
        if (data[0] == 0x78 && (data[1] == 0x01 || data[1] == 0x9C || data[1] == 0xDA))
            return CompressionType::Zlib;

        return CompressionType::None;
    }

    Expected<Vector<UINT8>, String> DataOps::ZlibInflate(IN Span<CONST UINT8> data)
    {
        z_stream zs{};
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;

        // 15 + 32 = Auto-detect Gzip or Zlib
        if (inflateInit2(&zs, 15 + 32) != Z_OK)
            return MakeUnexpected("Failed to initialize zlib inflate");

        zs.next_in = const_cast<Bytef *>(data.data());
        zs.avail_in = static_cast<uInt>(data.size());

        Vector<UINT8> outBuffer;
        // Start with 2x input size.
        size_t guessSize = data.size() < 1024 ? data.size() * 4 : data.size() * 2;
        outBuffer.resize(guessSize);

        zs.next_out = reinterpret_cast<Bytef *>(outBuffer.data());
        zs.avail_out = static_cast<uInt>(outBuffer.size());

        int ret;
        do
        {
            if (zs.avail_out == 0)
            {
                size_t currentPos = zs.total_out;

                size_t newSize = outBuffer.size() * 2;
                outBuffer.resize(newSize);

                zs.next_out = reinterpret_cast<Bytef *>(outBuffer.data() + currentPos);

                zs.avail_out = static_cast<uInt>(newSize - currentPos);
            }

            ret = inflate(&zs, Z_NO_FLUSH);

        } while (ret == Z_OK);

        inflateEnd(&zs);

        if (ret != Z_STREAM_END)
            return MakeUnexpected("Failed to inflate: corrupt data or stream error");

        outBuffer.resize(zs.total_out);

        return outBuffer;
    }

    Expected<Vector<UINT8>, String> DataOps::ZlibDeflate(IN Span<CONST UINT8> data)
    {
        z_stream zs{};
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;

        if (deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK)
            return MakeUnexpected("Failed to initialize zlib deflate");

        zs.next_in = const_cast<Bytef *>(data.data());
        zs.avail_in = static_cast<uInt>(data.size());

        Vector<UINT8> outBuffer;

        outBuffer.resize(deflateBound(&zs, data.size()));

        zs.next_out = reinterpret_cast<Bytef *>(outBuffer.data());
        zs.avail_out = static_cast<uInt>(outBuffer.size());

        int ret = deflate(&zs, Z_FINISH);

        if (ret != Z_STREAM_END)
        {
            deflateEnd(&zs);
            return MakeUnexpected("Failed to deflate, ran out of buffer memory");
        }

        outBuffer.resize(zs.total_out);

        deflateEnd(&zs);
        return outBuffer;
    }

    Expected<Vector<UINT8>, String> DataOps::ZstdInflate(IN Span<CONST UINT8> data)
    {
        unsigned long long const contentSize = ZSTD_getFrameContentSize(data.data(), data.size());

        if (contentSize == ZSTD_CONTENTSIZE_ERROR)
            return MakeUnexpected("Failed to inflate: Not valid ZSTD compressed data");

        if (contentSize != ZSTD_CONTENTSIZE_UNKNOWN)
        {
            // FAST PATH: We know the size
            Vector<UINT8> outBuffer;
            outBuffer.resize(static_cast<size_t>(contentSize));

            size_t const dSize = ZSTD_decompress(outBuffer.data(), outBuffer.size(), data.data(), data.size());

            if (ZSTD_isError(dSize))
                return MakeUnexpected(std::format("Failed to inflate: {}", ZSTD_getErrorName(dSize)));

            return outBuffer;
        }

        ZSTD_DCtx *dctx = ZSTD_createDCtx();
        Vector<UINT8> outBuffer;
        outBuffer.resize(data.size() * 2);

        ZSTD_inBuffer input = {data.data(), data.size(), 0};
        ZSTD_outBuffer output = {outBuffer.data(), outBuffer.size(), 0};

        size_t ret;
        do
        {
            ret = ZSTD_decompressStream(dctx, &output, &input);

            if (ZSTD_isError(ret))
            {
                ZSTD_freeDCtx(dctx);
                return MakeUnexpected(std::format("Failed to inflate: {}", ZSTD_getErrorName(ret)));
            }

            if (output.pos == output.size)
            {
                size_t newSize = outBuffer.size() * 2;
                outBuffer.resize(newSize);
                output.dst = outBuffer.data();
                output.size = newSize;
            }

        } while (ret != 0);

        outBuffer.resize(output.pos);
        ZSTD_freeDCtx(dctx);

        return outBuffer;
    }

    Expected<Vector<UINT8>, String> DataOps::ZstdDeflate(IN Span<CONST UINT8> data)
    {
        size_t const maxDstSize = ZSTD_compressBound(data.size());

        Vector<UINT8> outBuffer;
        outBuffer.resize(maxDstSize);

        size_t const compressedSize = ZSTD_compress(outBuffer.data(), maxDstSize, data.data(), data.size(), 3);

        if (ZSTD_isError(compressedSize))
            return MakeUnexpected(std::format("Failed to deflate: {}", ZSTD_getErrorName(compressedSize)));

        outBuffer.resize(compressedSize);
        return outBuffer;
    }

    Expected<Vector<UINT8>, String> DataOps::GZipDeflate(IN Span<CONST UINT8> data)
    {
        z_stream zs{};
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;

        // WindowBits = 15 + 16 (31) = Enforce GZIP encoding
        // MemLevel = 8 (default)
        // Strategy = Z_DEFAULT_STRATEGY
        if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
            return MakeUnexpected("Failed to initialize gzip deflate");

        zs.next_in = const_cast<Bytef *>(data.data());
        zs.avail_in = static_cast<uInt>(data.size());

        Vector<UINT8> outBuffer;

        outBuffer.resize(deflateBound(&zs, data.size()) + 1024); // Additional 1KB buffer for safety

        zs.next_out = reinterpret_cast<Bytef *>(outBuffer.data());
        zs.avail_out = static_cast<uInt>(outBuffer.size());

        int ret = deflate(&zs, Z_FINISH);

        if (ret != Z_STREAM_END)
        {
            deflateEnd(&zs);
            return MakeUnexpected("Failed to deflate");
        }

        outBuffer.resize(zs.total_out);

        deflateEnd(&zs);
        return outBuffer;
    }

    Expected<Vector<UINT8>, String> DataOps::GZipInflate(IN Span<CONST UINT8> data)
    {
        return ZlibInflate(data);
    }
} // namespace IACore