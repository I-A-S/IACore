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
#include <IACore/ProcessOps.hpp>

using namespace IACore;

#if IA_PLATFORM_WINDOWS
#  define CMD_ECHO_EXE "cmd.exe"
#  define CMD_ARG_PREFIX "/c echo"
#  define NULL_DEVICE "NUL"
#else
#  define CMD_ECHO_EXE "/bin/echo"
#  define CMD_ARG_PREFIX ""
#  define NULL_DEVICE "/dev/null"
#endif

IAT_BEGIN_BLOCK(Core, ProcessOps)

auto test_basic_run() -> bool
{

  String captured;

  const auto result = ProcessOps::spawn_process_sync(CMD_ECHO_EXE, CMD_ARG_PREFIX " HelloIA",
                                                     [&](StringView line) { captured = line; });

  IAT_CHECK(result.has_value());
  IAT_CHECK_EQ(*result, 0);

  IAT_CHECK(captured.find("HelloIA") != String::npos);

  return true;
}

auto test_arguments() -> bool
{
  Vec<String> lines;

  String args = String(CMD_ARG_PREFIX) + " one two";
  if (!args.empty() && args[0] == ' ')
  {
    args.erase(0, 1);
  }

  const auto result =
      ProcessOps::spawn_process_sync(CMD_ECHO_EXE, args, [&](StringView line) { lines.push_back(String(line)); });

  IAT_CHECK_EQ(*result, 0);
  IAT_CHECK(lines.size() > 0);

  IAT_CHECK(lines[0].find("one two") != String::npos);

  return true;
}

auto test_exit_codes() -> bool
{

  String cmd;
  String arg;

#if IA_PLATFORM_WINDOWS
  cmd = "cmd.exe";
  arg = "/c exit 42";
#else
  cmd = "/bin/sh";
  arg = "-c \"exit 42\"";
#endif

  const auto result = ProcessOps::spawn_process_sync(cmd, arg, [](StringView) {});

  IAT_CHECK(result.has_value());
  IAT_CHECK_EQ(*result, 42);

  return true;
}

auto test_missing_exe() -> bool
{

  const auto result = ProcessOps::spawn_process_sync("sdflkjghsdflkjg", "", [](StringView) {});

#if IA_PLATFORM_WINDOWS
  IAT_CHECK_NOT(result.has_value());
#else

  IAT_CHECK(result.has_value());
  IAT_CHECK_EQ(*result, 127);
#endif

  return true;
}

auto test_large_output() -> bool
{

  String massive_string;
  massive_string.reserve(5000);
  for (i32 i = 0; i < 500; ++i)
  {
    massive_string += "1234567890";
  }

  String cmd;
  String arg;

#if IA_PLATFORM_WINDOWS
  cmd = "cmd.exe";

  arg = "/c echo " + massive_string;
#else
  cmd = "/bin/echo";
  arg = massive_string;
#endif

  String captured;
  const auto result = ProcessOps::spawn_process_sync(cmd, arg, [&](StringView line) { captured += line; });

  IAT_CHECK(result.has_value());
  IAT_CHECK_EQ(*result, 0);

  IAT_CHECK_EQ(captured.length(), massive_string.length());

  return true;
}

auto test_multi_line() -> bool
{

  String cmd;
  String arg;
#if IA_PLATFORM_WINDOWS
  cmd = "cmd.exe";
  arg = "/c \"echo LineA && echo LineB\"";
#else
  cmd = "/bin/sh";
  arg = "-c \"echo LineA; echo LineB\"";
#endif

  i32 line_count = 0;
  bool found_a = false;
  bool found_b = false;

  const auto res = ProcessOps::spawn_process_sync(cmd, arg, [&](StringView line) {
    line_count++;
    if (line.find("LineA") != String::npos)
    {
      found_a = true;
    }
    if (line.find("LineB") != String::npos)
    {
      found_b = true;
    }
  });
  IAT_CHECK(res.has_value());

  IAT_CHECK(found_a);
  IAT_CHECK(found_b);

  IAT_CHECK(line_count >= 2);

  return true;
}

auto test_complex_arguments() -> bool
{

  const String complex_args = "-DDEFINED_MSG=\\\"Hello World\\\" -v path/to/file";

  const String cmd = CMD_ECHO_EXE;

  String final_args;
#if IA_PLATFORM_WINDOWS
  final_args = "/c echo " + complex_args;
#else
  final_args = complex_args;
#endif

  String captured;
  const auto result = ProcessOps::spawn_process_sync(cmd, final_args, [&](StringView line) { captured += line; });

  IAT_CHECK(result.has_value());
  IAT_CHECK_EQ(*result, 0);

  IAT_CHECK(captured.find("Hello World") != String::npos);
  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_basic_run);
IAT_ADD_TEST(test_arguments);
IAT_ADD_TEST(test_exit_codes);
IAT_ADD_TEST(test_missing_exe);
IAT_ADD_TEST(test_large_output);
IAT_ADD_TEST(test_multi_line);
IAT_ADD_TEST(test_complex_arguments);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, ProcessOps)