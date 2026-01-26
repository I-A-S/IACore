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

#include <IACore/StringOps.hpp>

namespace IACore
{

  static const String BASE64_CHAR_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  static auto is_base64(const u8 c) -> bool
  {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '+') || (c == '/');
  }

  static auto get_base64_index(const u8 c) -> u8
  {
    if (c >= 'A' && c <= 'Z')
      return c - 'A';
    if (c >= 'a' && c <= 'z')
      return c - 'a' + 26;
    if (c >= '0' && c <= '9')
      return c - '0' + 52;
    if (c == '+')
      return 62;
    if (c == '/')
      return 63;
    return 0;
  }

  auto StringOps::encode_base64(const Span<const u8> data) -> String
  {
    Mut<String> result;
    result.reserve(((data.size() + 2) / 3) * 4);

    for (Mut<usize> i = 0; i < data.size(); i += 3)
    {
      const u32 b0 = data[i];
      const u32 b1 = (i + 1 < data.size()) ? data[i + 1] : 0;
      const u32 b2 = (i + 2 < data.size()) ? data[i + 2] : 0;

      const u32 triple = (b0 << 16) | (b1 << 8) | b2;

      result += BASE64_CHAR_TABLE[(triple >> 18) & 0x3F];
      result += BASE64_CHAR_TABLE[(triple >> 12) & 0x3F];

      if (i + 1 < data.size())
      {
        result += BASE64_CHAR_TABLE[(triple >> 6) & 0x3F];
      }
      else
      {
        result += '=';
      }

      if (i + 2 < data.size())
      {
        result += BASE64_CHAR_TABLE[triple & 0x3F];
      }
      else
      {
        result += '=';
      }
    }
    return result;
  }

  auto StringOps::decode_base64(Ref<String> data) -> Vec<u8>
  {
    Mut<Vec<u8>> result;
    result.reserve(data.size() * 3 / 4);

    Mut<i32> i = 0;
    Mut<Array<u8, 4>> tmp_buf = {};

    for (const char c_char : data)
    {
      const u8 c = static_cast<u8>(c_char);
      if (c == '=')
      {
        break;
      }
      if (!is_base64(c))
      {
        break;
      }

      tmp_buf[i++] = c;
      if (i == 4)
      {
        const u8 n0 = get_base64_index(tmp_buf[0]);
        const u8 n1 = get_base64_index(tmp_buf[1]);
        const u8 n2 = get_base64_index(tmp_buf[2]);
        const u8 n3 = get_base64_index(tmp_buf[3]);

        result.push_back((n0 << 2) | ((n1 & 0x30) >> 4));
        result.push_back(((n1 & 0x0F) << 4) | ((n2 & 0x3C) >> 2));
        result.push_back(((n2 & 0x03) << 6) | n3);

        i = 0;
      }
    }

    if (i > 0)
    {
      for (Mut<i32> j = i; j < 4; ++j)
      {
        tmp_buf[j] = 'A';
      }

      const u8 n0 = get_base64_index(tmp_buf[0]);
      const u8 n1 = get_base64_index(tmp_buf[1]);
      const u8 n2 = get_base64_index(tmp_buf[2]);

      if (i > 1)
      {
        result.push_back((n0 << 2) | ((n1 & 0x30) >> 4));
      }
      if (i > 2)
      {
        result.push_back(((n1 & 0x0F) << 4) | ((n2 & 0x3C) >> 2));
      }
    }

    return result;
  }

} // namespace IACore