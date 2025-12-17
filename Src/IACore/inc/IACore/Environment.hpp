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
    class Environment
    {
      public:
        STATIC Optional<String> Find(CONST String &name)
        {
#if IA_PLATFORM_WINDOWS
            DWORD bufferSize = GetEnvironmentVariableA(name.c_str(), nullptr, 0);

            if (bufferSize == 0)
            {
                // DWORD 0 means failed
                return std::nullopt;
            }

            // 2. Allocate (bufferSize includes the null terminator request)
            String result;
            result.resize(bufferSize);

            // 3. Fetch
            // Returns num chars written excluding null terminator
            DWORD actualSize = GetEnvironmentVariableA(name.c_str(), result.data(), bufferSize);

            if (actualSize == 0 || actualSize > bufferSize)
            {
                return std::nullopt;
            }

            // Resize down to exclude the null terminator and any slack
            result.resize(actualSize);
            return result;

#else
            // getenv returns a pointer to the environment area
            const char *val = std::getenv(name.c_str());
            if (val == nullptr)
            {
                return std::nullopt;
            }
            return String(val);
#endif
        }

        STATIC String Get(CONST String &name, CONST String &defaultValue = "")
        {
            return Find(name).value_or(defaultValue);
        }

        STATIC BOOL Set(CONST String &name, CONST String &value)
        {
            if (name.empty())
                return FALSE;

#if IA_PLATFORM_WINDOWS
            return SetEnvironmentVariableA(name.c_str(), value.c_str()) != 0;
#else
            // Returns 0 on success, -1 on error
            return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
        }

        STATIC BOOL Unset(CONST String &name)
        {
            if (name.empty())
                return FALSE;

#if IA_PLATFORM_WINDOWS
            return SetEnvironmentVariableA(name.c_str(), nullptr) != 0;
#else
            return unsetenv(name.c_str()) == 0;
#endif
        }

        STATIC BOOL Exists(CONST String &name)
        {
#if IA_PLATFORM_WINDOWS
            return GetEnvironmentVariableA(name.c_str(), nullptr, 0) > 0;
#else
            return std::getenv(name.c_str()) != nullptr;
#endif
        }
    };
} // namespace IACore