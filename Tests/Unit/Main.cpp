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

#include <IACore/IACore.hpp>
#include <IACore/IATest.hpp>

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    printf(__CC_GREEN "\n===============================================================\n");
    printf("   IACore (Independent Architecture Core) - Unit Test Suite\n");
    printf("===============================================================\n" __CC_DEFAULT "\n");

    IACore::Initialize();

    int result = ia::iatest::TestRegistry::RunAll();

    IACore::Terminate();

    return result;
}