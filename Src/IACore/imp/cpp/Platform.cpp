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

#include <IACore/Platform.hpp>

#if IA_ARCH_X64

#    ifndef _MSC_VER
#        include <cpuid.h>
#    endif

#elif IA_ARCH_ARM64

#    if defined(__linux__) || defined(__ANDROID__)
#        include <sys/auxv.h>
#        include <asm/hwcap.h>
#    endif

#endif

namespace IACore
{
    Platform::Capabilities Platform::s_capabilities{};

#if IA_ARCH_X64
    VOID Platform::CPUID(IN INT32 function, IN INT32 subFunction, OUT INT32 out[4])
    {
#    ifdef _MSC_VER
        __cpuidex(out, function, subFunction);
#    else
        __cpuid_count(function, subFunction, out[0], out[1], out[2], out[3]);
#    endif
    }
#endif

    BOOL Platform::CheckCPU()
    {
#if IA_ARCH_X64
        INT32 cpuInfo[4];

        CPUID(0, 0, cpuInfo);
        if (cpuInfo[0] < 7)
            return false;

        CPUID(1, 0, cpuInfo);
        BOOL osxsave = (cpuInfo[2] & (1 << 27)) != 0;
        BOOL avx = (cpuInfo[2] & (1 << 28)) != 0;
        BOOL fma = (cpuInfo[2] & (1 << 12)) != 0;
        if (!osxsave || !avx || !fma)
            return false;

        UINT64 xcrFeatureMask = _xgetbv(0);
        if ((xcrFeatureMask & 0x6) != 0x6)
            return false;

        CPUID(7, 0, cpuInfo);
        BOOL avx2 = (cpuInfo[1] & (1 << 5)) != 0;
        if (!avx2)
            return false;

        s_capabilities.HardwareCRC32 = TRUE;
#elif IA_ARCH_ARM64
#    if defined(__linux__) || defined(__ANDROID__)
        unsigned long hw_caps = getauxval(AT_HWCAP);

#        ifndef HWCAP_CRC32
#            define HWCAP_CRC32 (1 << 7)
#        endif

        s_capabilities.HardwareCRC32 = hw_caps & HWCAP_CRC32;
#    elif defined(__APPLE__)
        // Apple silicon always has hardware CRC32
        s_capabilities.HardwareCRC32 = TRUE;
#    else
        s_capabilities.HardwareCRC32 = FALSE;
#    endif
#else
        s_capabilities.HardwareCRC32 = FALSE;
#endif
        return true;
    }

    PCCHAR Platform::GetArchitectureName()
    {
#if IA_ARCH_X64
        return "x86_64";
#elif IA_ARCH_ARM64
        return "aarch64";
#elif IA_ARCH_WASM
        return "wasm";
#endif
    }

    PCCHAR Platform::GetOperatingSystemName()
    {
#if IA_PLATFORM_WINDOWS
        return "Windows";
#elif IA_PLATFORM_IOS
        return "iOS";
#elif IA_PLATFORM_MAC
        return "Mac";
#elif IA_PLATFORM_ANDROID
        return "Android";
#elif IA_PLATFORM_LINUX
        return "Linux";
#endif
    }
} // namespace IACore