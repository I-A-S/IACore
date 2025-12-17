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

#include <IACore/IATest.hpp>

using namespace IACore;

// -----------------------------------------------------------------------------
// Platform Abstraction for Test Commands
// -----------------------------------------------------------------------------
#if IA_PLATFORM_WINDOWS
#    define CMD_ECHO_EXE "cmd.exe"
#    define CMD_ARG_PREFIX "/c echo"
#    define NULL_DEVICE "NUL"
#else
#    define CMD_ECHO_EXE "/bin/echo"
#    define CMD_ARG_PREFIX ""
#    define NULL_DEVICE "/dev/null"
#endif

IAT_BEGIN_BLOCK(Core, ProcessOps)

// -------------------------------------------------------------------------
// 1. Basic Execution (Exit Code 0)
// -------------------------------------------------------------------------
BOOL TestBasicRun()
{
    // Simple "echo hello"
    String captured;

    auto result = ProcessOps::SpawnProcessSync(CMD_ECHO_EXE, CMD_ARG_PREFIX " HelloIA",
                                               [&](StringView line) { captured = line; });

    IAT_CHECK(result.has_value());
    IAT_CHECK_EQ(*result, 0); // Exit code 0

    // We check if "HelloIA" is contained or equal.
    IAT_CHECK(captured.find("HelloIA") != String::npos);

    return TRUE;
}

// -------------------------------------------------------------------------
// 2. Argument Parsing
// -------------------------------------------------------------------------
BOOL TestArguments()
{
    Vector<String> lines;

    // Echo two distinct words.
    // Windows: cmd.exe /c echo one two
    // Linux: /bin/echo one two
    String args = String(CMD_ARG_PREFIX) + " one two";
    if (args[0] == ' ')
        args.erase(0, 1); // cleanup space if prefix empty

    auto result =
        ProcessOps::SpawnProcessSync(CMD_ECHO_EXE, args, [&](StringView line) { lines.push_back(String(line)); });

    IAT_CHECK_EQ(*result, 0);
    IAT_CHECK(lines.size() > 0);

    // Output should contain "one two"
    IAT_CHECK(lines[0].find("one two") != String::npos);

    return TRUE;
}

// -------------------------------------------------------------------------
// 3. Error / Non-Zero Exit Codes
// -------------------------------------------------------------------------
BOOL TestExitCodes()
{
    // We need a command that returns non-zero.
    // Windows: cmd /c exit 1
    // Linux: /bin/sh -c "exit 1"

    String cmd, arg;

#if IA_PLATFORM_WINDOWS
    cmd = "cmd.exe";
    arg = "/c exit 42";
#else
    cmd = "/bin/sh";
    arg = "-c \"exit 42\""; // quotes needed for sh -c
#endif

    auto result = ProcessOps::SpawnProcessSync(cmd, arg, [](StringView) {});

    IAT_CHECK(result.has_value());
    IAT_CHECK_EQ(*result, 42);

    return TRUE;
}

// -------------------------------------------------------------------------
// 4. Missing Executable Handling
// -------------------------------------------------------------------------
BOOL TestMissingExe()
{
    // Try to run a random string
    auto result = ProcessOps::SpawnProcessSync("sdflkjghsdflkjg", "", [](StringView) {});

    // Windows: CreateProcess usually fails -> returns unexpected
    // Linux: execvp fails inside child, returns 127 via waitpid

#if IA_PLATFORM_WINDOWS
    IAT_CHECK_NOT(result.has_value()); // Should be an error string
#else
    // Linux fork succeeds, but execvp fails, returning 127
    IAT_CHECK(result.has_value());
    IAT_CHECK_EQ(*result, 127);
#endif

    return TRUE;
}

// -------------------------------------------------------------------------
// 5. Line Buffer Logic (The 4096 split test)
// -------------------------------------------------------------------------
BOOL TestLargeOutput()
{
    // Need to generate output larger than the internal 4096 buffer
    // to ensure the "partial line" logic works when a line crosses a buffer boundary.

    String massiveString;
    massiveString.reserve(5000);
    for (int i = 0; i < 500; ++i)
        massiveString += "1234567890"; // 5000 chars

    String cmd, arg;

#if IA_PLATFORM_WINDOWS
    cmd = "cmd.exe";
    // Windows has command line length limits (~8k), 5k is safe.
    arg = "/c echo " + massiveString;
#else
    cmd = "/bin/echo";
    arg = massiveString;
#endif

    String captured;
    auto result = ProcessOps::SpawnProcessSync(cmd, arg, [&](StringView line) { captured += line; });

    IAT_CHECK(result.has_value());
    IAT_CHECK_EQ(*result, 0);

    // If the LineBuffer failed to stitch chunks, the length wouldn't match
    // or we would get multiple callbacks if we expected 1 line.
    IAT_CHECK_EQ(captured.length(), massiveString.length());

    return TRUE;
}

// -------------------------------------------------------------------------
// 6. Multi-Line Handling
// -------------------------------------------------------------------------
BOOL TestMultiLine()
{
    // Windows: cmd /c "echo A && echo B"
    // Linux: /bin/sh -c "echo A; echo B"

    String cmd, arg;
#if IA_PLATFORM_WINDOWS
    cmd = "cmd.exe";
    arg = "/c \"echo LineA && echo LineB\"";
#else
    cmd = "/bin/sh";
    arg = "-c \"echo LineA; echo LineB\"";
#endif

    int lineCount = 0;
    bool foundA = false;
    bool foundB = false;

    UNUSED(ProcessOps::SpawnProcessSync(cmd, arg, [&](StringView line) {
        lineCount++;
        if (line.find("LineA") != String::npos)
            foundA = true;
        if (line.find("LineB") != String::npos)
            foundB = true;
    }));

    IAT_CHECK(foundA);
    IAT_CHECK(foundB);
    // We expect at least 2 lines.
    // (Windows sometimes echoes the command itself depending on echo settings, but we check contents)
    IAT_CHECK(lineCount >= 2);

    return TRUE;
}

// -------------------------------------------------------------------------
// 6. Complex Command Line Arguments Handling
// -------------------------------------------------------------------------
BOOL TestComplexArguments()
{
    // Should parse as 3 arguments:
    // 1. -DDEFINED_MSG="Hello World"
    // 2. -v
    // 3. path/to/file
    String complexArgs = "-DDEFINED_MSG=\\\"Hello World\\\" -v path/to/file";

    String cmd = CMD_ECHO_EXE;

    String finalArgs;
#if IA_PLATFORM_WINDOWS
    finalArgs = "/c echo " + complexArgs;
#else
    finalArgs = complexArgs;
#endif

    String captured;
    auto result = ProcessOps::SpawnProcessSync(cmd, finalArgs, [&](StringView line) { captured += line; });

    IAT_CHECK(result.has_value());
    IAT_CHECK_EQ(*result, 0);

    // Verify the quotes were preserved in the output
    IAT_CHECK(captured.find("Hello World") != String::npos);
    return TRUE;
}

// -------------------------------------------------------------------------
// Registration
// -------------------------------------------------------------------------
IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(TestBasicRun);
IAT_ADD_TEST(TestArguments);
IAT_ADD_TEST(TestExitCodes);
IAT_ADD_TEST(TestMissingExe);
IAT_ADD_TEST(TestLargeOutput);
IAT_ADD_TEST(TestMultiLine);
IAT_ADD_TEST(TestComplexArguments);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, ProcessOps)
