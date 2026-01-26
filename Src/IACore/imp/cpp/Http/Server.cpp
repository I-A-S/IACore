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

#include <IACore/Http/Server.hpp>

namespace IACore
{

  auto HttpServer::Request::get_header(Ref<String> key) const -> String
  {
    if (auto it = headers.find(key); it != headers.end())
    {
      return it->second;
    }
    return "";
  }

  auto HttpServer::Request::get_param(Ref<String> key) const -> String
  {
    if (auto it = params.find(key); it != params.end())
    {
      return it->second;
    }
    return "";
  }

  auto HttpServer::Request::get_path_param(Ref<String> key) const -> String
  {
    if (auto it = path_params.find(key); it != path_params.end())
    {
      return it->second;
    }
    return "";
  }

  auto HttpServer::Request::has_header(Ref<String> key) const -> bool
  {
    return headers.contains(key);
  }

  auto HttpServer::Request::has_param(Ref<String> key) const -> bool
  {
    return params.contains(key);
  }

  auto HttpServer::Request::has_path_param(Ref<String> key) const -> bool
  {
    return path_params.contains(key);
  }

  void HttpServer::Response::set_content(Ref<String> content, Ref<String> type)
  {
    body = content;
    content_type = type;
  }

  void HttpServer::Response::set_status(const EResponseCode status_code)
  {
    code = status_code;
  }

  void HttpServer::Response::add_header(Ref<String> key, Ref<String> value)
  {
    headers[key] = value;
  }

  struct PublicHttpServer : public HttpServer
  {
    PublicHttpServer() = default;
  };

  HttpServer::HttpServer() = default;

  HttpServer::~HttpServer()
  {
    stop();
  }

  auto HttpServer::create() -> Result<Box<HttpServer>>
  {
    return make_box<PublicHttpServer>();
  }

  auto HttpServer::listen(Ref<String> host, const u32 port) -> Result<void>
  {
    if (!m_server.listen(host.c_str(), static_cast<int>(port)))
    {
      return fail("Failed to start HTTP server on {}:{}", host, port);
    }
    return {};
  }

  void HttpServer::stop()
  {
    if (m_server.is_running())
    {
      m_server.stop();
    }
  }

  auto HttpServer::is_running() const -> bool
  {
    return m_server.is_running();
  }

  void HttpServer::register_handler(Ref<String> method, Ref<String> pattern, const Handler handler)
  {
    const httplib::Server::Handler wrapper = [handler](Ref<httplib::Request> req, MutRef<httplib::Response> res) {
      Mut<Request> ia_req;
      ia_req.path = req.path;
      ia_req.method = req.method;
      ia_req.body = req.body;

      for (Ref<Pair<const String, String>> item : req.headers)
      {
        ia_req.headers[item.first] = item.second;
      }

      for (Ref<Pair<const String, String>> item : req.params)
      {
        ia_req.params[item.first] = item.second;
      }

      for (Ref<Pair<const String, String>> item : req.path_params)
      {
        ia_req.path_params[item.first] = item.second;
      }

      Mut<Response> ia_res;
      handler(ia_req, ia_res);

      res.status = static_cast<int>(ia_res.code);
      res.set_content(ia_res.body, ia_res.content_type.c_str());

      for (Ref<Pair<String, String>> item : ia_res.headers)
      {
        res.set_header(item.first.c_str(), item.second.c_str());
      }
    };

    if (method == "GET")
    {
      m_server.Get(pattern.c_str(), wrapper);
    }
    else if (method == "POST")
    {
      m_server.Post(pattern.c_str(), wrapper);
    }
    else if (method == "PUT")
    {
      m_server.Put(pattern.c_str(), wrapper);
    }
    else if (method == "DELETE")
    {
      m_server.Delete(pattern.c_str(), wrapper);
    }
    else if (method == "OPTIONS")
    {
      m_server.Options(pattern.c_str(), wrapper);
    }
  }

  void HttpServer::get(Ref<String> pattern, const Handler handler)
  {
    register_handler("GET", pattern, handler);
  }

  void HttpServer::post(Ref<String> pattern, const Handler handler)
  {
    register_handler("POST", pattern, handler);
  }

  void HttpServer::put(Ref<String> pattern, const Handler handler)
  {
    register_handler("PUT", pattern, handler);
  }

  void HttpServer::del(Ref<String> pattern, const Handler handler)
  {
    register_handler("DELETE", pattern, handler);
  }

  void HttpServer::options(Ref<String> pattern, const Handler handler)
  {
    register_handler("OPTIONS", pattern, handler);
  }

} // namespace IACore