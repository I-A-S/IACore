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

#include <IACore/ProcessOps.hpp>

namespace IACore
{
    // ---------------------------------------------------------------------
    // Output Buffering Helper
    // Splits raw chunks into lines, preserving partial lines across chunks
    // ---------------------------------------------------------------------
    struct LineBuffer
    {
        String Accumulator;
        Function<VOID(StringView)> &Callback;

        VOID Append(IN PCCHAR data, IN SIZE_T size);
        VOID Flush();
    };
} // namespace IACore

namespace IACore
{
    NativeProcessID ProcessOps::GetCurrentProcessID()
    {
#if IA_PLATFORM_WINDOWS
        return ::GetCurrentProcessId();
#else
        return getpid();
#endif
    }

    Expected<INT32, String> ProcessOps::SpawnProcessSync(IN CONST String &command, IN CONST String &args,
                                                         IN Function<VOID(IN StringView line)> onOutputLineCallback)
    {
        Atomic<NativeProcessID> id;
#if IA_PLATFORM_WINDOWS
        return SpawnProcessWindows(command, args, onOutputLineCallback, id);
#else
        return SpawnProcessPosix(command, args, onOutputLineCallback, id);
#endif
    }

    SharedPtr<ProcessHandle> ProcessOps::SpawnProcessAsync(IN CONST String &command, IN CONST String &args,
                                                           IN Function<VOID(IN StringView line)> onOutputLineCallback,
                                                           IN Function<VOID(Expected<INT32, String>)> onFinishCallback)
    {
        SharedPtr<ProcessHandle> handle = std::make_shared<ProcessHandle>();
        handle->IsRunning = true;

        handle->ThreadHandle =
            JoiningThread([=, h = handle.get(), cmd = IA_MOVE(command), args = std::move(args)]() mutable {

#if IA_PLATFORM_WINDOWS
                auto result = SpawnProcessWindows(cmd, args, onOutputLineCallback, h->ID);
#else
                auto result = SpawnProcessPosix(cmd, args, onOutputLineCallback, h->ID);
#endif

                h->IsRunning = false;

                if (!onFinishCallback)
                    return;

                if (!result)
                    onFinishCallback(MakeUnexpected(result.error()));
                else
                    onFinishCallback(*result);
            });

        return handle;
    }

    VOID ProcessOps::TerminateProcess(IN CONST SharedPtr<ProcessHandle> &handle)
    {
        if (!handle || !handle->IsActive())
            return;

        NativeProcessID pid = handle->ID.load();
        if (pid == 0)
            return;

#if IA_PLATFORM_WINDOWS
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess != NULL)
        {
            ::TerminateProcess(hProcess, 9);
            CloseHandle(hProcess);
        }
#else
        kill(pid, SIGKILL);
#endif
    }
} // namespace IACore

namespace IACore
{
#if IA_PLATFORM_WINDOWS
    Expected<INT32, String> ProcessOps::SpawnProcessWindows(IN CONST String &command, IN CONST String &args,
                                                            IN Function<VOID(StringView)> onOutputLineCallback,
                                                            OUT Atomic<NativeProcessID> &id)
    {
        SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE}; // Allow inheritance
        HANDLE hRead = NULL, hWrite = NULL;

        if (!CreatePipe(&hRead, &hWrite, &saAttr, 0))
            return tl::make_unexpected("Failed to create pipe");

        // Ensure the read handle to the pipe for STDOUT is NOT inherited
        if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0))
            return tl::make_unexpected("Failed to secure pipe handles");

        STARTUPINFOA si = {sizeof(STARTUPINFOA)};
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = hWrite;
        si.hStdError = hWrite; // Merge stderr
        si.hStdInput = NULL;   // No input

        PROCESS_INFORMATION pi = {0};

        // Windows command line needs to be mutable and concatenated
        String commandLine = std::format("\"{}\" {}", command, args);

        BOOL success = CreateProcessA(NULL, commandLine.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

        // Close write end in parent, otherwise ReadFile never returns EOF!
        CloseHandle(hWrite);

        if (!success)
        {
            CloseHandle(hRead);
            return tl::make_unexpected(String("CreateProcess failed: ") + std::to_string(GetLastError()));
        }

        id.store(pi.dwProcessId);

        // Read Loop
        LineBuffer lineBuf{"", onOutputLineCallback};
        DWORD bytesRead;
        CHAR buffer[4096];

        while (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead != 0)
        {
            lineBuf.Append(buffer, bytesRead);
        }
        lineBuf.Flush();

        // NOW we wait for exit code
        DWORD exitCode = 0;
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hRead);
        id.store(0);

        return static_cast<INT32>(exitCode);
    }
#endif

#if IA_PLATFORM_UNIX
    Expected<INT32, String> ProcessOps::SpawnProcessPosix(IN CONST String &command, IN CONST String &args,
                                                          IN Function<VOID(StringView)> onOutputLineCallback,
                                                          OUT Atomic<NativeProcessID> &id)
    {
        int pipefd[2];
        if (pipe(pipefd) == -1)
            return tl::make_unexpected("Failed to create pipe");

        pid_t pid = fork();

        if (pid == -1)
        {
            return tl::make_unexpected("Failed to fork process");
        }
        else if (pid == 0)
        {
            // --- Child Process ---
            close(pipefd[0]);

            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);

            std::vector<std::string> argStorage; // To keep strings alive
            std::vector<char *> argv;

            std::string cmdStr = command;
            argv.push_back(cmdStr.data());

            // Manual Quote-Aware Splitter
            std::string currentToken;
            bool inQuotes = false;
            bool isEscaped = false;

            for (char c : args)
            {
                if (isEscaped)
                {
                    // Previous char was '\', so we treat this char literally.
                    currentToken += c;
                    isEscaped = false;
                    continue;
                }

                if (c == '\\')
                {
                    // Escape sequence start
                    isEscaped = true;
                    continue;
                }

                if (c == '\"')
                {
                    // Toggle quote state
                    inQuotes = !inQuotes;
                    continue;
                }

                if (c == ' ' && !inQuotes)
                {
                    // Token boundary
                    if (!currentToken.empty())
                    {
                        argStorage.push_back(currentToken);
                        currentToken.clear();
                    }
                }
                else
                {
                    currentToken += c;
                }
            }

            if (!currentToken.empty())
            {
                argStorage.push_back(currentToken);
            }

            // Build char* array from the std::string storage
            for (auto &s : argStorage)
            {
                argv.push_back(s.data());
            }
            argv.push_back(nullptr);

            execvp(argv[0], argv.data());
            _exit(127);
        }
        else
        {
            // --- Parent Process ---
            id.store(pid);

            close(pipefd[1]);

            LineBuffer lineBuf{"", onOutputLineCallback};
            char buffer[4096];
            ssize_t count;

            while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0)
            {
                lineBuf.Append(buffer, count);
            }
            lineBuf.Flush();
            close(pipefd[0]);

            int status;
            waitpid(pid, &status, 0);

            id.store(0);
            if (WIFEXITED(status))
                return WEXITSTATUS(status);
            return -1;
        }
    }
#endif
} // namespace IACore

namespace IACore
{
    VOID LineBuffer::Append(IN PCCHAR data, IN SIZE_T size)
    {
        SIZE_T start = 0;
        for (SIZE_T i = 0; i < size; ++i)
        {
            if (data[i] == '\n' || data[i] == '\r')
            {
                // Flush Accumulator + current chunk
                if (!Accumulator.empty())
                {
                    Accumulator.append(data + start, i - start);
                    if (!Accumulator.empty())
                        Callback(Accumulator);
                    Accumulator.clear();
                }
                else
                {
                    if (i > start)
                        Callback(StringView(data + start, i - start));
                }

                // Skip \r\n sequence if needed, or just start next
                if (data[i] == '\r' && i + 1 < size && data[i + 1] == '\n')
                    i++;
                start = i + 1;
            }
        }
        // Save remaining partial line
        if (start < size)
        {
            Accumulator.append(data + start, size - start);
        }
    }

    VOID LineBuffer::Flush()
    {
        if (!Accumulator.empty())
        {
            Callback(Accumulator);
            Accumulator.clear();
        }
    }
} // namespace IACore