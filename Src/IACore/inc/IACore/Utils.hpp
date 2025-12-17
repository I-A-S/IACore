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

#include <IACore/PCH.hpp>

#include <algorithm>

namespace IACore
{

    class Utils
    {
      public:
        INLINE STATIC String BinaryToHexString(std::span<const UINT8> data)
        {
            STATIC CONSTEXPR char LUT[] = "0123456789ABCDEF";
            String res;
            res.reserve(data.size() * 2);

            for (UINT8 b : data)
            {
                res.push_back(LUT[(b >> 4) & 0x0F]);
                res.push_back(LUT[b & 0x0F]);
            }
            return res;
        }

        INLINE STATIC tl::expected<Vector<UINT8>, String> HexStringToBinary(CONST StringView &hex)
        {
            if (hex.size() % 2 != 0)
            {
                return tl::make_unexpected(String("Hex string must have even length"));
            }

            Vector<UINT8> out;
            out.reserve(hex.size() / 2);

            for (SIZE_T i = 0; i < hex.size(); i += 2)
            {
                char high = hex[i];
                char low = hex[i + 1];

                auto fromHexChar = [](char c) -> int {
                    if (c >= '0' && c <= '9')
                        return c - '0';
                    if (c >= 'A' && c <= 'F')
                        return c - 'A' + 10;
                    if (c >= 'a' && c <= 'f')
                        return c - 'a' + 10;
                    return -1;
                };

                int h = fromHexChar(high);
                int l = fromHexChar(low);

                if (h == -1 || l == -1)
                {
                    return tl::make_unexpected(String("Invalid hex character found"));
                }

                out.push_back(CAST((h << 4) | l, UINT8));
            }

            return out;
        }

        template<typename Range> INLINE STATIC VOID Sort(Range &&range)
        {
            std::ranges::sort(range);
        }

        template<typename Range, typename T> INLINE STATIC auto BinarySearchLeft(Range &&range, CONST T &value)
        {
            return std::ranges::lower_bound(range, value);
        }

        template<typename Range, typename T> INLINE STATIC auto BinarySearchRight(Range &&range, CONST T &value)
        {
            return std::ranges::upper_bound(range, value);
        }

        template<typename T> INLINE STATIC void HashCombine(UINT64 &seed, CONST T &v)
        {
            UINT64 h;

            if constexpr (std::is_constructible_v<std::string_view, T>)
            {
                std::string_view sv(v);
                auto hasher = ankerl::unordered_dense::hash<std::string_view>();
                h = hasher(sv);
            }
            else
            {
                auto hasher = ankerl::unordered_dense::hash<T>();
                h = hasher(v);
            }

            seed ^= h + 0x9e3779b97f4a7c15 + (seed << 6) + (seed >> 2);
        }

        template<typename... Args> INLINE STATIC UINT64 ComputeHash(CONST Args &...args)
        {
            UINT64 seed = 0;
            (HashCombine(seed, args), ...);
            return seed;
        }

        template<typename T, typename... MemberPtrs>
        INLINE STATIC UINT64 ComputeHashFlat(CONST T &obj, MemberPtrs... members)
        {
            UINT64 seed = 0;
            (HashCombine(seed, obj.*members), ...);
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
    template<> struct ankerl::unordered_dense::hash<Type>                                                              \
    {                                                                                                                  \
        using is_avalanching = void;                                                                                   \
        NO_DISCARD("Hash value should be used")                                                                        \
        UINT64 operator()(CONST Type &v) const NOEXCEPT                                                                \
        {                                                                                                              \
            /* Pass the object and the list of member pointers */                                                      \
            return IACore::Utils::ComputeHashFlat(v, __VA_ARGS__);                                                     \
        }                                                                                                              \
    };
