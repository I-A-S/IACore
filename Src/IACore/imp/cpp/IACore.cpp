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
#include <IACore/Logger.hpp>

#include <chrono>
#include <mimalloc.h>

namespace IACore
{
  Mut<std::chrono::high_resolution_clock::time_point> g_start_time = {};

  static Mut<std::thread::id> g_main_thread_id = {};
  static Mut<i32> g_core_init_count = 0;

  auto initialize() -> void
  {
    g_core_init_count++;
    if (g_core_init_count > 1)
    {
      return;
    }

    g_main_thread_id = std::this_thread::get_id();
    g_start_time = std::chrono::high_resolution_clock::now();

    Logger::initialize();

    mi_option_set(mi_option_verbose, 0);
  }

  auto terminate() -> void
  {
    g_core_init_count--;
    if (g_core_init_count > 0)
    {
      return;
    }

    Logger::terminate();
  }

  auto is_initialized() -> bool
  {
    return g_core_init_count > 0;
  }

  auto is_main_thread() -> bool
  {
    return std::this_thread::get_id() == g_main_thread_id;
  }
} // namespace IACore