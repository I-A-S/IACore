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

#pragma once

#include <IACore/PCH.hpp>

#define IA_LOG_SET_FILE(path) IACore::Logger::enable_logging_to_disk(path)
#define IA_LOG_SET_LEVEL(level) IACore::Logger::set_log_level(IACore::Logger::LogLevel::level)

#define IA_LOG_TRACE(...) IACore::Logger::trace(__VA_ARGS__)
#define IA_LOG_DEBUG(...) IACore::Logger::debug(__VA_ARGS__)
#define IA_LOG_INFO(...) IACore::Logger::info(__VA_ARGS__)
#define IA_LOG_WARN(...) IACore::Logger::warn(__VA_ARGS__)
#define IA_LOG_ERROR(...) IACore::Logger::error(__VA_ARGS__)

namespace IACore
{
  class Logger
  {
public:
    enum class LogLevel
    {
      Trace,
      Debug,
      Info,
      Warn,
      Error
    };

public:
    static auto enable_logging_to_disk(const char *file_path) -> Result<void>;
    static auto set_log_level(const LogLevel log_level) -> void;

    template<typename... Args>
    static auto trace(const std::format_string<Args...> fmt, ForwardRef<Args>... args) -> void
    {
      log_trace(std::vformat(fmt.get(), std::make_format_args(args...)));
    }

    template<typename... Args>
    static auto debug(const std::format_string<Args...> fmt, ForwardRef<Args>... args) -> void
    {
      log_debug(std::vformat(fmt.get(), std::make_format_args(args...)));
    }

    template<typename... Args> static auto info(const std::format_string<Args...> fmt, ForwardRef<Args>... args) -> void
    {
      log_info(std::vformat(fmt.get(), std::make_format_args(args...)));
    }

    template<typename... Args> static auto warn(const std::format_string<Args...> fmt, ForwardRef<Args>... args) -> void
    {
      log_warn(std::vformat(fmt.get(), std::make_format_args(args...)));
    }

    template<typename... Args>
    static auto error(const std::format_string<Args...> fmt, ForwardRef<Args>... args) -> void
    {
      log_error(std::vformat(fmt.get(), std::make_format_args(args...)));
    }

    static auto flush_logs() -> void;

private:
#if IA_DISABLE_LOGGING > 0
    static auto log_trace(ForwardRef<String> msg) -> void
    {
      IA_UNUSED(msg);
    }

    static auto log_debug(ForwardRef<String> msg) -> void
    {
      IA_UNUSED(msg);
    }

    static auto log_info(ForwardRef<String> msg) -> void
    {
      IA_UNUSED(msg);
    }

    static auto log_warn(ForwardRef<String> msg) -> void
    {
      IA_UNUSED(msg);
    }

    static auto log_error(ForwardRef<String> msg) -> void
    {
      IA_UNUSED(msg);
    }
#else
    static auto log_trace(ForwardRef<String> msg) -> void
    {
      if (m_log_level <= LogLevel::Trace)
        log_internal(console::RESET, "TRACE", std::move(msg));
    }

    static auto log_debug(ForwardRef<String> msg) -> void
    {
      if (m_log_level <= LogLevel::Debug)
        log_internal(console::CYAN, "DEBUG", std::move(msg));
    }

    static auto log_info(ForwardRef<String> msg) -> void
    {
      if (m_log_level <= LogLevel::Info)
        log_internal(console::GREEN, "INFO", std::move(msg));
    }

    static auto log_warn(ForwardRef<String> msg) -> void
    {
      if (m_log_level <= LogLevel::Warn)
        log_internal(console::YELLOW, "WARN", std::move(msg));
    }

    static auto log_error(ForwardRef<String> msg) -> void
    {
      if (m_log_level <= LogLevel::Error)
        log_internal(console::RED, "ERROR", std::move(msg));
    }
#endif

    static auto log_internal(const char *prefix, const char *tag, ForwardRef<String> msg) -> void;

private:
    static Mut<LogLevel> m_log_level;
    static Mut<std::ofstream> m_log_file;

    static auto initialize() -> void;
    static auto terminate() -> void;

    friend void initialize();
    friend void terminate();
  };
} // namespace IACore