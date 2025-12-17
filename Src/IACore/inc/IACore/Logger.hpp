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

#pragma once

#include <IACore/PCH.hpp>

namespace IACore
{
    class Logger
    {
      public:
        enum class ELogLevel
        {
            VERBOSE,
            INFO,
            WARN,
            ERROR
        };

      public:
        STATIC BOOL EnableLoggingToDisk(IN PCCHAR filePath);
        STATIC VOID SetLogLevel(IN ELogLevel logLevel);

        template<typename... Args> STATIC VOID Status(FormatterString<Args...> fmt, Args &&...args)
        {
            LogStatus(std::vformat(fmt.get(), std::make_format_args(args...)));
        }

        template<typename... Args> STATIC VOID Info(FormatterString<Args...> fmt, Args &&...args)
        {
            LogInfo(std::vformat(fmt.get(), std::make_format_args(args...)));
        }

        template<typename... Args> STATIC VOID Warn(FormatterString<Args...> fmt, Args &&...args)
        {
            LogWarn(std::vformat(fmt.get(), std::make_format_args(args...)));
        }

        template<typename... Args> STATIC VOID Error(FormatterString<Args...> fmt, Args &&...args)
        {
            LogError(std::vformat(fmt.get(), std::make_format_args(args...)));
        }

        STATIC VOID FlushLogs();

      private:
#if IA_DISABLE_LOGGING > 0
        STATIC VOID LogStatus(IN String &&msg)
        {
            UNUSED(msg);
        }

        STATIC VOID LogInfo(IN String &&msg)
        {
            UNUSED(msg);
        }

        STATIC VOID LogWarn(IN String &&msg)
        {
            UNUSED(msg);
        }

        STATIC VOID LogError(IN String &&msg)
        {
            UNUSED(msg);
        }
#else
        STATIC VOID LogStatus(IN String &&msg)
        {
            if (s_logLevel <= ELogLevel::VERBOSE)
                LogInternal(__CC_WHITE, "STATUS", IA_MOVE(msg));
        }

        STATIC VOID LogInfo(IN String &&msg)
        {
            if (s_logLevel <= ELogLevel::INFO)
                LogInternal(__CC_GREEN, "INFO", IA_MOVE(msg));
        }

        STATIC VOID LogWarn(IN String &&msg)
        {
            if (s_logLevel <= ELogLevel::WARN)
                LogInternal(__CC_YELLOW, "WARN", IA_MOVE(msg));
        }

        STATIC VOID LogError(IN String &&msg)
        {
            if (s_logLevel <= ELogLevel::ERROR)
                LogInternal(__CC_RED, "ERROR", IA_MOVE(msg));
        }
#endif

        STATIC VOID LogInternal(IN PCCHAR prefix, IN PCCHAR tag, IN String &&msg);

      private:
        STATIC ELogLevel s_logLevel;
        STATIC std::ofstream s_logFile;

        STATIC VOID Initialize();
        STATIC VOID Terminate();

        friend VOID Initialize();
        friend VOID Terminate();
    };
} // namespace IACore