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
#include <cstdlib>

namespace IACore
{

  class Environment
  {
public:
    static auto find(Ref<String> name) -> Option<String>
    {
#if IA_PLATFORM_WINDOWS
      const u32 buffer_size = static_cast<u32>(GetEnvironmentVariableA(name.c_str(), nullptr, 0));

      if (buffer_size == 0)
      {
        return std::nullopt;
      }

      Mut<String> result;
      result.resize(buffer_size);

      const u32 actual_size = static_cast<u32>(GetEnvironmentVariableA(name.c_str(), result.data(), buffer_size));

      if (actual_size == 0 || actual_size > buffer_size)
      {
        return std::nullopt;
      }

      result.resize(actual_size);
      return result;

#else
      const char *val = std::getenv(name.c_str());
      if (val == nullptr)
      {
        return std::nullopt;
      }
      return String(val);
#endif
    }

    static auto get(Ref<String> name, Ref<String> default_value = "") -> String
    {
      return find(name).value_or(default_value);
    }

    static auto set(Ref<String> name, Ref<String> value) -> Result<void>
    {
      if (name.empty())
      {
        return fail("Environment variable name cannot be empty");
      }

#if IA_PLATFORM_WINDOWS
      if (SetEnvironmentVariableA(name.c_str(), value.c_str()) == 0)
      {
        return fail("Failed to set environment variable: {}", name);
      }
#else
      if (setenv(name.c_str(), value.c_str(), 1) != 0)
      {
        return fail("Failed to set environment variable: {}", name);
      }
#endif
      return {};
    }

    static auto unset(Ref<String> name) -> Result<void>
    {
      if (name.empty())
      {
        return fail("Environment variable name cannot be empty");
      }

#if IA_PLATFORM_WINDOWS
      if (SetEnvironmentVariableA(name.c_str(), nullptr) == 0)
      {
        return fail("Failed to unset environment variable: {}", name);
      }
#else
      if (unsetenv(name.c_str()) != 0)
      {
        return fail("Failed to unset environment variable: {}", name);
      }
#endif
      return {};
    }

    static auto exists(Ref<String> name) -> bool
    {
      return find(name).has_value();
    }
  };

} // namespace IACore