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

// -------------------------------------------------------------------------
// Platform Detection
// -------------------------------------------------------------------------
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#    ifdef _WIN64
#        define IA_PLATFORM_WIN64 1
#        define IA_PLATFORM_WINDOWS 1
#    else
#        define IA_PLATFORM_WIN32 1
#        define IA_PLATFORM_WINDOWS 1
#    endif
#elif __APPLE__
#    include <TargetConditionals.h>
#    if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR || TARGET_OS_MACCATALYST
#        define IA_PLATFORM_IOS 1
#    elif TARGET_OS_MAC
#        define IA_PLATFORM_MAC 1
#    endif
#    define IA_PLATFORM_UNIX 1
#elif __ANDROID__
#    define IA_PLATFORM_ANDROID 1
#    define IA_PLATFORM_LINUX 1
#    define IA_PLATFORM_UNIX 1
#elif __linux__
#    define IA_PLATFORM_LINUX 1
#    define IA_PLATFORM_UNIX 1
#elif __unix__
#    define IA_PLATFORM_UNIX 1
#endif

#if IA_PLATFORM_WIN32 || IA_PLATFORM_WIN64
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#    undef VOID
#    undef ERROR
#elif IA_PLATFORM_UNIX
#    include <unistd.h>
#    include <sys/wait.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <fcntl.h>
#    include <spawn.h>
#    include <signal.h>
#endif

// -----------------------------------------------------------------------------
// Configuration Macros
// -----------------------------------------------------------------------------

#define IA_CHECK(o) (o > 0)

#if defined(__IA_DEBUG) && __IA_DEBUG
#    define __DEBUG_MODE__
#    define __BUILD_MODE_NAME "debug"
#    define DEBUG_ONLY(v) v
#    ifndef _DEBUG
#        define _DEBUG
#    endif
#else
#    define __RELEASE_MODE__
#    define __BUILD_MODE_NAME "release"
#    ifndef NDEBUG
#        define NDEBUG
#    endif
#    ifndef __OPTIMIZE__
#        define __OPTIMIZE__
#    endif
#    define DEBUG_ONLY(f)
#endif

#if IA_CHECK(IA_PLATFORM_WIN64) || IA_CHECK(IA_PLATFORM_UNIX)
#    define IA_CORE_PLATFORM_FEATURES 1
#else
#    define IA_CORE_PLATFORM_FEATURES 0
#    warning "IACore Unsupported Platform: Platform specific features will be disabled"
#endif

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#ifdef __cplusplus
#    include <bit>
#    include <new>
#    include <span>
#    include <atomic>
#    include <mutex>
#    include <thread>
#    include <limits>
#    include <cstring>
#    include <cstddef>
#    include <chrono>
#    include <iomanip>
#    include <fstream>
#    include <iostream>
#    include <concepts>
#    include <filesystem>
#    include <functional>
#    include <type_traits>
#    include <initializer_list>
#    include <condition_variable>

#    include <tuple>
#    include <array>
#    include <deque>
#    include <string>
#    include <vector>
#    include <format>
#    include <sstream>
#    include <optional>
#    include <string_view>

#    include <tl/expected.hpp>
#    include <ankerl/unordered_dense.h>

#else
#    include <float.h>
#    include <stdbool.h>
#    include <stddef.h>
#    include <string.h>
#endif

// -----------------------------------------------------------------------------
// Security Macros
// -----------------------------------------------------------------------------

#define IA_PANIC(msg)                                                                                                  \
    {                                                                                                                  \
        fprintf(stderr, "PANIC: %s\n", msg);                                                                           \
        __builtin_trap();                                                                                              \
    }

// Advanced Security features are not included in OSS builds
// (OSS version does not implement 'IAC_CHECK_*'s)
#define IACORE_SECURITY_LEVEL 0

#define IAC_SEC_LEVEL(v) (IACORE_SECURITY_LEVEL >= v)

#if __IA_DEBUG || IAC_SEC_LEVEL(1)
#    define __IAC_OVERFLOW_CHECKS 1
#else
#    define __IAC_OVERFLOW_CHECKS 0
#endif
#if __IA_DEBUG || IAC_SEC_LEVEL(2)
#    define __IAC_SANITY_CHECKS 1
#else
#    define __IAC_SANITY_CHECKS 0
#endif

// -----------------------------------------------------------------------------
// Language Abstraction Macros
// -----------------------------------------------------------------------------

#define AUTO auto
#define CONST const
#define STATIC static
#define EXTERN extern

#ifdef __cplusplus
#    define VIRTUAL virtual
#    define OVERRIDE override
#    define CONSTEXPR constexpr
#    define NOEXCEPT noexcept
#    define NULLPTR nullptr
#    define IA_MOVE(...) std::move(__VA_ARGS__)
#    define DECONST(t, v) const_cast<t>(v)
#    define NORETURN [[noreturn]]
#else
#    define VIRTUAL
#    define OVERRIDE
#    define CONSTEXPR const
#    define NOEXCEPT
#    define NULLPTR NULL
#    define IA_MOVE(...) (__VA_ARGS__)
#    define DECONST(t, v) ((t) (v))
#    define NORETURN
#endif

#define DEFINE_TYPE(t, v) typedef v t
#define FORWARD_DECLARE(t, i) t i

#ifdef __cplusplus
#    define PURE_VIRTUAL(f) VIRTUAL f = 0
#endif

// -----------------------------------------------------------------------------
// Attributes & Compiler Intrinsics
// -----------------------------------------------------------------------------

#define INLINE inline
#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#    define ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#    define ALWAYS_INLINE __forceinline
#else
#    define ALWAYS_INLINE inline
#endif

#define UNUSED(v) ((void) v);

#if defined(__cplusplus)
#    define NO_DISCARD(s) [[nodiscard(s)]]
#    define B_LIKELY(cond) (cond) [[likely]]
#    define B_UNLIKELY(cond) (cond) [[unlikely]]
#else
#    define NO_DISCARD(s)
#    define B_LIKELY(cond) (cond)
#    define B_UNLIKELY(cond) (cond)
#endif

#define __INTERNAL_IA_STRINGIFY(value) #value
#define IA_STRINGIFY(value) __INTERNAL_IA_STRINGIFY(value)

#define ALIGN(a) __attribute__((aligned(a)))

#define ASM(...) __asm__ volatile(__VA_ARGS__)

#ifndef NULL
#    define NULL 0
#endif

#ifndef _WIN32
#    undef TRUE
#    undef FALSE
#    ifdef __cplusplus
#        define FALSE false
#        define TRUE true
#    else
#        define FALSE 0
#        define TRUE 1
#    endif
#endif

// Parameter Annotations
#define IN
#define OUT
#define INOUT

// -----------------------------------------------------------------------------
// Extern C Handling
// -----------------------------------------------------------------------------

#ifdef __cplusplus
#    define IA_EXTERN_C_BEGIN                                                                                          \
        extern "C"                                                                                                     \
        {
#    define IA_EXTERN_C_END }
#    define C_DECL(f) extern "C" f
#else
#    define IA_EXTERN_C_BEGIN
#    define IA_EXTERN_C_END
#    define C_DECL(f) f
#endif

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------

#ifdef __cplusplus
#    define CAST(v, t) (static_cast<t>(v))
#    define REINTERPRET(v, t) (reinterpret_cast<t>(v))
#else
#    define CAST(v, t) ((t) (v))
#endif

// Templates and Aliases
#ifdef __cplusplus
#    define ALIAS_FUNCTION(alias, function)                                                                            \
        template<typename... Args> auto alias(Args &&...args) -> decltype(function(std::forward<Args>(args)...))       \
        {                                                                                                              \
            return function(std::forward<Args>(args)...);                                                              \
        }

#    define ALIAS_TEMPLATE_FUNCTION(t, alias, function)                                                                \
        template<typename t, typename... Args>                                                                         \
        auto alias(Args &&...args) -> decltype(function<t>(std::forward<Args>(args)...))                               \
        {                                                                                                              \
            return function<t>(std::forward<Args>(args)...);                                                           \
        }
#endif

// Assertions
#define IA_RELEASE_ASSERT(v) assert((v))
#define IA_RELEASE_ASSERT_MSG(v, m) assert((v) && m)

#if defined(__DEBUG_MODE__)
#    define IA_ASSERT(v) IA_RELEASE_ASSERT(v)
#    define IA_ASSERT_MSG(v, m) IA_RELEASE_ASSERT_MSG(v, m)
#else
#    define IA_ASSERT(v)
#    define IA_ASSERT_MSG(v, m)
#endif

#define IA_ASSERT_EQ(a, b) IA_ASSERT((a) == (b))
#define IA_ASSERT_GE(a, b) IA_ASSERT((a) >= (b))
#define IA_ASSERT_LE(a, b) IA_ASSERT(a <= b)
#define IA_ASSERT_LT(a, b) IA_ASSERT(a < b)
#define IA_ASSERT_GT(a, b) IA_ASSERT(a > b)
#define IA_ASSERT_IMPLIES(a, b) IA_ASSERT(!(a) || (b))
#define IA_ASSERT_NOT_NULL(v) IA_ASSERT(((v) != NULLPTR))

#define IA_UNREACHABLE(msg) IA_RELEASE_ASSERT_MSG(FALSE, "Unreachable code: " msg)

#define IA_TRY_PURE(expr)                                                                                              \
    {                                                                                                                  \
        auto _ia_res = (expr);                                                                                         \
        if (!_ia_res)                                                                                                  \
        {                                                                                                              \
            return tl::make_unexpected(std::move(_ia_res.error()));                                                    \
        }                                                                                                              \
    }

#define IA_TRY(expr)                                                                                                   \
    __extension__({                                                                                                    \
        auto _ia_res = (expr);                                                                                         \
        if (!_ia_res)                                                                                                  \
        {                                                                                                              \
            return tl::make_unexpected(std::move(_ia_res.error()));                                                    \
        }                                                                                                              \
        std::move(*_ia_res);                                                                                           \
    })

#define IA_CONCAT_IMPL(x, y) x##y
#define IA_CONCAT(x, y) IA_CONCAT_IMPL(x, y)
#define IA_UNIQUE_NAME(prefix) IA_CONCAT(prefix, __LINE__)

#define SIZE_KB(v) (v * 1024)
#define SIZE_MB(v) (v * 1024 * 1024)
#define SIZE_GB(v) (v * 1024 * 1024 * 1024)

#define ENSURE_BINARY_COMPATIBILITY(A, B)                                                                              \
    static_assert(sizeof(A) == sizeof(B),                                                                              \
                  #A ", " #B " size mismatch! Do not add virtual functions or new member variables.");

// -----------------------------------------------------------------------------
// Limits & Versioning
// -----------------------------------------------------------------------------

#ifdef __cplusplus
#    define IA_MAX_POSSIBLE_SIZE (static_cast<SIZE_T>(0x7FFFFFFFFFFFF))
#else
#    define IA_MAX_POSSIBLE_SIZE ((SIZE_T) (0x7FFFFFFFFFFFF))
#endif

#define IA_MAX_PATH_LENGTH 4096
#define IA_MAX_STRING_LENGTH (IA_MAX_POSSIBLE_SIZE >> 8)

#define IA_VERSION_TYPE uint64_t
#define IA_MAKE_VERSION(major, minor, patch)                                                                           \
    ((((uint64_t) (major) & 0xFFFFFF) << 40) | (((uint64_t) (minor) & 0xFFFFFF) << 16) | ((uint64_t) (patch) & 0xFFFF))

// -----------------------------------------------------------------------------
// DLL Export/Import
// -----------------------------------------------------------------------------

#if defined(_MSC_VER)
#    define IA_DLL_EXPORT __declspec(dllexport)
#    define IA_DLL_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
#    define IA_DLL_EXPORT __attribute__((visibility("default")))
#    define IA_DLL_IMPORT
#else
#    define IA_DLL_EXPORT
#    define IA_DLL_IMPORT
#endif

// -----------------------------------------------------------------------------
// Console Colors (ANSI Escape Codes)
// -----------------------------------------------------------------------------

#define __CC_BLACK "\033[30m"
#define __CC_RED "\033[31m"
#define __CC_GREEN "\033[32m"
#define __CC_YELLOW "\033[33m"
#define __CC_BLUE "\033[34m"
#define __CC_MAGENTA "\033[35m"
#define __CC_CYAN "\033[36m"
#define __CC_WHITE "\033[37m"
#define __CC_DEFAULT "\033[39m"

// -------------------------------------------------------------------------
// Base Types
// -------------------------------------------------------------------------
typedef void VOID;

#ifndef _WIN32
#    ifdef __cplusplus
typedef bool BOOL;
#    else
typedef _Bool BOOL;
#    endif
#endif

typedef char CHAR;
typedef uint16_t CHAR16;

typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

typedef float FLOAT32;
typedef double FLOAT64;

typedef INT32 INT;
typedef UINT32 UINT;
typedef size_t SIZE_T;

#ifdef __cplusplus
typedef std::make_signed_t<size_t> SSIZE_T;
typedef std::align_val_t ALIGN_T;
#else
typedef ptrdiff_t SSIZE_T;
typedef size_t ALIGN_T;
#endif

// -------------------------------------------------------------------------
// Pointer Types
// -------------------------------------------------------------------------
typedef VOID *PVOID;
typedef BOOL *PBOOL;
typedef CHAR *PCHAR;
typedef CHAR16 *PCHAR16;

typedef INT8 *PINT8;
typedef INT16 *PINT16;
typedef INT32 *PINT32;
typedef INT64 *PINT64;

typedef UINT8 *PUINT8;
typedef UINT16 *PUINT16;
typedef UINT32 *PUINT32;
typedef UINT64 *PUINT64;

typedef INT *PINT;
typedef UINT *PUINT;

typedef FLOAT32 *PFLOAT32;
typedef FLOAT64 *PFLOAT64;

// -------------------------------------------------------------------------
// Const Pointer Types
// -------------------------------------------------------------------------
typedef CONST VOID *PCVOID;
typedef CONST BOOL *PCBOOL;
typedef CONST CHAR *PCCHAR;
typedef CONST CHAR16 *PCCHAR16;

typedef CONST INT8 *PCINT8;
typedef CONST INT16 *PCINT16;
typedef CONST INT32 *PCINT32;
typedef CONST INT64 *PCINT64;

typedef CONST UINT8 *PCUINT8;
typedef CONST UINT16 *PCUINT16;
typedef CONST UINT32 *PCUINT32;
typedef CONST UINT64 *PCUINT64;

typedef CONST INT *PCINT;
typedef CONST UINT *PCUINT;
typedef CONST SIZE_T *PCSIZE;
typedef CONST SSIZE_T *PCSSIZE;

typedef CONST FLOAT32 *PCFLOAT32;
typedef CONST FLOAT64 *PCFLOAT64;

// -------------------------------------------------------------------------
// GUID Structure
// -------------------------------------------------------------------------
#ifndef _WIN32
typedef struct _IA_GUID
{
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8 Data4[8];

#    ifdef __cplusplus
    bool operator==(const _IA_GUID &other) const
    {
        return __builtin_memcmp(this, &other, sizeof(_IA_GUID)) == 0;
    }

    bool operator!=(const _IA_GUID &other) const
    {
        return !(*this == other);
    }
#    endif
} GUID;
#endif

STATIC INLINE BOOL IA_GUID_Equals(CONST GUID *a, CONST GUID *b)
{
    if (a == NULLPTR || b == NULLPTR)
        return FALSE;
    return memcmp(a, b, sizeof(GUID)) == 0;
}

// -------------------------------------------------------------------------
// Numeric Constants
// -------------------------------------------------------------------------
#ifdef __cplusplus
STATIC CONSTEXPR FLOAT32 FLOAT32_EPSILON = std::numeric_limits<FLOAT32>::epsilon();
STATIC CONSTEXPR FLOAT64 FLOAT64_EPSILON = std::numeric_limits<FLOAT64>::epsilon();
#else
STATIC CONST FLOAT32 FLOAT32_EPSILON = FLT_EPSILON;
STATIC CONST FLOAT64 FLOAT64_EPSILON = DBL_EPSILON;
#endif

// -------------------------------------------------------------------------
// Containers and Helpers
// -------------------------------------------------------------------------
#ifdef __cplusplus

template<typename _function_type> using Function = std::function<_function_type>;
template<typename _value_type> using InitializerList = std::initializer_list<_value_type>;
template<typename _value_type, SIZE_T count> using Array = std::array<_value_type, count>;
template<typename _value_type> using Vector = std::vector<_value_type>;
template<typename _value_type> using Optional = std::optional<_value_type>;
template<typename _key_type> using UnorderedSet = ankerl::unordered_dense::set<_key_type>;
template<typename _value_type> using Span = std::span<_value_type>;
template<typename _key_type, typename _value_type>
using UnorderedMap = ankerl::unordered_dense::map<_key_type, _value_type>;
template<typename _value_type> using Atomic = std::atomic<_value_type>;
template<typename _value_type> using SharedPtr = std::shared_ptr<_value_type>;
template<typename _value_type> using UniquePtr = std::unique_ptr<_value_type>;
template<typename _value_type> using Deque = std::deque<_value_type>;
template<typename _type_a, typename _type_b> using Pair = std::pair<_type_a, _type_b>;
template<typename... types> using Tuple = std::tuple<types...>;
template<typename _key_type, typename _value_type> using KeyValuePair = std::pair<_key_type, _value_type>;

template<typename _expected_type, typename _unexpected_type>
using Expected = tl::expected<_expected_type, _unexpected_type>;
ALIAS_FUNCTION(MakeUnexpected, tl::make_unexpected);

using String = std::string;
using StringView = std::string_view;
using StringStream = std::stringstream;

using SteadyClock = std::chrono::steady_clock;
using SteadyTimePoint = std::chrono::time_point<SteadyClock>;
using HighResClock = std::chrono::high_resolution_clock;
using HighResTimePoint = std::chrono::time_point<HighResClock>;

using Mutex = std::mutex;
using StopToken = std::stop_token;
using ScopedLock = std::scoped_lock<Mutex>;
using UniqueLock = std::unique_lock<Mutex>;
using JoiningThread = std::jthread;
using ConditionVariable = std::condition_variable;

namespace FileSystem = std::filesystem;
using FilePath = FileSystem::path;

template<typename... Args> using FormatterString = std::format_string<Args...>;

#endif
