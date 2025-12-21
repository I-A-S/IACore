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

#include <IACore/IATest.hpp>

using namespace IACore;

// -----------------------------------------------------------------------------
// Test Block Definition
// -----------------------------------------------------------------------------

IAT_BEGIN_BLOCK(Core, DataOps)

BOOL TestCRC32()
{
    {
        String s = "123456789";
        Span<CONST UINT8> span(reinterpret_cast<PCUINT8>(s.data()), s.size());
        UINT32 result = DataOps::CRC32(span);

        IAT_CHECK_EQ(result, 0xE3069283);
    }

    {
        UINT32 result = DataOps::CRC32({});
        IAT_CHECK_EQ(result, 0U);
    }

    {
        std::vector<UINT8> buffer(33);
        for (size_t i = 1; i < 33; ++i)
            buffer[i] = (UINT8) i;

        std::vector<UINT8> refData(32);
        for (size_t i = 0; i < 32; ++i)
            refData[i] = (UINT8) (i + 1);

        UINT32 hashRef = DataOps::CRC32(Span<CONST UINT8>(refData.data(), refData.size()));

        UINT32 hashUnaligned = DataOps::CRC32(Span<CONST UINT8>(buffer.data() + 1, 32));

        IAT_CHECK_EQ(hashRef, hashUnaligned);
    }

    return TRUE;
}

BOOL TestHash_xxHash()
{
    {
        String s = "123456789";
        UINT32 result = DataOps::Hash_xxHash(s);
        IAT_CHECK_EQ(result, 0x937bad67);
    }

    {
        String s = "The quick brown fox jumps over the lazy dog";
        UINT32 result = DataOps::Hash_xxHash(s);
        IAT_CHECK_EQ(result, 0xE85EA4DE);
    }

    {
        String s = "Test";
        UINT32 r1 = DataOps::Hash_xxHash(s);
        UINT32 r2 = DataOps::Hash_xxHash(Span<CONST UINT8>((PCUINT8) s.data(), s.size()));
        IAT_CHECK_EQ(r1, r2);
    }

    return TRUE;
}

BOOL TestHash_FNV1A()
{
    {
        String s = "123456789";
        UINT32 result = DataOps::Hash_FNV1A(Span<CONST UINT8>((PCUINT8) s.data(), s.size()));
        IAT_CHECK_EQ(result, 0xbb86b11c);
    }

    {
        UINT32 result = DataOps::Hash_FNV1A(Span<CONST UINT8>{});
        IAT_CHECK_EQ(result, 0x811C9DC5);
    }

    return TRUE;
}

// -------------------------------------------------------------------------
// Registration
// -------------------------------------------------------------------------
IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(TestCRC32);
IAT_ADD_TEST(TestHash_FNV1A);
IAT_ADD_TEST(TestHash_xxHash);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, DataOps)
