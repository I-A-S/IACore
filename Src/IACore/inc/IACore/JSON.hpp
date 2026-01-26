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

#include <IACore/PCH.hpp>

#include <glaze/glaze.hpp>
#include <nlohmann/json.hpp>
#include <simdjson.h>

namespace IACore
{
  class JsonDocument
  {
public:
    JsonDocument(ForwardRef<JsonDocument>) noexcept = default;
    auto operator=(ForwardRef<JsonDocument>) noexcept -> MutRef<JsonDocument> = default;

    JsonDocument(Ref<JsonDocument>) = delete;
    auto operator=(Ref<JsonDocument>) -> MutRef<JsonDocument> = delete;

    [[nodiscard]]
    auto root() const noexcept -> simdjson::dom::element
    {
      return m_root;
    }

private:
    friend class Json;

    JsonDocument(Mut<Box<simdjson::dom::parser>> p, Mut<simdjson::dom::element> r) : m_parser(std::move(p)), m_root(r)
    {
    }

    Mut<Box<simdjson::dom::parser>> m_parser;
    Mut<simdjson::dom::element> m_root;
  };

  class Json
  {
private:
    static constexpr const glz::opts GLAZE_OPTS = glz::opts{.error_on_unknown_keys = false};

public:
    static auto parse(Ref<String> json_str) -> Result<nlohmann::json>;
    static auto encode(Ref<nlohmann::json> data) -> String;

    static auto parse_read_only(Ref<String> json_str) -> Result<JsonDocument>;

    template<typename T> static auto parse_to_struct(Ref<String> json_str) -> Result<T>;

    template<typename T> static auto encode_struct(Ref<T> data) -> Result<String>;
  };

  inline auto Json::parse(Ref<String> json_str) -> Result<nlohmann::json>
  {
    const nlohmann::json res = nlohmann::json::parse(json_str, nullptr, false, true);

    if (res.is_discarded())
    {
      return fail("Failed to parse JSON (Invalid Syntax)");
    }
    return res;
  }

  inline auto Json::parse_read_only(Ref<String> json_str) -> Result<JsonDocument>
  {
    Mut<Box<simdjson::dom::parser>> parser = make_box<simdjson::dom::parser>();

    Mut<simdjson::dom::element> root;

    const simdjson::error_code error = parser->parse(json_str).get(root);

    if (error)
    {
      return fail("JSON Error: {}", simdjson::error_message(error));
    }

    return JsonDocument(std::move(parser), root);
  }

  inline auto Json::encode(Ref<nlohmann::json> data) -> String
  {
    return data.dump();
  }

  template<typename T> inline auto Json::parse_to_struct(Ref<String> json_str) -> Result<T>
  {
    Mut<T> result{};

    const glz::error_ctx err = glz::read<GLAZE_OPTS>(result, json_str);

    if (err)
    {
      return fail("JSON Struct Parse Error: {}", glz::format_error(err, json_str));
    }
    return result;
  }

  template<typename T> inline auto Json::encode_struct(Ref<T> data) -> Result<String>
  {
    Mut<String> result;
    const glz::error_ctx err = glz::write_json(data, result);

    if (err)
    {
      return fail("JSON Struct Encode Error");
    }
    return result;
  }
} // namespace IACore