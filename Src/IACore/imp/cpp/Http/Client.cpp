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

namespace IACore
{
  auto HttpClient::create(Ref<String> host) -> Result<Box<HttpClient>>
  {
    return make_box_protected<HttpClient>(httplib::Client(host));
  }

  static auto build_headers(Span<const HttpClient::Header> headers, const char *default_content_type)
      -> httplib::Headers
  {
    Mut<httplib::Headers> out;
    Mut<bool> has_content_type = false;

    for (Ref<HttpClient::Header> h : headers)
    {
      out.emplace(h.first, h.second);

      if (h.first == HttpClient::header_type_to_string(HttpClient::EHeaderType::CONTENT_TYPE))
      {
        has_content_type = true;
      }
    }

    if (!has_content_type && default_content_type)
    {
      out.emplace("Content-Type", default_content_type);
    }
    return out;
  }

  HttpClient::HttpClient(ForwardRef<httplib::Client> client)
      : m_client(std::move(client)), m_last_response_code(EResponseCode::INTERNAL_SERVER_ERROR)
  {
    m_client.enable_server_certificate_verification(true);
  }

  HttpClient::~HttpClient() = default;

  auto HttpClient::enable_certificate_verification() -> void
  {
    m_client.enable_server_certificate_verification(true);
  }

  auto HttpClient::disable_certificate_verification() -> void
  {
    m_client.enable_server_certificate_verification(false);
  }

  auto HttpClient::preprocess_response(Ref<String> response) -> String
  {
    const Span<const u8> response_bytes = {reinterpret_cast<const u8 *>(response.data()), response.size()};
    const DataOps::CompressionType compression = DataOps::detect_compression(response_bytes);

    switch (compression)
    {
    case DataOps::CompressionType::Gzip: {
      const Result<Vec<u8>> data = DataOps::gzip_inflate(response_bytes);
      if (!data)
      {
        return response;
      }
      return String(reinterpret_cast<const char *>(data->data()), data->size());
    }

    case DataOps::CompressionType::Zlib: {
      const Result<Vec<u8>> data = DataOps::zlib_inflate(response_bytes);
      if (!data)
      {
        return response;
      }
      return String(reinterpret_cast<const char *>(data->data()), data->size());
    }

    case DataOps::CompressionType::None:
    default:
      break;
    }
    return response;
  }

  auto HttpClient::raw_get(Ref<String> path, Span<const Header> headers, const char *default_content_type)
      -> Result<String>
  {
    const httplib::Headers http_headers = build_headers(headers, default_content_type);

    Mut<String> adjusted_path = path;
    if (!path.empty() && path[0] != '/')
    {
      adjusted_path = "/" + path;
    }

    const httplib::Result res = m_client.Get(adjusted_path.c_str(), http_headers);

    if (res)
    {
      m_last_response_code = static_cast<EResponseCode>(res->status);
      if (res->status >= 200 && res->status < 300)
      {
        return preprocess_response(res->body);
      }
      return fail("HTTP Error {} : {}", res->status, res->body);
    }

    return fail("Network Error: {}", httplib::to_string(res.error()));
  }

  auto HttpClient::raw_post(Ref<String> path, Span<const Header> headers, Ref<String> body,
                            const char *default_content_type) -> Result<String>
  {
    Mut<httplib::Headers> http_headers = build_headers(headers, default_content_type);

    Mut<String> content_type = default_content_type;
    if (http_headers.count("Content-Type"))
    {
      const httplib::Headers::iterator t = http_headers.find("Content-Type");
      content_type = t->second;
      http_headers.erase(t);
    }

    m_client.set_keep_alive(true);

    Mut<String> adjusted_path = path;
    if (!path.empty() && path[0] != '/')
    {
      adjusted_path = "/" + path;
    }

    const httplib::Result res = m_client.Post(adjusted_path.c_str(), http_headers, body, content_type.c_str());

    if (res)
    {
      m_last_response_code = static_cast<EResponseCode>(res->status);
      if (res->status >= 200 && res->status < 300)
      {
        return preprocess_response(res->body);
      }
      return fail("HTTP Error {} : {}", res->status, res->body);
    }

    return fail("Network Error: {}", httplib::to_string(res.error()));
  }
} // namespace IACore