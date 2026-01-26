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

#include <IACore/ProcessOps.hpp>

namespace IACore
{
  struct LineBuffer
  {
    Mut<String> m_accumulator;
    const std::function<void(StringView)> m_callback;

    auto append(const char *data, const usize size) -> void;
    auto flush() -> void;
  };

  auto LineBuffer::append(const char *data, const usize size) -> void
  {
    Mut<usize> start = 0;
    for (Mut<usize> i = 0; i < size; ++i)
    {
      if (data[i] == '\n' || data[i] == '\r')
      {
        if (!m_accumulator.empty())
        {
          m_accumulator.append(data + start, i - start);
          if (!m_accumulator.empty())
          {
            m_callback(m_accumulator);
          }
          m_accumulator.clear();
        }
        else
        {
          if (i > start)
          {
            m_callback(StringView(data + start, i - start));
          }
        }

        if (data[i] == '\r' && i + 1 < size && data[i + 1] == '\n')
        {
          i++;
        }
        start = i + 1;
      }
    }
    if (start < size)
    {
      m_accumulator.append(data + start, size - start);
    }
  }

  auto LineBuffer::flush() -> void
  {
    if (!m_accumulator.empty())
    {
      m_callback(m_accumulator);
      m_accumulator.clear();
    }
  }

  auto ProcessOps::get_current_process_id() -> NativeProcessID
  {
#if IA_PLATFORM_WINDOWS
    return ::GetCurrentProcessId();
#else
    return getpid();
#endif
  }

  auto ProcessOps::spawn_process_sync(Ref<String> command, Ref<String> args,
                                      const std::function<void(StringView)> on_output_line_callback) -> Result<i32>
  {
    Mut<std::atomic<NativeProcessID>> id = 0;
    if constexpr (Env::IS_WINDOWS)
    {
      return spawn_process_windows(command, args, on_output_line_callback, id);
    }
    else
    {
      return spawn_process_posix(command, args, on_output_line_callback, id);
    }
  }

  auto ProcessOps::spawn_process_async(Ref<String> command, Ref<String> args,
                                       const std::function<void(StringView)> on_output_line_callback,
                                       const std::function<void(Result<i32>)> on_finish_callback)
      -> Result<Box<ProcessHandle>>
  {
    Mut<Box<ProcessHandle>> handle = make_box<ProcessHandle>();
    handle->is_running = true;

    Mut<ProcessHandle *> h_ptr = handle.get();

    handle->m_thread_handle = std::jthread(
        [h_ptr, cmd = command, arg = args, cb = on_output_line_callback, fin = on_finish_callback]() mutable {
          Mut<Result<i32>> result = fail("Platform not supported");

          if constexpr (Env::IS_WINDOWS)
          {
            result = spawn_process_windows(cmd, arg, cb, h_ptr->id);
          }
          else
          {
            result = spawn_process_posix(cmd, arg, cb, h_ptr->id);
          }

          h_ptr->is_running = false;

          if (fin)
          {
            if (!result)
            {
              fin(fail(std::move(result.error())));
            }
            else
            {
              fin(*result);
            }
          }
        });

    return handle;
  }

  auto ProcessOps::terminate_process(Ref<Box<ProcessHandle>> handle) -> void
  {
    if (!handle || !handle->is_active())
    {
      return;
    }

    const NativeProcessID pid = handle->id.load();
    if (pid == 0)
    {
      return;
    }

#if IA_PLATFORM_WINDOWS
    Mut<HANDLE> h_process = OpenProcess(PROCESS_TERMINATE, false, pid);
    if (h_process != NULL)
    {
      ::TerminateProcess(h_process, 9);
      CloseHandle(h_process);
    }
#endif
#if IA_PLATFORM_UNIX
    kill(pid, SIGKILL);
#endif
  }

  auto ProcessOps::spawn_process_windows(Ref<String> command, Ref<String> args,
                                         const std::function<void(StringView)> on_output_line_callback,
                                         MutRef<std::atomic<NativeProcessID>> id) -> Result<i32>
  {
#if IA_PLATFORM_WINDOWS
    Mut<SECURITY_ATTRIBUTES> sa_attr = {sizeof(SECURITY_ATTRIBUTES), NULL, true};
    Mut<HANDLE> h_read = NULL;
    Mut<HANDLE> h_write = NULL;

    if (!CreatePipe(&h_read, &h_write, &sa_attr, 0))
    {
      return fail("Failed to create pipe");
    }

    if (!SetHandleInformation(h_read, HANDLE_FLAG_INHERIT, 0))
    {
      return fail("Failed to secure pipe handles");
    }

    Mut<STARTUPINFOA> si = {sizeof(STARTUPINFOA)};
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = h_write;
    si.hStdError = h_write;
    si.hStdInput = NULL;

    Mut<PROCESS_INFORMATION> pi = {0};

    Mut<String> command_line = std::format("\"{}\" {}", command, args);

    const BOOL success = CreateProcessA(NULL, command_line.data(), NULL, NULL, true, 0, NULL, NULL, &si, &pi);

    CloseHandle(h_write);

    if (!success)
    {
      CloseHandle(h_read);
      return fail("CreateProcess failed: {}", GetLastError());
    }

    id.store(pi.dwProcessId);

    Mut<LineBuffer> line_buf{"", on_output_line_callback};
    Mut<DWORD> bytes_read = 0;
    Mut<Array<char, 4096>> buffer;

    while (ReadFile(h_read, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, NULL) && bytes_read != 0)
    {
      line_buf.append(buffer.data(), bytes_read);
    }
    line_buf.flush();

    Mut<DWORD> exit_code = 0;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(h_read);
    id.store(0);

    return static_cast<i32>(exit_code);
#else
    AU_UNUSED(command);
    AU_UNUSED(args);
    AU_UNUSED(on_output_line_callback);
    AU_UNUSED(id);
    return fail("Windows implementation not available.");
#endif
  }

  auto ProcessOps::spawn_process_posix(Ref<String> command, Ref<String> args,
                                       const std::function<void(StringView)> on_output_line_callback,
                                       MutRef<std::atomic<NativeProcessID>> id) -> Result<i32>
  {
#if IA_PLATFORM_UNIX
    Mut<Array<i32, 2>> pipefd;
    if (pipe(pipefd.data()) == -1)
    {
      return fail("Failed to create pipe");
    }

    const pid_t pid = fork();

    if (pid == -1)
    {
      return fail("Failed to fork process");
    }
    else if (pid == 0)
    {
      close(pipefd[0]);

      dup2(pipefd[1], STDOUT_FILENO);
      dup2(pipefd[1], STDERR_FILENO);
      close(pipefd[1]);

      Mut<Vec<String>> arg_storage;
      Mut<Vec<char *>> argv;

      Mut<String> cmd_str = command;
      argv.push_back(cmd_str.data());

      Mut<String> current_token;
      Mut<bool> in_quotes = false;
      Mut<bool> is_escaped = false;

      for (const char c : args)
      {
        if (is_escaped)
        {
          current_token += c;
          is_escaped = false;
          continue;
        }

        if (c == '\\')
        {
          is_escaped = true;
          continue;
        }

        if (c == '\"')
        {
          in_quotes = !in_quotes;
          continue;
        }

        if (c == ' ' && !in_quotes)
        {
          if (!current_token.empty())
          {
            arg_storage.push_back(current_token);
            current_token.clear();
          }
        }
        else
        {
          current_token += c;
        }
      }

      if (!current_token.empty())
      {
        arg_storage.push_back(current_token);
      }

      for (MutRef<String> s : arg_storage)
      {
        argv.push_back(s.data());
      }
      argv.push_back(nullptr);

      execvp(argv[0], argv.data());
      _exit(127);
    }
    else
    {
      id.store(pid);

      close(pipefd[1]);

      Mut<LineBuffer> line_buf{"", on_output_line_callback};
      Mut<Array<char, 4096>> buffer;
      Mut<isize> count;

      while ((count = read(pipefd[0], buffer.data(), buffer.size())) > 0)
      {
        line_buf.append(buffer.data(), static_cast<usize>(count));
      }
      line_buf.flush();
      close(pipefd[0]);

      Mut<i32> status;
      waitpid(pid, &status, 0);

      id.store(0);
      if (WIFEXITED(status))
      {
        return WEXITSTATUS(status);
      }
      return -1;
    }
#else
    AU_UNUSED(command);
    AU_UNUSED(args);
    AU_UNUSED(on_output_line_callback);
    AU_UNUSED(id);
    return fail("Posix implementation not available.");
#endif
  }

} // namespace IACore