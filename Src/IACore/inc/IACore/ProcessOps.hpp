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

#if IA_PLATFORM_WINDOWS
using NativeProcessID = DWORD;
#elif IA_PLATFORM_UNIX
using NativeProcessID = pid_t;
#else
#  error "This platform does not support IACore ProcessOps"
#endif

namespace IACore
{
  struct ProcessHandle
  {
    Mut<std::atomic<NativeProcessID>> id{0};
    Mut<std::atomic<bool>> is_running{false};

    [[nodiscard]] auto is_active() const -> bool
    {
      return is_running && id != 0;
    }

private:
    Mut<std::jthread> m_thread_handle;

    friend class ProcessOps;
  };

  class ProcessOps
  {
public:
    static auto get_current_process_id() -> NativeProcessID;

    static auto spawn_process_sync(Ref<String> command, Ref<String> args,
                                   const std::function<void(StringView)> on_output_line_callback) -> Result<i32>;

    static auto spawn_process_async(Ref<String> command, Ref<String> args,
                                    const std::function<void(StringView)> on_output_line_callback,
                                    const std::function<void(Result<i32>)> on_finish_callback)
        -> Result<Box<ProcessHandle>>;

    static auto terminate_process(Ref<Box<ProcessHandle>> handle) -> void;

private:
    static auto spawn_process_windows(Ref<String> command, Ref<String> args,
                                      const std::function<void(StringView)> on_output_line_callback,
                                      MutRef<std::atomic<NativeProcessID>> id) -> Result<i32>;

    static auto spawn_process_posix(Ref<String> command, Ref<String> args,
                                    const std::function<void(StringView)> on_output_line_callback,
                                    MutRef<std::atomic<NativeProcessID>> id) -> Result<i32>;
  };
} // namespace IACore