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

#include <IACore/CLI.hpp>
#include <IACore/IATest.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, CLI)

auto test_basic_traversal() -> bool
{
  const Vec<String> args = {"ignored", "one", "two", "three"};
  CLIParser parser(args);

  IAT_CHECK(parser.remaining());

  IAT_CHECK_EQ(String(parser.next()), "one");
  IAT_CHECK(parser.remaining());

  IAT_CHECK_EQ(String(parser.next()), "two");
  IAT_CHECK(parser.remaining());

  IAT_CHECK_EQ(String(parser.next()), "three");

  IAT_CHECK_NOT(parser.remaining());

  IAT_CHECK_EQ(String(parser.next()), "");

  return true;
}

auto test_peek() -> bool
{
  const Vec<String> args = {"ignored", "peek_val", "next_val"};
  CLIParser parser(args);

  IAT_CHECK_EQ(String(parser.peek()), "peek_val");
  IAT_CHECK(parser.remaining());

  IAT_CHECK_EQ(String(parser.next()), "peek_val");

  IAT_CHECK_EQ(String(parser.peek()), "next_val");
  IAT_CHECK_EQ(String(parser.next()), "next_val");

  IAT_CHECK_NOT(parser.remaining());

  return true;
}

auto test_consume() -> bool
{
  const Vec<String> args = {"ignored", "-v", "--output", "file.txt"};
  CLIParser parser(args);

  IAT_CHECK_NOT(parser.consume("-x"));

  IAT_CHECK_EQ(String(parser.peek()), "-v");

  IAT_CHECK(parser.consume("-v"));

  IAT_CHECK_EQ(String(parser.peek()), "--output");

  IAT_CHECK(parser.consume("--output"));

  IAT_CHECK_EQ(String(parser.next()), "file.txt");

  IAT_CHECK_NOT(parser.remaining());

  return true;
}

auto test_empty() -> bool
{
  const Vec<String> args = {};
  CLIParser parser(args);

  IAT_CHECK_NOT(parser.remaining());
  IAT_CHECK_EQ(String(parser.peek()), "");
  IAT_CHECK_EQ(String(parser.next()), "");
  IAT_CHECK_NOT(parser.consume("-help"));

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_basic_traversal);
IAT_ADD_TEST(test_peek);
IAT_ADD_TEST(test_consume);
IAT_ADD_TEST(test_empty);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, CLI)