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

#include <IACore/Logger.hpp>
#include <IACore/IACore.hpp>
#include <IACore/FileOps.hpp>

namespace IACore
{
    Logger::ELogLevel Logger::s_logLevel{Logger::ELogLevel::INFO};
    std::ofstream Logger::s_logFile{};

    VOID Logger::Initialize()
    {
    }

    VOID Logger::Terminate()
    {
        if (s_logFile.is_open())
        {
            s_logFile.flush();
            s_logFile.close();
        }
    }

    BOOL Logger::EnableLoggingToDisk(IN PCCHAR filePath)
    {
        if (s_logFile.is_open())
        {
            s_logFile.flush();
            s_logFile.close();
        }
        s_logFile.open(filePath);
        return s_logFile.is_open();
    }

    VOID Logger::SetLogLevel(IN ELogLevel logLevel)
    {
        s_logLevel = logLevel;
    }

    VOID Logger::FlushLogs()
    {
        std::cout.flush();
        if (s_logFile)
            s_logFile.flush();
    }

    VOID Logger::LogInternal(IN PCCHAR prefix, IN PCCHAR tag, IN String &&msg)
    {
        const auto outLine = std::format("[{:>8.3f}]: [{}]: {}", GetSecondsCount(), tag, msg);
        std::cout << prefix << outLine << "\033[39m\n";
        if (s_logFile)
        {
            s_logFile.write(outLine.data(), outLine.size());
            s_logFile.put('\n');
            s_logFile.flush();
        }
    }
} // namespace IACore