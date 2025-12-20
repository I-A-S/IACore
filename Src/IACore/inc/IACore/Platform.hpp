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

#if IA_ARCH_X64

#    ifdef _MSC_VER
#        include <intrin.h>
#    else
#        include <immintrin.h>
#    endif

#elif IA_ARCH_ARM64

#    include <arm_acle.h>

#endif

namespace IACore
{
    class Platform
    {
      public:
        struct Capabilities
        {
            BOOL HardwareCRC32{FALSE};
        };

      public:
        STATIC BOOL CheckCPU();

#if IA_ARCH_X64
        STATIC VOID CPUID(IN INT32 function, IN INT32 subFunction, OUT INT32 out[4]);
#endif

        STATIC PCCHAR GetArchitectureName();
        STATIC PCCHAR GetOperatingSystemName();

      public:
        STATIC CONST Capabilities &GetCapabilities()
        {
            return s_capabilities;
        }

      private:
        STATIC Capabilities s_capabilities;
    };
} // namespace IACore