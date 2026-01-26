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

#include <IACore/IACore.hpp>
#include <IACore/IATest.hpp>

#include <IACore/SocketOps.hpp>

using namespace IACore;

IACORE_MAIN()
{
  (void) args;

  AU_TRY_PURE(SocketOps::initialize());

  std::cout << console::GREEN << "\n===============================================================\n";
  std::cout << "   IACore (Independent Architecture Core) - Unit Test Suite\n";
  std::cout << "===============================================================\n" << console::RESET << "\n";

  const i32 result = Test::TestRegistry::run_all();

  SocketOps::terminate();

  return result;
}