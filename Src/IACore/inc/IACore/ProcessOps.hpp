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

#if IA_PLATFORM_WINDOWS
using NativeProcessID = DWORD;
#elif IA_PLATFORM_UNIX
using NativeProcessID = pid_t;
#else
#    error "This platform does not support IACore ProcessOps"
#endif

namespace IACore
{
    struct ProcessHandle
    {
        Atomic<NativeProcessID> ID{0};
        Atomic<BOOL> IsRunning{false};

        BOOL IsActive() CONST
        {
            return IsRunning && ID != 0;
        }

      private:
        JoiningThread ThreadHandle;

        friend class ProcessOps;
    };

    class ProcessOps
    {
      public:
        STATIC NativeProcessID GetCurrentProcessID();

        STATIC Expected<INT32, String> SpawnProcessSync(IN CONST String &command, IN CONST String &args,
                                                        IN Function<VOID(IN StringView line)> onOutputLineCallback);
        STATIC SharedPtr<ProcessHandle> SpawnProcessAsync(IN CONST String &command, IN CONST String &args,
                                                          IN Function<VOID(IN StringView line)> onOutputLineCallback,
                                                          IN Function<VOID(Expected<INT32, String>)> onFinishCallback);

        STATIC VOID TerminateProcess(IN CONST SharedPtr<ProcessHandle> &handle);

      private:
        STATIC Expected<INT32, String> SpawnProcessWindows(IN CONST String &command, IN CONST String &args,
                                                           IN Function<VOID(StringView)> onOutputLineCallback,
                                                           OUT Atomic<NativeProcessID> &id);
        STATIC Expected<INT32, String> SpawnProcessPosix(IN CONST String &command, IN CONST String &args,
                                                         IN Function<VOID(StringView)> onOutputLineCallback,
                                                         OUT Atomic<NativeProcessID> &id);
    };
} // namespace IACore