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

#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
#  define IA_ARCH_X64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#  define IA_ARCH_ARM64 1
#elif defined(__wasm__) || defined(__wasm32__) || defined(__wasm64__)
#  define IA_ARCH_WASM 1
#else
#  error "IACore: Unsupported Architecture."
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define IA_PLATFORM_WINDOWS 1
#elif __APPLE__
#  include <TargetConditionals.h>
#  define IA_PLATFORM_APPLE 1
#  define IA_PLATFORM_UNIX 1
#elif __linux__
#  define IA_PLATFORM_LINUX 1
#  define IA_PLATFORM_UNIX 1
#elif __wasm__
#  define IA_PLATFORM_WASM 1
#else
#  error "IACore: Unsupported Platform."
#endif

#if IA_PLATFORM_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#elif IA_PLATFORM_UNIX
#  include <signal.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>

#include <ankerl/unordered_dense.h>

#include <auxid/auxid.hpp>

namespace IACore
{

  using namespace Auxid;

  // =============================================================================
  // Build Environment & Constants
  // =============================================================================
  namespace Env
  {

    using namespace Auxid::Env;

#if IA_PLATFORM_WINDOWS
    constexpr const bool IS_WINDOWS = true;
    constexpr const bool IS_UNIX = false;
#else
    constexpr const bool IS_WINDOWS = false;
    constexpr const bool IS_UNIX = true;
#endif

    constexpr const usize MAX_PATH_LEN = 4096;

  } // namespace Env

  // =============================================================================
  // Data Structures & Aliases
  // =============================================================================
  template<typename K, typename V> using HashMap = ankerl::unordered_dense::map<K, V>;
  template<typename T> using HashSet = ankerl::unordered_dense::set<T>;

  using Path = std::filesystem::path;

  // =============================================================================
  // Versioning
  // =============================================================================
  struct Version
  {
    u32 major = 0;
    u32 minor = 0;
    u32 patch = 0;

    [[nodiscard]] constexpr auto to_u64() const -> u64
    {
      return (static_cast<u64>(major) << 40) | (static_cast<u64>(minor) << 16) | (static_cast<u64>(patch));
    }
  };

  // =============================================================================
  // Console Colors
  // =============================================================================
  namespace console
  {
    constexpr const char *RESET = "\033[0m";
    constexpr const char *RED = "\033[31m";
    constexpr const char *GREEN = "\033[32m";
    constexpr const char *YELLOW = "\033[33m";
    constexpr const char *BLUE = "\033[34m";
    constexpr const char *MAGENTA = "\033[35m";
    constexpr const char *CYAN = "\033[36m";
  } // namespace console

} // namespace IACore

#define IA_NODISCARD [[nodiscard]]
#define IA_UNUSED(v) (void) (v)
