// IACore-OSS; The Core Library for All IA Open Source Projects
// Copyright (C) 2025 IAS (ias@iasoft.dev)
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

#include <simdjson.h>
#include <glaze/glaze.hpp>
#include <nlohmann/json.hpp>

namespace IACore
{
    class JSON
    {
      private:
        STATIC CONSTEXPR AUTO GLAZE_JSON_OPTS = glz::opts{.error_on_unknown_keys = false};

      public:
        STATIC Expected<nlohmann::json, String> Parse(IN CONST String &json);
        STATIC Expected<Pair<SharedPtr<simdjson::dom::parser>, simdjson::dom::object>, String> ParseReadOnly(
            IN CONST String &json);
        template<typename _object_type> STATIC Expected<_object_type, String> ParseToStruct(IN CONST String &json);

        STATIC String Encode(IN nlohmann::json data);
        template<typename _object_type> STATIC Expected<String, String> EncodeStruct(IN CONST _object_type &data);
    };

    template<typename _object_type> Expected<_object_type, String> JSON::ParseToStruct(IN CONST String &json)
    {
        _object_type result{};
        const auto parseError = glz::read_json<GLAZE_JSON_OPTS>(result, json);
        if (parseError)
            return MakeUnexpected(std::format("JSON Error: {}", glz::format_error(parseError, json)));
        return result;
    }

    template<typename _object_type> Expected<String, String> JSON::EncodeStruct(IN CONST _object_type &data)
    {
        String result;
        const auto encodeError = glz::write_json(data, result);
        if (encodeError)
            return MakeUnexpected(std::format("JSON Error: {}", glz::format_error(encodeError)));
        return result;
    }
} // namespace IACore