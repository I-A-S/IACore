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

#include <IACore/IATest.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, StreamReader)

// -------------------------------------------------------------------------
// 1. Basic Primitive Reading (UINT8)
// -------------------------------------------------------------------------
BOOL TestReadUint8()
{
    UINT8 data[] = {0xAA, 0xBB, 0xCC};
    StreamReader reader(data);

    // Read First Byte
    auto val1 = reader.Read<UINT8>();
    IAT_CHECK(val1.has_value());
    IAT_CHECK_EQ(*val1, 0xAA);
    IAT_CHECK_EQ(reader.Cursor(), (SIZE_T) 1);

    // Read Second Byte
    auto val2 = reader.Read<UINT8>();
    IAT_CHECK_EQ(*val2, 0xBB);

    return TRUE;
}

// -------------------------------------------------------------------------
// 2. Multi-byte Reading (Endianness check)
// -------------------------------------------------------------------------
BOOL TestReadMultiByte()
{
    // 0x04030201 in Little Endian memory layout
    // IACore always assumes a Little Endian machine
    UINT8 data[] = {0x01, 0x02, 0x03, 0x04};
    StreamReader reader(data);

    auto val = reader.Read<UINT32>();
    IAT_CHECK(val.has_value());

    IAT_CHECK_EQ(*val, (UINT32) 0x04030201);

    IAT_CHECK_EQ(reader.Cursor(), (SIZE_T) 4);
    IAT_CHECK(reader.IsEOF());

    return TRUE;
}

// -------------------------------------------------------------------------
// 3. Floating Point (Approx check)
// -------------------------------------------------------------------------
BOOL TestReadFloat()
{
    FLOAT32 pi = 3.14159f;
    // Bit-cast float to bytes for setup
    UINT8 data[4];
    std::memcpy(data, &pi, 4);

    StreamReader reader(data);
    auto val = reader.Read<FLOAT32>();

    IAT_CHECK(val.has_value());
    IAT_CHECK_APPROX(*val, pi);

    return TRUE;
}

// -------------------------------------------------------------------------
// 4. Batch Buffer Reading
// -------------------------------------------------------------------------
BOOL TestReadBuffer()
{
    UINT8 src[] = {1, 2, 3, 4, 5};
    UINT8 dst[3] = {0};
    StreamReader reader(src);

    // Read 3 bytes into dst
    auto res = reader.Read(dst, 3);
    IAT_CHECK(res.has_value());

    // Verify dst content
    IAT_CHECK_EQ(dst[0], 1);
    IAT_CHECK_EQ(dst[1], 2);
    IAT_CHECK_EQ(dst[2], 3);

    // Verify cursor
    IAT_CHECK_EQ(reader.Cursor(), (SIZE_T) 3);

    return TRUE;
}

// -------------------------------------------------------------------------
// 5. Navigation (Seek, Skip, Remaining)
// -------------------------------------------------------------------------
BOOL TestNavigation()
{
    UINT8 data[10] = {0}; // Zero init
    StreamReader reader(data);

    IAT_CHECK_EQ(reader.Remaining(), (SIZE_T) 10);

    // Skip
    reader.Skip(5);
    IAT_CHECK_EQ(reader.Cursor(), (SIZE_T) 5);
    IAT_CHECK_EQ(reader.Remaining(), (SIZE_T) 5);

    // Skip clamping
    reader.Skip(100); // Should clamp to 10
    IAT_CHECK_EQ(reader.Cursor(), (SIZE_T) 10);
    IAT_CHECK(reader.IsEOF());

    // Seek
    reader.Seek(2);
    IAT_CHECK_EQ(reader.Cursor(), (SIZE_T) 2);
    IAT_CHECK_EQ(reader.Remaining(), (SIZE_T) 8);
    IAT_CHECK_NOT(reader.IsEOF());

    return TRUE;
}

// -------------------------------------------------------------------------
// 6. Error Handling (EOF Protection)
// -------------------------------------------------------------------------
BOOL TestBoundaryChecks()
{
    UINT8 data[] = {0x00, 0x00};
    StreamReader reader(data);

    // Valid read
    UNUSED(reader.Read<UINT16>());
    IAT_CHECK(reader.IsEOF());

    // Invalid Read Primitive
    auto val = reader.Read<UINT8>();
    IAT_CHECK_NOT(val.has_value()); // Should be unexpected

    // Invalid Batch Read
    UINT8 buf[1];
    auto batch = reader.Read(buf, 1);
    IAT_CHECK_NOT(batch.has_value());

    return TRUE;
}

// -------------------------------------------------------------------------
// Registration
// -------------------------------------------------------------------------
IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(TestReadUint8);
IAT_ADD_TEST(TestReadMultiByte);
IAT_ADD_TEST(TestReadFloat);
IAT_ADD_TEST(TestReadBuffer);
IAT_ADD_TEST(TestNavigation);
IAT_ADD_TEST(TestBoundaryChecks);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, StreamReader)
