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

#include <IACore/Http/Common.hpp>

namespace IACore
{
  auto HttpCommon::url_encode(Ref<String> value) -> String
  {
    Mut<std::stringstream> escaped;
    escaped.fill('0');
    escaped << std::hex << std::uppercase;

    for (const char c : value)
    {
      if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~')
        escaped << c;
      else
        escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
    }

    return escaped.str();
  }

  auto HttpCommon::url_decode(Ref<String> value) -> String
  {
    Mut<String> result;
    result.reserve(value.length());

    for (Mut<size_t> i = 0; i < value.length(); ++i)
    {
      if (value[i] == '%' && i + 2 < value.length())
      {
        const std::string hex_str = value.substr(i + 1, 2);
        const char decoded_char = static_cast<char>(std::strtol(hex_str.c_str(), nullptr, 16));
        result += decoded_char;
        i += 2;
      }
      else if (value[i] == '+')
        result += ' ';
      else
        result += value[i];
    }

    return result;
  }

  auto HttpCommon::header_type_to_string(const EHeaderType type) -> String
  {
    switch (type)
    {
    case EHeaderType::ACCEPT:
      return "Accept";
    case EHeaderType::ACCEPT_CHARSET:
      return "Accept-Charset";
    case EHeaderType::ACCEPT_ENCODING:
      return "Accept-Encoding";
    case EHeaderType::ACCEPT_LANGUAGE:
      return "Accept-Language";
    case EHeaderType::AUTHORIZATION:
      return "Authorization";
    case EHeaderType::CACHE_CONTROL:
      return "Cache-Control";
    case EHeaderType::CONNECTION:
      return "Connection";
    case EHeaderType::CONTENT_LENGTH:
      return "Content-Length";
    case EHeaderType::CONTENT_TYPE:
      return "Content-Type";
    case EHeaderType::COOKIE:
      return "Cookie";
    case EHeaderType::DATE:
      return "Date";
    case EHeaderType::EXPECT:
      return "Expect";
    case EHeaderType::HOST:
      return "Host";
    case EHeaderType::IF_MATCH:
      return "If-Match";
    case EHeaderType::IF_MODIFIED_SINCE:
      return "If-Modified-Since";
    case EHeaderType::IF_NONE_MATCH:
      return "If-None-Match";
    case EHeaderType::ORIGIN:
      return "Origin";
    case EHeaderType::PRAGMA:
      return "Pragma";
    case EHeaderType::PROXY_AUTHORIZATION:
      return "Proxy-Authorization";
    case EHeaderType::RANGE:
      return "Range";
    case EHeaderType::REFERER:
      return "Referer";
    case EHeaderType::TE:
      return "TE";
    case EHeaderType::UPGRADE:
      return "Upgrade";
    case EHeaderType::USER_AGENT:
      return "User-Agent";
    case EHeaderType::VIA:
      return "Via";
    case EHeaderType::WARNING:
      return "Warning";
    }
    return "<Unknown>";
  }

  auto HttpCommon::is_success_response_code(const EResponseCode code) -> bool
  {
    return (i32) code >= 200 && (i32) code < 300;
  }
} // namespace IACore