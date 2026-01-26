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
#include <IACore/IATest.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, DataOps)

auto test_crc32() -> bool
{
  {
    const String s = "123456789";
    const Span<const u8> span(reinterpret_cast<const u8 *>(s.data()), s.size());
    const u32 result = DataOps::crc32(span);

    IAT_CHECK_EQ(result, 0xE3069283);
  }

  {
    const u32 result = DataOps::crc32({});
    IAT_CHECK_EQ(result, 0U);
  }

  {
    Vec<u8> buffer(33);
    for (usize i = 1; i < 33; ++i)
    {
      buffer[i] = static_cast<u8>(i);
    }

    Vec<u8> ref_data(32);
    for (usize i = 0; i < 32; ++i)
    {
      ref_data[i] = static_cast<u8>(i + 1);
    }

    const u32 hash_ref = DataOps::crc32(Span<const u8>(ref_data.data(), ref_data.size()));

    const u32 hash_unaligned = DataOps::crc32(Span<const u8>(buffer.data() + 1, 32));

    IAT_CHECK_EQ(hash_ref, hash_unaligned);
  }

  return true;
}

auto test_hash_xxhash() -> bool
{
  {
    const String s = "123456789";
    const u32 result = DataOps::hash_xxhash(s);
    IAT_CHECK_EQ(result, 0x937bad67);
  }

  {
    const String s = "The quick brown fox jumps over the lazy dog";
    const u32 result = DataOps::hash_xxhash(s);
    IAT_CHECK_EQ(result, 0xE85EA4DE);
  }

  {
    const String s = "Test";
    const u32 r1 = DataOps::hash_xxhash(s);
    const u32 r2 = DataOps::hash_xxhash(Span<const u8>(reinterpret_cast<const u8 *>(s.data()), s.size()));
    IAT_CHECK_EQ(r1, r2);
  }

  return true;
}

auto test_hash_fnv1a() -> bool
{
  {
    const String s = "123456789";
    const u32 result = DataOps::hash_fnv1a(Span<const u8>(reinterpret_cast<const u8 *>(s.data()), s.size()));
    IAT_CHECK_EQ(result, 0xbb86b11c);
  }

  {
    const u32 result = DataOps::hash_fnv1a(Span<const u8>{});
    IAT_CHECK_EQ(result, 0x811C9DC5);
  }

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_crc32);
IAT_ADD_TEST(test_hash_fnv1a);
IAT_ADD_TEST(test_hash_xxhash);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, DataOps)