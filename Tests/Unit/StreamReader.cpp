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

#include <IACore/IATest.hpp>
#include <IACore/StreamReader.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, StreamReader)

auto test_read_uint8() -> bool
{
  u8 data[] = {0xAA, 0xBB, 0xCC};
  StreamReader reader(data);

  auto val1 = reader.read<u8>();
  IAT_CHECK(val1.has_value());
  IAT_CHECK_EQ(*val1, 0xAA);
  IAT_CHECK_EQ(reader.cursor(), static_cast<usize>(1));

  auto val2 = reader.read<u8>();
  IAT_CHECK(val2.has_value());
  IAT_CHECK_EQ(*val2, 0xBB);

  return true;
}

auto test_read_multi_byte() -> bool
{

  u8 data[] = {0x01, 0x02, 0x03, 0x04};
  StreamReader reader(data);

  auto val = reader.read<u32>();
  IAT_CHECK(val.has_value());

  IAT_CHECK_EQ(*val, static_cast<u32>(0x04030201));

  IAT_CHECK_EQ(reader.cursor(), static_cast<usize>(4));
  IAT_CHECK(reader.is_eof());

  return true;
}

auto test_read_float() -> bool
{
  const f32 pi = 3.14159f;

  u8 data[4];
  std::memcpy(data, &pi, 4);

  StreamReader reader(data);
  auto val = reader.read<f32>();

  IAT_CHECK(val.has_value());
  IAT_CHECK_APPROX(*val, pi);

  return true;
}

auto test_read_buffer() -> bool
{
  u8 src[] = {1, 2, 3, 4, 5};
  u8 dst[3] = {0};
  StreamReader reader(src);

  const auto res = reader.read(dst, 3);
  IAT_CHECK(res.has_value());

  IAT_CHECK_EQ(dst[0], 1);
  IAT_CHECK_EQ(dst[1], 2);
  IAT_CHECK_EQ(dst[2], 3);

  IAT_CHECK_EQ(reader.cursor(), static_cast<usize>(3));

  return true;
}

auto test_navigation() -> bool
{
  u8 data[10] = {0};
  StreamReader reader(data);

  IAT_CHECK_EQ(reader.remaining(), static_cast<usize>(10));

  reader.skip(5);
  IAT_CHECK_EQ(reader.cursor(), static_cast<usize>(5));
  IAT_CHECK_EQ(reader.remaining(), static_cast<usize>(5));

  reader.skip(100);
  IAT_CHECK_EQ(reader.cursor(), static_cast<usize>(10));
  IAT_CHECK(reader.is_eof());

  reader.seek(2);
  IAT_CHECK_EQ(reader.cursor(), static_cast<usize>(2));
  IAT_CHECK_EQ(reader.remaining(), static_cast<usize>(8));
  IAT_CHECK_NOT(reader.is_eof());

  return true;
}

auto test_boundary_checks() -> bool
{
  u8 data[] = {0x00, 0x00};
  StreamReader reader(data);

  (void) reader.read<u16>();
  IAT_CHECK(reader.is_eof());

  auto val = reader.read<u8>();
  IAT_CHECK_NOT(val.has_value());

  u8 buf[1];
  auto batch = reader.read(buf, 1);
  IAT_CHECK_NOT(batch.has_value());

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_read_uint8);
IAT_ADD_TEST(test_read_multi_byte);
IAT_ADD_TEST(test_read_float);
IAT_ADD_TEST(test_read_buffer);
IAT_ADD_TEST(test_navigation);
IAT_ADD_TEST(test_boundary_checks);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, StreamReader)