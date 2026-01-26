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
#include <functional>

namespace IACore
{
  class HttpServer : public HttpCommon
  {
public:
    struct Request
    {
      Mut<String> path;
      Mut<String> method;
      Mut<String> body;
      Mut<HashMap<String, String>> headers;
      Mut<HashMap<String, String>> params;      // Query params
      Mut<HashMap<String, String>> path_params; // Path params (like /object/:id)

      [[nodiscard]] auto get_header(Ref<String> key) const -> String;
      [[nodiscard]] auto get_param(Ref<String> key) const -> String;
      [[nodiscard]] auto get_path_param(Ref<String> key) const -> String;

      [[nodiscard]] auto has_header(Ref<String> key) const -> bool;
      [[nodiscard]] auto has_param(Ref<String> key) const -> bool;
      [[nodiscard]] auto has_path_param(Ref<String> key) const -> bool;
    };

    struct Response
    {
      Mut<EResponseCode> code = EResponseCode::OK;
      Mut<String> body;
      Mut<HashMap<String, String>> headers;
      Mut<String> content_type = "text/plain";

      void set_content(Ref<String> content, Ref<String> type);
      void set_status(const EResponseCode status_code);
      void add_header(Ref<String> key, Ref<String> value);
    };

    using Handler = std::function<void(Ref<Request>, MutRef<Response>)>;

public:
    static auto create() -> Result<Box<HttpServer>>;

    ~HttpServer();

    HttpServer(HttpServer &&) = delete;
    HttpServer(const HttpServer &) = delete;
    auto operator=(HttpServer &&) -> HttpServer & = delete;
    auto operator=(const HttpServer &) -> HttpServer & = delete;

    auto listen(Ref<String> host, const u32 port) -> Result<void>;
    void stop();
    auto is_running() const -> bool;

    void get(Ref<String> pattern, const Handler handler);
    void post(Ref<String> pattern, const Handler handler);
    void put(Ref<String> pattern, const Handler handler);
    void del(Ref<String> pattern, const Handler handler);
    void options(Ref<String> pattern, const Handler handler);

    template<typename ResponseType>
    void json_get(Ref<String> pattern, const std::function<Result<ResponseType(Ref<Request>)>> handler);

    template<typename PayloadType, typename ResponseType>
    void json_post(Ref<String> pattern, const std::function<Result<ResponseType(Ref<PayloadType>)>> handler);

protected:
    HttpServer();

private:
    Mut<httplib::Server> m_server;

    void register_handler(Ref<String> method, Ref<String> pattern, const Handler handler);
  };

  template<typename ResponseType>
  void HttpServer::json_get(Ref<String> pattern, const std::function<Result<ResponseType(Ref<Request>)>> handler)
  {
    get(pattern, [handler](Ref<Request> req, MutRef<Response> res) {
      const Result<ResponseType> result = handler(req);
      if (!result)
      {
        res.set_status(EResponseCode::INTERNAL_SERVER_ERROR);
        res.set_content(result.error(), "text/plain");
        return;
      }

      const Result<String> json_res = Json::encode_struct(*result);
      if (!json_res)
      {
        res.set_status(EResponseCode::INTERNAL_SERVER_ERROR);
        res.set_content("Failed to encode JSON response", "text/plain");
        return;
      }

      res.set_status(EResponseCode::OK);
      res.set_content(*json_res, "application/json");
    });
  }

  template<typename PayloadType, typename ResponseType>
  void HttpServer::json_post(Ref<String> pattern, const std::function<Result<ResponseType(Ref<PayloadType>)>> handler)
  {
    post(pattern, [handler](Ref<Request> req, MutRef<Response> res) {
      const Result<PayloadType> payload = Json::parse_to_struct<PayloadType>(req.body);
      if (!payload)
      {
        res.set_status(EResponseCode::BAD_REQUEST);
        res.set_content("Invalid JSON Payload", "text/plain");
        return;
      }

      const Result<ResponseType> result = handler(*payload);
      if (!result)
      {
        res.set_status(EResponseCode::INTERNAL_SERVER_ERROR);
        res.set_content(result.error(), "text/plain");
        return;
      }

      const Result<String> json_res = Json::encode_struct(*result);
      if (!json_res)
      {
        res.set_status(EResponseCode::INTERNAL_SERVER_ERROR);
        res.set_content("Failed to encode JSON response", "text/plain");
        return;
      }

      res.set_status(EResponseCode::OK);
      res.set_content(*json_res, "application/json");
    });
  }

} // namespace IACore