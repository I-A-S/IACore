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

#include <IACore/Utils.hpp>
#include <chrono>
#include <cstdlib>

namespace IACore
{
  namespace
  {
    auto from_hex_char(const char c) -> i32
    {
      if (c >= '0' && c <= '9')
      {
        return c - '0';
      }
      if (c >= 'A' && c <= 'F')
      {
        return c - 'A' + 10;
      }
      if (c >= 'a' && c <= 'f')
      {
        return c - 'a' + 10;
      }
      return -1;
    }
  } // namespace

  extern Mut<std::chrono::high_resolution_clock::time_point> g_start_time;

  auto Utils::get_unix_time() -> u64
  {
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
  }

  auto Utils::get_ticks_count() -> u64
  {
    const std::chrono::high_resolution_clock::duration duration =
        std::chrono::high_resolution_clock::now() - g_start_time;
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  }

  auto Utils::get_seconds_count() -> f64
  {
    const std::chrono::high_resolution_clock::duration duration =
        std::chrono::high_resolution_clock::now() - g_start_time;
    return static_cast<f64>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
  }

  auto Utils::get_random() -> f32
  {
    return static_cast<f32>(std::rand()) / static_cast<f32>(RAND_MAX);
  }

  auto Utils::get_random(const u64 max) -> u64
  {
    return static_cast<u64>(static_cast<f32>(max) * get_random());
  }

  auto Utils::get_random(const i64 min, const i64 max) -> i64
  {
    return min + static_cast<i64>(static_cast<f32>(max - min) * get_random());
  }

  auto Utils::sleep(const u64 milliseconds) -> void
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  }

  auto Utils::binary_to_hex_string(const Span<const u8> data) -> String
  {
    static constexpr const char LUT[17] = "0123456789ABCDEF";
    Mut<String> res = String();
    res.reserve(data.size() * 2);

    for (u8 b : data)
    {
      res.push_back(LUT[(b >> 4) & 0x0F]);
      res.push_back(LUT[b & 0x0F]);
    }
    return res;
  }

  auto Utils::hex_string_to_binary(const StringView hex) -> Result<Vec<u8>>
  {
    if (hex.size() % 2 != 0)
    {
      return fail("Hex string must have even length");
    }

    Mut<Vec<u8>> out = Vec<u8>();
    out.reserve(hex.size() / 2);

    for (Mut<usize> i = 0; i < hex.size(); i += 2)
    {
      const char high = hex[i];
      const char low = hex[i + 1];

      const i32 h = from_hex_char(high);
      const i32 l = from_hex_char(low);

      if (h == -1 || l == -1)
      {
        return fail("Invalid hex character found");
      }

      out.push_back(static_cast<u8>((h << 4) | l));
    }

    return out;
  }
} // namespace IACore