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

#include <IACore/DataOps.hpp>
#include <IACore/Http/Client.hpp>

#include <curl/curl.h>

namespace IACore
{
  struct CurlHeaders
  {
    struct curl_slist *list = nullptr;

    ~CurlHeaders()
    {
      if (list)
        curl_slist_free_all(list);
    }

    auto add(Ref<String> key, Ref<String> value) -> void
    {
      String header_str = key + ": " + value;
      list = curl_slist_append(list, header_str.c_str());
    }

    auto add(const char *raw) -> void
    {
      list = curl_slist_append(list, raw);
    }
  };

  static auto build_headers_list(Span<const HttpClient::Header> headers, const char *default_content_type)
      -> CurlHeaders
  {
    CurlHeaders out;
    bool has_content_type = false;

    for (const auto &h : headers)
    {
      out.add(h.first, h.second);
      if (h.first == HttpClient::header_type_to_string(HttpClient::EHeaderType::CONTENT_TYPE))
      {
        has_content_type = true;
      }
    }

    if (!has_content_type && default_content_type)
    {
      out.add("Content-Type", default_content_type);
    }
    return out;
  }

  auto HttpClient::create(Ref<String> host) -> Result<Box<HttpClient>>
  {
    curl_global_init(CURL_GLOBAL_ALL);
    return make_box_protected<HttpClient>(host);
  }

  HttpClient::HttpClient(Ref<String> host) : m_host(host)
  {
    m_curl = curl_easy_init();

    if (m_curl)
    {
      curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPALIVE, 1L);
      curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "");
      curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
      curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 2L);
    }
  }

  HttpClient::~HttpClient()
  {
    if (m_curl)
    {
      curl_easy_cleanup(m_curl);
      m_curl = nullptr;
    }
  }

  HttpClient::HttpClient(HttpClient &&other) noexcept
      : m_curl(other.m_curl), m_host(std::move(other.m_host)), m_last_response_code(other.m_last_response_code)
  {
    other.m_curl = nullptr;
  }

  auto HttpClient::operator=(HttpClient &&other) noexcept -> HttpClient &
  {
    if (this != &other)
    {
      if (m_curl)
        curl_easy_cleanup(m_curl);
      m_curl = other.m_curl;
      m_host = std::move(other.m_host);
      m_last_response_code = other.m_last_response_code;
      other.m_curl = nullptr;
    }
    return *this;
  }

  auto HttpClient::enable_certificate_verification() -> void
  {
    if (m_curl)
      curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
  }

  auto HttpClient::disable_certificate_verification() -> void
  {
    if (m_curl)
      curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
  }

  auto HttpClient::write_callback(void *contents, size_t size, size_t nmemb, void *userp) -> size_t
  {
    size_t realsize = size * nmemb;
    auto *str = static_cast<String *>(userp);
    str->append(static_cast<char *>(contents), realsize);
    return realsize;
  }

  auto HttpClient::perform_request(Ref<String> full_url, struct curl_slist *headers) -> Result<String>
  {
    if (!m_curl)
      return fail("CURL not initialized");

    String response_body;

    curl_easy_setopt(m_curl, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, HttpClient::write_callback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response_body);

    CURLcode res = curl_easy_perform(m_curl);

    if (res != CURLE_OK)
    {
      return fail("Network Error: {}", curl_easy_strerror(res));
    }

    long response_code;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);
    m_last_response_code = static_cast<EResponseCode>(response_code);

    if (response_code >= 200 && response_code < 300)
    {
      return response_body;
    }

    return fail("HTTP Error {} : {}", response_code, response_body);
  }

  auto HttpClient::raw_get(Ref<String> path, Span<const Header> headers, const char *default_content_type)
      -> Result<String>
  {
    String full_url = m_host;
    if (!path.empty() && path[0] != '/' && m_host.back() != '/')
      full_url += "/";
    full_url += path;

    auto curl_headers = build_headers_list(headers, default_content_type);

    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);

    return perform_request(full_url, curl_headers.list);
  }

  auto HttpClient::raw_post(Ref<String> path, Span<const Header> headers, Ref<String> body,
                            const char *default_content_type) -> Result<String>
  {
    String full_url = m_host;
    if (!path.empty() && path[0] != '/' && m_host.back() != '/')
      full_url += "/";
    full_url += path;

    auto curl_headers = build_headers_list(headers, default_content_type);

    curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, (long) body.size());

    return perform_request(full_url, curl_headers.list);
  }
} // namespace IACore