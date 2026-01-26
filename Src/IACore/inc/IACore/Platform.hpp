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

#if IA_ARCH_X64
#  ifdef _MSC_VER
#    include <intrin.h>
#  else
#    include <immintrin.h>
#  endif
#elif IA_ARCH_ARM64
#  include <arm_acle.h>
#endif

namespace IACore
{
  class Platform
  {
public:
    struct Capabilities
    {
      Mut<bool> hardware_crc32 = false;
    };

    static auto check_cpu() -> bool;

#if IA_ARCH_X64
    static auto cpuid(const i32 function, const i32 sub_function, Mut<i32 *> out) -> void;
#endif

    static auto get_architecture_name() -> const char *;
    static auto get_operating_system_name() -> const char *;

    static auto get_capabilities() -> Ref<Capabilities>
    {
      return s_capabilities;
    }

private:
    static Mut<Capabilities> s_capabilities;
  };
} // namespace IACore