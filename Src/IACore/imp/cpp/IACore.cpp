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

#include <IACore/IACore.hpp>
#include <IACore/Logger.hpp>

namespace IACore
{
    HighResTimePoint g_startTime{};
    std::thread::id g_mainThreadID{};

    VOID Initialize()
    {
        g_mainThreadID = std::this_thread::get_id();
        g_startTime = HighResClock::now();
        Logger::Initialize();
    }

    VOID Terminate()
    {
        Logger::Terminate();
    }

    UINT64 GetUnixTime()
    {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    }

    UINT64 GetTicksCount()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(HighResClock::now() - g_startTime).count();
    }

    FLOAT64 GetSecondsCount()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(HighResClock::now() - g_startTime).count();
    }

    FLOAT32 GetRandom()
    {
        return static_cast<FLOAT32>(rand()) / static_cast<FLOAT32>(RAND_MAX);
    }

    UINT32 GetRandom(IN UINT32 seed)
    {
        srand(seed);
        return (UINT32) GetRandom(0, UINT32_MAX);
    }

    INT64 GetRandom(IN INT64 min, IN INT64 max)
    {
        const auto t = static_cast<FLOAT32>(rand()) / static_cast<FLOAT32>(RAND_MAX);
        return min + (max - min) * t;
    }

    VOID Sleep(IN UINT64 milliseconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    BOOL IsMainThread()
    {
        return std::this_thread::get_id() == g_mainThreadID;
    }
} // namespace IACore