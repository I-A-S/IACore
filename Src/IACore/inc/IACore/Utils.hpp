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

#include <algorithm>

namespace IACore
{
  class Utils
  {
public:
    static auto get_unix_time() -> u64;

    static auto get_ticks_count() -> u64;

    static auto get_seconds_count() -> f64;

    static auto get_random() -> f32;
    static auto get_random(const u64 max) -> u64;
    static auto get_random(const i64 min, const i64 max) -> i64;

    static auto sleep(const u64 milliseconds) -> void;

    static auto binary_to_hex_string(const Span<const u8> data) -> String;

    static auto hex_string_to_binary(const StringView hex) -> Result<Vec<u8>>;

    template<typename Range> inline static auto sort(ForwardRef<Range> range) -> void
    {
      std::ranges::sort(std::forward<Range>(range));
    }

    template<typename Range, typename T>
    inline static auto binary_search_left(ForwardRef<Range> range, Ref<T> value) -> auto
    {
      return std::ranges::lower_bound(std::forward<Range>(range), value);
    }

    template<typename Range, typename T>
    inline static auto binary_search_right(ForwardRef<Range> range, Ref<T> value) -> auto
    {
      return std::ranges::upper_bound(std::forward<Range>(range), value);
    }

    template<typename T> inline static auto hash_combine(MutRef<u64> seed, Ref<T> v) -> void
    {
      Mut<u64> h = 0;

      if constexpr (std::is_constructible_v<StringView, T>)
      {
        const StringView sv(v);
        const ankerl::unordered_dense::hash<StringView> hasher;
        h = hasher(sv);
      }
      else
      {
        const ankerl::unordered_dense::hash<T> hasher;
        h = hasher(v);
      }

      seed ^= h + 0x9e3779b97f4a7c15 + (seed << 6) + (seed >> 2);
    }

    template<typename... Args> inline static auto compute_hash(Ref<Args>... args) -> u64
    {
      Mut<u64> seed = 0;
      (hash_combine(seed, args), ...);
      return seed;
    }

    template<typename T, typename... MemberPtrs>
    inline static auto compute_hash_flat(Ref<T> obj, const MemberPtrs... members) -> u64
    {
      Mut<u64> seed = 0;
      (hash_combine(seed, obj.*members), ...);
      return seed;
    }
  };
} // namespace IACore

// -----------------------------------------------------------------------------
// MACRO: IA_MAKE_HASHABLE
//
// Injects the specialization for ankerl::unordered_dense::hash.
//
// Usage:
//   struct Vector3 { float x, y, z; };
//   IA_MAKE_HASHABLE(Vector3, &Vector3::x, &Vector3::y, &Vector3::z)
// -----------------------------------------------------------------------------
#define IA_MAKE_HASHABLE(Type, ...)                                                                                    \
  template<> struct ankerl::unordered_dense::hash<Type>                                                                \
  {                                                                                                                    \
    using is_avalanching = void;                                                                                       \
    IA_NODISCARD                                                                                                       \
    auto operator()(IACore::Ref<Type> v) const noexcept -> IACore::u64                                                 \
    {                                                                                                                  \
      return IACore::Utils::compute_hash_flat(v, __VA_ARGS__);                                                         \
    }                                                                                                                  \
  };