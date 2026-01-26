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

#include <IACore/IATest.hpp>
#include <IACore/Http/Client.hpp>

using namespace IACore;

struct TestPayload
{
  String message;
  i32 id;

  friend std::ostream &operator<<(std::ostream &os, const TestPayload &p)
  {
    return os << "{ message: " << p.message << ", id: " << p.id << " }";
  }

  bool operator==(const TestPayload &other) const = default;
};

struct HttpBinGetResponse
{
  String url;
  String origin;

  friend std::ostream &operator<<(std::ostream &os, const HttpBinGetResponse &p)
  {
    return os << "HttpBinGetResponse { url: " << p.url << " }";
  }
};

struct HttpBinPostResponse
{
  TestPayload json;
  String url;

  friend std::ostream &operator<<(std::ostream &os, const HttpBinPostResponse &p)
  {
    return os << "HttpBinPostResponse { json: " << p.json << " }";
  }
};

IAT_BEGIN_BLOCK(Core, HttpClient)

auto test_raw_get() -> bool
{
  auto client_res = HttpClient::create("http://httpbin.org");
  IAT_CHECK(client_res.has_value());

  auto &client = *client_res;

  std::vector<HttpCommon::Header> headers;
  auto response = client->raw_get("/get", headers);

  IAT_CHECK(response.has_value());
  IAT_CHECK_EQ(client->last_response_code(), HttpCommon::EResponseCode::OK);

  String body = *response;
  IAT_CHECK(body.find("http://httpbin.org/get") != String::npos);

  return true;
}

auto test_json_get_typed() -> bool
{
  auto client_res = HttpClient::create("http://httpbin.org");
  IAT_CHECK(client_res.has_value());
  auto &client = *client_res;

  std::vector<HttpCommon::Header> headers;
  auto result = client->json_get<HttpBinGetResponse>("/get", headers);

  IAT_CHECK(result.has_value());

  const HttpBinGetResponse &resp = *result;
  IAT_CHECK_EQ(resp.url, String("http://httpbin.org/get"));
  IAT_CHECK_NOT(resp.origin.empty());

  return true;
}

auto test_json_post_roundtrip() -> bool
{
  auto client_res = HttpClient::create("http://httpbin.org");
  IAT_CHECK(client_res.has_value());
  auto &client = *client_res;

  TestPayload payload{.message = "IACore Test", .id = 999};
  std::vector<HttpCommon::Header> headers;

  auto result = client->json_post<TestPayload, HttpBinPostResponse>("/post", headers, payload);

  IAT_CHECK(result.has_value());

  const HttpBinPostResponse &resp = *result;

  IAT_CHECK_EQ(resp.json.message, payload.message);
  IAT_CHECK_EQ(resp.json.id, payload.id);

  return true;
}

auto test_https_support() -> bool
{
  auto client_res = HttpClient::create("https://httpbin.org");
  IAT_CHECK(client_res.has_value());
  auto &client = *client_res;

  std::vector<HttpCommon::Header> headers;
  auto response = client->raw_get("/get", headers);

  IAT_CHECK(response.has_value());
  IAT_CHECK_EQ(client->last_response_code(), HttpCommon::EResponseCode::OK);

  return true;
}

auto test_error_handling_404() -> bool
{
  auto client_res = HttpClient::create("http://httpbin.org");
  IAT_CHECK(client_res.has_value());
  auto &client = *client_res;

  std::vector<HttpCommon::Header> headers;

  auto raw_res = client->raw_get("/status/404", headers);

  IAT_CHECK(!raw_res.has_value());
  IAT_CHECK_EQ(client->last_response_code(), HttpCommon::EResponseCode::NOT_FOUND);

  return true;
}

auto test_json_fail_on_error() -> bool
{
  auto client_res = HttpClient::create("http://httpbin.org");
  auto &client = *client_res;
  std::vector<HttpCommon::Header> headers;

  auto result = client->json_get<HttpBinGetResponse>("/status/500", headers);

  IAT_CHECK_NOT(result.has_value());

  IAT_CHECK_EQ(client->last_response_code(), HttpCommon::EResponseCode::INTERNAL_SERVER_ERROR);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_raw_get);
IAT_ADD_TEST(test_json_get_typed);
IAT_ADD_TEST(test_json_post_roundtrip);
IAT_ADD_TEST(test_https_support);
IAT_ADD_TEST(test_error_handling_404);
IAT_ADD_TEST(test_json_fail_on_error);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, HttpClient)