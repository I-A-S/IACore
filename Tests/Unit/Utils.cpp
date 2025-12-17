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

#include <IACore/Utils.hpp>

#include <IACore/IATest.hpp>

using namespace IACore;

// -----------------------------------------------------------------------------
// Test Structs for Hashing (Must be defined at Global Scope)
// -----------------------------------------------------------------------------

struct TestVec3
{
    FLOAT32 x, y, z;

    // Equality operator required for hash maps, though strictly
    // the hash function itself doesn't need it, it's good practice to test both.
    bool operator==(const TestVec3 &other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Inject the hash specialization into the ankerl namespace
// This proves the macro works structurally
IA_MAKE_HASHABLE(TestVec3, &TestVec3::x, &TestVec3::y, &TestVec3::z);

// -----------------------------------------------------------------------------
// Test Block Definition
// -----------------------------------------------------------------------------

IAT_BEGIN_BLOCK(Core, Utils)

// -------------------------------------------------------------------------
// 1. Binary <-> Hex String Conversion
// -------------------------------------------------------------------------
BOOL TestHexConversion()
{
    // A. Binary To Hex
    UINT8 bin[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xFF};
    String hex = IACore::Utils::BinaryToHexString(bin);

    IAT_CHECK_EQ(hex, String("DEADBEEF00FF"));

    // B. Hex To Binary (Valid Upper)
    auto resUpper = IACore::Utils::HexStringToBinary("DEADBEEF00FF");
    IAT_CHECK(resUpper.has_value());
    IAT_CHECK_EQ(resUpper->size(), (SIZE_T) 6);
    IAT_CHECK_EQ((*resUpper)[0], 0xDE);
    IAT_CHECK_EQ((*resUpper)[5], 0xFF);

    // C. Hex To Binary (Valid Lower/Mixed)
    auto resLower = IACore::Utils::HexStringToBinary("deadbeef00ff");
    IAT_CHECK(resLower.has_value());
    IAT_CHECK_EQ((*resLower)[0], 0xDE);

    // D. Round Trip Integrity
    Vector<UINT8> original = {1, 2, 3, 4, 5};
    String s = IACore::Utils::BinaryToHexString(original);
    auto back = IACore::Utils::HexStringToBinary(s);
    IAT_CHECK(back.has_value());
    IAT_CHECK_EQ(original.size(), back->size());
    IAT_CHECK_EQ(original[2], (*back)[2]);

    return TRUE;
}

// -------------------------------------------------------------------------
// 2. Hex Error Handling
// -------------------------------------------------------------------------
BOOL TestHexErrors()
{
    // Odd Length
    auto odd = IACore::Utils::HexStringToBinary("ABC");
    IAT_CHECK_NOT(odd.has_value());

    // Invalid Characters
    auto invalid = IACore::Utils::HexStringToBinary("ZZTOP");
    IAT_CHECK_NOT(invalid.has_value());

    // Empty string is valid (empty vector)
    auto empty = IACore::Utils::HexStringToBinary("");
    IAT_CHECK(empty.has_value());
    IAT_CHECK_EQ(empty->size(), (SIZE_T) 0);

    return TRUE;
}

// -------------------------------------------------------------------------
// 3. Algorithms: Sorting
// -------------------------------------------------------------------------
BOOL TestSort()
{
    Vector<int> nums = {5, 1, 4, 2, 3};

    IACore::Utils::Sort(nums);

    IAT_CHECK_EQ(nums[0], 1);
    IAT_CHECK_EQ(nums[1], 2);
    IAT_CHECK_EQ(nums[2], 3);
    IAT_CHECK_EQ(nums[3], 4);
    IAT_CHECK_EQ(nums[4], 5);

    return TRUE;
}

// -------------------------------------------------------------------------
// 4. Algorithms: Binary Search (Left/Right)
// -------------------------------------------------------------------------
BOOL TestBinarySearch()
{
    // Must be sorted for Binary Search
    Vector<int> nums = {10, 20, 20, 20, 30};

    // Search Left (Lower Bound) -> First element >= value
    auto itLeft = IACore::Utils::BinarySearchLeft(nums, 20);
    IAT_CHECK(itLeft != nums.end());
    IAT_CHECK_EQ(*itLeft, 20);
    IAT_CHECK_EQ(std::distance(nums.begin(), itLeft), 1); // Index 1 is first 20

    // Search Right (Upper Bound) -> First element > value
    auto itRight = IACore::Utils::BinarySearchRight(nums, 20);
    IAT_CHECK(itRight != nums.end());
    IAT_CHECK_EQ(*itRight, 30);                            // Points to 30
    IAT_CHECK_EQ(std::distance(nums.begin(), itRight), 4); // Index 4

    // Search for non-existent
    auto itFail = IACore::Utils::BinarySearchLeft(nums, 99);
    IAT_CHECK(itFail == nums.end());

    return TRUE;
}

// -------------------------------------------------------------------------
// 5. Hashing Basics
// -------------------------------------------------------------------------
BOOL TestHashBasics()
{
    UINT64 h1 = IACore::Utils::ComputeHash(10, 20.5f, "Hello");
    UINT64 h2 = IACore::Utils::ComputeHash(10, 20.5f, "Hello");
    UINT64 h3 = IACore::Utils::ComputeHash(10, 20.5f, "World");

    // Determinism
    IAT_CHECK_EQ(h1, h2);

    // Differentiation
    IAT_CHECK_NEQ(h1, h3);

    // Order sensitivity (Golden ratio combine should care about order)
    // Hash(A, B) != Hash(B, A)
    UINT64 orderA = IACore::Utils::ComputeHash(1, 2);
    UINT64 orderB = IACore::Utils::ComputeHash(2, 1);
    IAT_CHECK_NEQ(orderA, orderB);

    return TRUE;
}

// -------------------------------------------------------------------------
// 6. Macro Verification (IA_MAKE_HASHABLE)
// -------------------------------------------------------------------------
BOOL TestHashMacro()
{
    TestVec3 v1{1.0f, 2.0f, 3.0f};
    TestVec3 v2{1.0f, 2.0f, 3.0f};
    TestVec3 v3{1.0f, 2.0f, 4.0f};

    ankerl::unordered_dense::hash<TestVec3> hasher;

    UINT64 h1 = hasher(v1);
    UINT64 h2 = hasher(v2);
    UINT64 h3 = hasher(v3);

    IAT_CHECK_EQ(h1, h2);  // Same content = same hash
    IAT_CHECK_NEQ(h1, h3); // Different content = different hash

    // -------------------------------------------------------------
    // Verify ComputeHash integration
    // -------------------------------------------------------------

    UINT64 hManual = 0;
    IACore::Utils::HashCombine(hManual, v1);

    UINT64 hWrapper = IACore::Utils::ComputeHash(v1);

    // This proves ComputeHash found the specialization and mixed it correctly
    IAT_CHECK_EQ(hManual, hWrapper);

    // Verify the avalanche effect took place (hWrapper should NOT be h1)
    IAT_CHECK_NEQ(h1, hWrapper);

    return TRUE;
}

// -------------------------------------------------------------------------
// Registration
// -------------------------------------------------------------------------
IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(TestHexConversion);
IAT_ADD_TEST(TestHexErrors);
IAT_ADD_TEST(TestSort);
IAT_ADD_TEST(TestBinarySearch);
IAT_ADD_TEST(TestHashBasics);
IAT_ADD_TEST(TestHashMacro);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, Utils)
