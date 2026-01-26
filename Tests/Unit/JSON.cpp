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
#include <IACore/JSON.hpp>

using namespace IACore;

struct UserProfile
{
  String username;
  u32 id;
  bool is_active;
  Vec<String> roles;

  bool operator==(const UserProfile &other) const
  {
    return username == other.username && id == other.id && is_active == other.is_active && roles == other.roles;
  }
};

IAT_BEGIN_BLOCK(Core, JSON)

auto test_dynamic_parse() -> bool
{
  const String json_text = R"({
        "string": "Hello World",
        "int": 42,
        "float": 3.14159,
        "bool": true,
        "array": [10, 20, 30],
        "object": { "key": "value" }
    })";

  auto res = Json::parse(json_text);
  IAT_CHECK(res.has_value());

  const auto &j = *res;

  IAT_CHECK(j["string"].is_string());
  IAT_CHECK_EQ(j["string"].get<String>(), String("Hello World"));

  IAT_CHECK(j["int"].is_number_integer());
  IAT_CHECK_EQ(j["int"].get<i32>(), 42);

  IAT_CHECK(j["float"].is_number_float());
  IAT_CHECK_APPROX(j["float"].get<f32>(), 3.14159f);

  IAT_CHECK(j["bool"].is_boolean());
  IAT_CHECK_EQ(j["bool"].get<bool>(), true);

  IAT_CHECK(j["array"].is_array());
  IAT_CHECK_EQ(j["array"].size(), 3u);
  IAT_CHECK_EQ(j["array"][0].get<i32>(), 10);

  IAT_CHECK(j["object"].is_object());
  IAT_CHECK_EQ(j["object"]["key"].get<String>(), String("value"));

  return true;
}

auto test_dynamic_encode() -> bool
{
  nlohmann::json j;
  j["name"] = "IACore";
  j["version"] = 2;

  const String encoded = Json::encode(j);

  IAT_CHECK(encoded.find("IACore") != String::npos);
  IAT_CHECK(encoded.find("version") != String::npos);
  IAT_CHECK(encoded.find("2") != String::npos);

  return true;
}

auto test_parse_invalid() -> bool
{
  const String bad_json = "{ key: value }";
  auto res = Json::parse(bad_json);
  IAT_CHECK_NOT(res.has_value());
  return true;
}

auto test_struct_round_trip() -> bool
{
  UserProfile original{.username = "test_user", .id = 12345, .is_active = true, .roles = {"admin", "editor"}};

  auto encode_res = Json::encode_struct(original);
  IAT_CHECK(encode_res.has_value());
  String json_str = *encode_res;

  IAT_CHECK(json_str.find("test_user") != String::npos);
  IAT_CHECK(json_str.find("roles") != String::npos);

  auto decode_res = Json::parse_to_struct<UserProfile>(json_str);
  IAT_CHECK(decode_res.has_value());

  UserProfile decoded = *decode_res;
  IAT_CHECK(decoded == original);

  return true;
}

auto test_struct_parse_error() -> bool
{
  const String malformed = "{ broken_json: ";
  auto res = Json::parse_to_struct<UserProfile>(malformed);
  IAT_CHECK_NOT(res.has_value());
  return true;
}

auto test_read_only() -> bool
{
  const String json_text = R"({
        "id": 999,
        "name": "Simd",
        "scores": [1.1, 2.2]
    })";

  auto res = Json::parse_read_only(json_text);
  IAT_CHECK(res.has_value());

  auto &doc = *res;
  simdjson::dom::element root = doc.root();

  u64 id = 0;
  auto err_id = root["id"].get(id);
  IAT_CHECK(!err_id);
  IAT_CHECK_EQ(id, 999ULL);

  std::string_view name;
  auto err_name = root["name"].get(name);
  IAT_CHECK(!err_name);
  IAT_CHECK_EQ(String(name), String("Simd"));

  simdjson::dom::array scores;
  auto err_arr = root["scores"].get(scores);
  IAT_CHECK(!err_arr);
  IAT_CHECK_EQ(scores.size(), 2u);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_dynamic_parse);
IAT_ADD_TEST(test_dynamic_encode);
IAT_ADD_TEST(test_parse_invalid);
IAT_ADD_TEST(test_struct_round_trip);
IAT_ADD_TEST(test_struct_parse_error);
IAT_ADD_TEST(test_read_only);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, JSON)