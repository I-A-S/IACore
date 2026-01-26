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
#include <IACore/SIMD.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, IntVec4)

auto test_constructors() -> bool
{
  IntVec4 v_broadcast(10);
  alignas(16) u32 store_buf[4];
  v_broadcast.store(store_buf);

  IAT_CHECK_EQ(store_buf[0], 10U);
  IAT_CHECK_EQ(store_buf[3], 10U);

  IntVec4 v_comp(1, 2, 3, 4);
  v_comp.store(store_buf);
  IAT_CHECK_EQ(store_buf[0], 1U);
  IAT_CHECK_EQ(store_buf[3], 4U);

  alignas(16) u32 src_buf[4] = {100, 200, 300, 400};
  IntVec4 v_load = IntVec4::load(src_buf);
  v_load.store(store_buf);
  IAT_CHECK_EQ(store_buf[1], 200U);

  return true;
}

auto test_arithmetic() -> bool
{
  const IntVec4 v1(10, 20, 30, 40);
  const IntVec4 v2(1, 2, 3, 4);

  IntVec4 v_add = v1 + v2;
  alignas(16) u32 res[4];
  v_add.store(res);
  IAT_CHECK_EQ(res[0], 11U);
  IAT_CHECK_EQ(res[3], 44U);

  IntVec4 v_sub = v1 - v2;
  v_sub.store(res);
  IAT_CHECK_EQ(res[0], 9U);

  IntVec4 v_mul = v1 * v2;
  v_mul.store(res);
  IAT_CHECK_EQ(res[0], 10U);
  IAT_CHECK_EQ(res[2], 90U);
  IAT_CHECK_EQ(res[3], 160U);

  return true;
}

auto test_bitwise() -> bool
{
  const IntVec4 v_all_ones(0xFFFFFFFF);
  const IntVec4 v_zero((u32) 0);
  const IntVec4 v_pattern(0xAAAAAAAA);

  alignas(16) u32 res[4];

  (v_all_ones & v_pattern).store(res);
  IAT_CHECK_EQ(res[0], 0xAAAAAAAAU);

  (v_zero | v_pattern).store(res);
  IAT_CHECK_EQ(res[0], 0xAAAAAAAAU);

  (v_all_ones ^ v_pattern).store(res);
  IAT_CHECK_EQ(res[0], 0x55555555U);

  (~v_pattern).store(res);
  IAT_CHECK_EQ(res[0], 0x55555555U);

  const IntVec4 v_shift(1);
  (v_shift << 1).store(res);
  IAT_CHECK_EQ(res[0], 2U);

  const IntVec4 v_shift_right(4);
  (v_shift_right >> 1).store(res);
  IAT_CHECK_EQ(res[0], 2U);

  return true;
}

auto test_saturation() -> bool
{
  const u32 max = 0xFFFFFFFF;
  const IntVec4 v_high(max - 10);
  const IntVec4 v_add(20);

  alignas(16) u32 res[4];

  v_high.sat_add(v_add).store(res);
  IAT_CHECK_EQ(res[0], max);

  const IntVec4 v_low(10);
  const IntVec4 v_sub(20);
  v_low.sat_sub(v_sub).store(res);
  IAT_CHECK_EQ(res[0], 0U);

  return true;
}

auto test_advanced_ops() -> bool
{
  const IntVec4 v(0, 50, 100, 150);
  alignas(16) u32 res[4];

  v.clamp(40, 110).store(res);
  IAT_CHECK_EQ(res[0], 40U);
  IAT_CHECK_EQ(res[1], 50U);
  IAT_CHECK_EQ(res[2], 100U);
  IAT_CHECK_EQ(res[3], 110U);

  const IntVec4 a(2);
  const IntVec4 b(10);
  const IntVec4 c(5);
  a.mult_add(b, c).store(res);
  IAT_CHECK_EQ(res[0], 25U);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_constructors);
IAT_ADD_TEST(test_arithmetic);
IAT_ADD_TEST(test_bitwise);
IAT_ADD_TEST(test_saturation);
IAT_ADD_TEST(test_advanced_ops);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, IntVec4)