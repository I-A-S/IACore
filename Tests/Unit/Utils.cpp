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
#include <IACore/Utils.hpp>

using namespace IACore;

struct TestVec3
{
  f32 x, y, z;

  bool operator==(const TestVec3 &other) const
  {
    return x == other.x && y == other.y && z == other.z;
  }
};

IA_MAKE_HASHABLE(TestVec3, &TestVec3::x, &TestVec3::y, &TestVec3::z);

IAT_BEGIN_BLOCK(Core, Utils)

auto test_hex_conversion() -> bool
{

  u8 bin[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xFF};
  String hex = Utils::binary_to_hex_string(bin);

  IAT_CHECK_EQ(hex, String("DEADBEEF00FF"));

  auto res_upper = Utils::hex_string_to_binary("DEADBEEF00FF");
  IAT_CHECK(res_upper.has_value());
  IAT_CHECK_EQ(res_upper->size(), static_cast<usize>(6));
  IAT_CHECK_EQ((*res_upper)[0], 0xDE);
  IAT_CHECK_EQ((*res_upper)[5], 0xFF);

  auto res_lower = Utils::hex_string_to_binary("deadbeef00ff");
  IAT_CHECK(res_lower.has_value());
  IAT_CHECK_EQ((*res_lower)[0], 0xDE);

  Vec<u8> original = {1, 2, 3, 4, 5};
  String s = Utils::binary_to_hex_string(original);
  auto back = Utils::hex_string_to_binary(s);
  IAT_CHECK(back.has_value());
  IAT_CHECK_EQ(original.size(), back->size());
  IAT_CHECK_EQ(original[2], (*back)[2]);

  return true;
}

auto test_hex_errors() -> bool
{

  auto odd = Utils::hex_string_to_binary("ABC");
  IAT_CHECK_NOT(odd.has_value());

  auto invalid = Utils::hex_string_to_binary("ZZTOP");
  IAT_CHECK_NOT(invalid.has_value());

  auto empty = Utils::hex_string_to_binary("");
  IAT_CHECK(empty.has_value());
  IAT_CHECK_EQ(empty->size(), static_cast<usize>(0));

  return true;
}

auto test_sort() -> bool
{
  Vec<i32> nums = {5, 1, 4, 2, 3};

  Utils::sort(nums);

  IAT_CHECK_EQ(nums[0], 1);
  IAT_CHECK_EQ(nums[1], 2);
  IAT_CHECK_EQ(nums[2], 3);
  IAT_CHECK_EQ(nums[3], 4);
  IAT_CHECK_EQ(nums[4], 5);

  return true;
}

auto test_binary_search() -> bool
{

  Vec<i32> nums = {10, 20, 20, 20, 30};

  auto it_left = Utils::binary_search_left(nums, 20);
  IAT_CHECK(it_left != nums.end());
  IAT_CHECK_EQ(*it_left, 20);
  IAT_CHECK_EQ(std::distance(nums.begin(), it_left), 1);

  auto it_right = Utils::binary_search_right(nums, 20);
  IAT_CHECK(it_right != nums.end());
  IAT_CHECK_EQ(*it_right, 30);
  IAT_CHECK_EQ(std::distance(nums.begin(), it_right), 4);

  auto it_fail = Utils::binary_search_left(nums, 99);
  IAT_CHECK(it_fail == nums.end());

  return true;
}

auto test_hash_basics() -> bool
{
  u64 h1 = Utils::compute_hash(10, 20.5f, "Hello");
  u64 h2 = Utils::compute_hash(10, 20.5f, "Hello");
  u64 h3 = Utils::compute_hash(10, 20.5f, "World");

  IAT_CHECK_EQ(h1, h2);

  IAT_CHECK_NEQ(h1, h3);

  u64 order_a = Utils::compute_hash(1, 2);
  u64 order_b = Utils::compute_hash(2, 1);
  IAT_CHECK_NEQ(order_a, order_b);

  return true;
}

auto test_hash_macro() -> bool
{
  TestVec3 v1{1.0f, 2.0f, 3.0f};
  TestVec3 v2{1.0f, 2.0f, 3.0f};
  TestVec3 v3{1.0f, 2.0f, 4.0f};

  ankerl::unordered_dense::hash<TestVec3> hasher;

  u64 h1 = hasher(v1);
  u64 h2 = hasher(v2);
  u64 h3 = hasher(v3);

  IAT_CHECK_EQ(h1, h2);
  IAT_CHECK_NEQ(h1, h3);

  u64 h_manual = 0;
  Utils::hash_combine(h_manual, v1);

  u64 h_wrapper = Utils::compute_hash(v1);

  IAT_CHECK_EQ(h_manual, h_wrapper);

  IAT_CHECK_NEQ(h1, h_wrapper);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_hex_conversion);
IAT_ADD_TEST(test_hex_errors);
IAT_ADD_TEST(test_sort);
IAT_ADD_TEST(test_binary_search);
IAT_ADD_TEST(test_hash_basics);
IAT_ADD_TEST(test_hash_macro);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, Utils)