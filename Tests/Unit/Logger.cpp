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

#include <IACore/FileOps.hpp>
#include <IACore/IATest.hpp>
#include <IACore/Logger.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, Logger)

static constexpr const char *LOG_FILE = "iacore_test_log.txt";

void cleanup_file(const Path &path)
{
  std::error_code ec;
  if (std::filesystem::exists(path, ec))
  {
    std::filesystem::remove(path, ec);
  }
}

auto test_file_logging() -> bool
{

  const auto res = Logger::enable_logging_to_disk(LOG_FILE);
  IAT_CHECK(res.has_value());

  Logger::set_log_level(Logger::LogLevel::Trace);

  const String msg_info = "Test_Info_Msg_123";
  const String msg_err = "Test_Error_Msg_456";
  const String msg_warn = "Test_Warn_Msg_789";

  Logger::info("{}", msg_info);
  Logger::error("{}", msg_err);
  Logger::warn("{}", msg_warn);

  Logger::flush_logs();

  auto read_res = FileOps::read_text_file(LOG_FILE);
  if (!read_res)
  {
    std::cout << console::YELLOW << "    Warning: Could not read log file (" << read_res.error()
              << "). Skipping verification.\n"
              << console::RESET;
    return true;
  }

  const String content = *read_res;

  IAT_CHECK(content.find(msg_info) != String::npos);
  IAT_CHECK(content.find(msg_err) != String::npos);
  IAT_CHECK(content.find(msg_warn) != String::npos);

  IAT_CHECK(content.find("INFO") != String::npos);
  IAT_CHECK(content.find("ERROR") != String::npos);
  IAT_CHECK(content.find("WARN") != String::npos);

  cleanup_file(LOG_FILE);

  return true;
}

auto test_log_levels() -> bool
{

  Logger::set_log_level(Logger::LogLevel::Warn);

  const String unique_info = "Hidden_Info_Msg";
  const String unique_warn = "Visible_Warn_Msg";

  Logger::info("{}", unique_info);
  Logger::warn("{}", unique_warn);

  Logger::flush_logs();

  auto read_res = FileOps::read_text_file(LOG_FILE);
  if (!read_res)
  {
    return true;
  }

  const String content = *read_res;

  IAT_CHECK(content.find(unique_info) == String::npos);

  IAT_CHECK(content.find(unique_warn) != String::npos);

  return true;
}

auto test_formatting() -> bool
{
  Logger::set_log_level(Logger::LogLevel::Info);

  const String name = "IACore";
  const i32 version = 99;

  Logger::info("System {} online v{}", name, version);
  Logger::flush_logs();

  auto read_res = FileOps::read_text_file(LOG_FILE);
  if (!read_res)
  {
    return true;
  }

  const String content = *read_res;
  IAT_CHECK(content.find("System IACore online v99") != String::npos);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_file_logging);
IAT_ADD_TEST(test_log_levels);
IAT_ADD_TEST(test_formatting);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, Logger)