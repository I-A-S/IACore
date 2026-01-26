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

#include <IACore/Http/Common.hpp>
#include <IACore/JSON.hpp>

typedef void CURL;
struct curl_slist;

namespace IACore
{
  class HttpClient : public HttpCommon
  {
public:
    static auto create(Ref<String> host) -> Result<Box<HttpClient>>;
    ~HttpClient();

    HttpClient(const HttpClient &) = delete;
    auto operator=(const HttpClient &) -> HttpClient & = delete;

    HttpClient(HttpClient &&other) noexcept;
    auto operator=(HttpClient &&other) noexcept -> HttpClient &;

public:
    auto raw_get(Ref<String> path, Span<const Header> headers,
                 const char *default_content_type = "application/x-www-form-urlencoded") -> Result<String>;

    auto raw_post(Ref<String> path, Span<const Header> headers, Ref<String> body,
                  const char *default_content_type = "application/x-www-form-urlencoded") -> Result<String>;

    template<typename ResponseType> auto json_get(Ref<String> path, Span<const Header> headers) -> Result<ResponseType>;

    template<typename PayloadType, typename ResponseType>
    auto json_post(Ref<String> path, Span<const Header> headers, Ref<PayloadType> body) -> Result<ResponseType>;

    auto enable_certificate_verification() -> void;
    auto disable_certificate_verification() -> void;

    auto last_response_code() -> EResponseCode
    {
      return m_last_response_code;
    }

protected:
    explicit HttpClient(Ref<String> host);

private:
    Mut<CURL *> m_curl = nullptr;
    Mut<String> m_host;
    Mut<EResponseCode> m_last_response_code = EResponseCode::INTERNAL_SERVER_ERROR;

    auto perform_request(Ref<String> full_url, struct curl_slist *headers) -> Result<String>;

    static auto write_callback(void *contents, size_t size, size_t nmemb, void *userp) -> size_t;
  };

  template<typename ResponseType>
  auto HttpClient::json_get(Ref<String> path, Span<const Header> headers) -> Result<ResponseType>
  {
    const String raw_response = AU_TRY(raw_get(path, headers, "application/json"));

    if (last_response_code() != EResponseCode::OK)
    {
      return fail("Server responded with code {}", static_cast<i32>(last_response_code()));
    }
    return Json::parse_to_struct<ResponseType>(raw_response);
  }

  template<typename PayloadType, typename ResponseType>
  auto HttpClient::json_post(Ref<String> path, Span<const Header> headers, Ref<PayloadType> body)
      -> Result<ResponseType>
  {
    const String encoded_body = AU_TRY(Json::encode_struct(body));

    const String raw_response = AU_TRY(raw_post(path, headers, encoded_body, "application/json"));

    if (last_response_code() != EResponseCode::OK)
    {
      return fail("Server responded with code {}", static_cast<i32>(last_response_code()));
    }
    return Json::parse_to_struct<ResponseType>(raw_response);
  }
} // namespace IACore