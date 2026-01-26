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

#include <IACore/Logger.hpp>
#include <IACore/PCH.hpp>

#define IACORE_MAIN()                                                                                                  \
  auto _app_entry(IACore::Ref<IACore::Vec<IACore::String>> args) -> IACore::Result<IACore::i32>;                       \
  auto main(int argc, Mut<char *> argv[]) -> int                                                                       \
  {                                                                                                                    \
    IACore::Mut<IACore::i32> exit_code = 0;                                                                            \
    IACore::initialize();                                                                                              \
    IACore::Mut<IACore::Vec<IACore::String>> args;                                                                     \
    args.reserve(static_cast<IACore::usize>(argc));                                                                    \
    for (IACore::Mut<IACore::i32> i = 0; i < argc; ++i)                                                                \
    {                                                                                                                  \
      args.push_back(argv[i]);                                                                                         \
    }                                                                                                                  \
    IACore::Result<IACore::i32> result = _app_entry(args);                                                             \
    if (!result)                                                                                                       \
    {                                                                                                                  \
      IACore::Logger::error("Application exited with an error: '{}'.", result.error());                                \
      exit_code = -20;                                                                                                 \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      exit_code = *result;                                                                                             \
      if (exit_code == 0)                                                                                              \
      {                                                                                                                \
        IACore::Logger::info("Application exited successfully.");                                                      \
      }                                                                                                                \
      else                                                                                                             \
      {                                                                                                                \
        IACore::Logger::error("Application exited with error code: {}.", exit_code);                                   \
      }                                                                                                                \
    }                                                                                                                  \
    IACore::terminate();                                                                                               \
    return exit_code;                                                                                                  \
  }                                                                                                                    \
  auto _app_entry(IACore::Ref<IACore::Vec<IACore::String>> args) -> IACore::Result<IACore::i32>

namespace IACore
{
  // Must be called from main thread
  // Safe to call multiple times but, given every initialize call is paired with a
  // corresponding terminate call
  auto initialize() -> void;

  // Must be called from same thread as initialize
  // Safe to call multiple times but, given every initialize call is paired with a
  // corresponding terminate call
  auto terminate() -> void;

  auto is_initialized() -> bool;

  auto is_main_thread() -> bool;
} // namespace IACore