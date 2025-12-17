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

#include <IACore/Environment.hpp>

#include <IACore/IATest.hpp>

using namespace IACore;

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
static const char *TEST_KEY = "IA_TEST_ENV_VAR_12345";
static const char *TEST_VAL = "Hello World";

// -----------------------------------------------------------------------------
// Test Block Definition
// -----------------------------------------------------------------------------

IAT_BEGIN_BLOCK(Core, Environment)

// -------------------------------------------------------------------------
// 1. Basic Set and Get (The Happy Path)
// -------------------------------------------------------------------------
BOOL TestBasicCycle()
{
    // 1. Ensure clean slate
    Environment::Unset(TEST_KEY);
    IAT_CHECK_NOT(Environment::Exists(TEST_KEY));

    // 2. Set
    BOOL setRes = Environment::Set(TEST_KEY, TEST_VAL);
    IAT_CHECK(setRes);
    IAT_CHECK(Environment::Exists(TEST_KEY));

    // 3. Find (Optional)
    auto opt = Environment::Find(TEST_KEY);
    IAT_CHECK(opt.has_value());
    IAT_CHECK_EQ(*opt, String(TEST_VAL));

    // 4. Get (Direct)
    String val = Environment::Get(TEST_KEY);
    IAT_CHECK_EQ(val, String(TEST_VAL));

    // Cleanup
    Environment::Unset(TEST_KEY);
    return TRUE;
}

// -------------------------------------------------------------------------
// 2. Overwriting Values
// -------------------------------------------------------------------------
BOOL TestOverwrite()
{
    Environment::Set(TEST_KEY, "ValueA");
    IAT_CHECK_EQ(Environment::Get(TEST_KEY), String("ValueA"));

    // Overwrite
    Environment::Set(TEST_KEY, "ValueB");
    IAT_CHECK_EQ(Environment::Get(TEST_KEY), String("ValueB"));

    Environment::Unset(TEST_KEY);
    return TRUE;
}

// -------------------------------------------------------------------------
// 3. Unset Logic
// -------------------------------------------------------------------------
BOOL TestUnset()
{
    Environment::Set(TEST_KEY, "To Be Deleted");
    IAT_CHECK(Environment::Exists(TEST_KEY));

    BOOL unsetRes = Environment::Unset(TEST_KEY);
    IAT_CHECK(unsetRes);

    // Verify it is actually gone
    IAT_CHECK_NOT(Environment::Exists(TEST_KEY));

    // Find should return nullopt
    auto opt = Environment::Find(TEST_KEY);
    IAT_CHECK_NOT(opt.has_value());

    return TRUE;
}

// -------------------------------------------------------------------------
// 4. Default Value Fallbacks
// -------------------------------------------------------------------------
BOOL TestDefaults()
{
    const char *ghostKey = "IA_THIS_KEY_DOES_NOT_EXIST";

    // Ensure it really doesn't exist
    Environment::Unset(ghostKey);

    // Test Get with implicit default ("")
    String empty = Environment::Get(ghostKey);
    IAT_CHECK(empty.empty());

    // Test Get with explicit default
    String fallback = Environment::Get(ghostKey, "MyDefault");
    IAT_CHECK_EQ(fallback, String("MyDefault"));

    return TRUE;
}

// -------------------------------------------------------------------------
// 5. Empty Strings vs Null/Unset
// -------------------------------------------------------------------------
// Does Set(Key, "") create an existing empty variable, or unset it?
// Standard POSIX/Windows API behavior is that it EXISTS, but is empty.
BOOL TestEmptyValue()
{
    Environment::Set(TEST_KEY, "");

#if IA_PLATFORM_WINDOWS
    // Windows behavior: Setting to empty string removes the variable
    // IAT_CHECK_NOT(Environment::Exists(TEST_KEY));
    // auto opt = Environment::Find(TEST_KEY);
    // IAT_CHECK_NOT(opt.has_value());
#else
    // POSIX behavior: Variable exists but is empty
    IAT_CHECK(Environment::Exists(TEST_KEY));
    auto opt = Environment::Find(TEST_KEY);
    IAT_CHECK(opt.has_value());
    IAT_CHECK(opt->empty());
#endif

    // Cleanup
    Environment::Unset(TEST_KEY);
    IAT_CHECK_NOT(Environment::Exists(TEST_KEY));

    return TRUE;
}

// -------------------------------------------------------------------------
// 6. Validation / Bad Input
// -------------------------------------------------------------------------
BOOL TestBadInput()
{
    // Setting an empty key should fail gracefully
    BOOL res = Environment::Set("", "Value");
    IAT_CHECK_NOT(res);

    // Unsetting an empty key should fail
    BOOL resUnset = Environment::Unset("");
    IAT_CHECK_NOT(resUnset);

    return TRUE;
}

// -------------------------------------------------------------------------
// Registration
// -------------------------------------------------------------------------
IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(TestBasicCycle);
IAT_ADD_TEST(TestOverwrite);
IAT_ADD_TEST(TestUnset);
IAT_ADD_TEST(TestDefaults);
IAT_ADD_TEST(TestEmptyValue);
IAT_ADD_TEST(TestBadInput);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, Environment)
