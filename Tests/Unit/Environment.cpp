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

#include <IACore/Environment.hpp>
#include <IACore/IATest.hpp>

using namespace IACore;

static constexpr const char *TEST_KEY = "IA_TEST_ENV_VAR_12345";
static constexpr const char *TEST_VAL = "Hello World";

IAT_BEGIN_BLOCK(Core, Environment)

auto test_basic_cycle() -> bool
{

  (void) Environment::unset(TEST_KEY);
  IAT_CHECK_NOT(Environment::exists(TEST_KEY));

  const auto set_res = Environment::set(TEST_KEY, TEST_VAL);
  IAT_CHECK(set_res.has_value());
  IAT_CHECK(Environment::exists(TEST_KEY));

  const auto opt = Environment::find(TEST_KEY);
  IAT_CHECK(opt.has_value());
  IAT_CHECK_EQ(*opt, String(TEST_VAL));

  const String val = Environment::get(TEST_KEY);
  IAT_CHECK_EQ(val, String(TEST_VAL));

  (void) Environment::unset(TEST_KEY);
  return true;
}

auto test_overwrite() -> bool
{
  (void) Environment::set(TEST_KEY, "ValueA");
  IAT_CHECK_EQ(Environment::get(TEST_KEY), String("ValueA"));

  (void) Environment::set(TEST_KEY, "ValueB");
  IAT_CHECK_EQ(Environment::get(TEST_KEY), String("ValueB"));

  (void) Environment::unset(TEST_KEY);
  return true;
}

auto test_unset() -> bool
{
  (void) Environment::set(TEST_KEY, "To Be Deleted");
  IAT_CHECK(Environment::exists(TEST_KEY));

  const auto unset_res = Environment::unset(TEST_KEY);
  IAT_CHECK(unset_res.has_value());

  IAT_CHECK_NOT(Environment::exists(TEST_KEY));

  const auto opt = Environment::find(TEST_KEY);
  IAT_CHECK_NOT(opt.has_value());

  return true;
}

auto test_defaults() -> bool
{
  const char *ghost_key = "IA_THIS_KEY_DOES_NOT_EXIST";

  (void) Environment::unset(ghost_key);

  const String empty = Environment::get(ghost_key);
  IAT_CHECK(empty.empty());

  const String fallback = Environment::get(ghost_key, "MyDefault");
  IAT_CHECK_EQ(fallback, String("MyDefault"));

  return true;
}

auto test_empty_value() -> bool
{
  (void) Environment::set(TEST_KEY, "");

#if IA_PLATFORM_WINDOWS

#else

  IAT_CHECK(Environment::exists(TEST_KEY));
  const auto opt = Environment::find(TEST_KEY);
  IAT_CHECK(opt.has_value());
  IAT_CHECK(opt->empty());
#endif

  (void) Environment::unset(TEST_KEY);
  IAT_CHECK_NOT(Environment::exists(TEST_KEY));

  return true;
}

auto test_bad_input() -> bool
{

  const auto res = Environment::set("", "Value");
  IAT_CHECK_NOT(res.has_value());

  const auto res_unset = Environment::unset("");
  IAT_CHECK_NOT(res_unset.has_value());

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_basic_cycle);
IAT_ADD_TEST(test_overwrite);
IAT_ADD_TEST(test_unset);
IAT_ADD_TEST(test_defaults);
IAT_ADD_TEST(test_empty_value);
IAT_ADD_TEST(test_bad_input);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, Environment)