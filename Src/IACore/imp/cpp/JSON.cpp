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

#include <IACore/JSON.hpp>

namespace IACore
{
    Expected<nlohmann::json, String> JSON::Parse(IN CONST String &json)
    {
        const auto parseResult = nlohmann::json::parse(json, nullptr, false, true);
        if (parseResult.is_discarded())
            return MakeUnexpected("Failed to parse JSON");
        return parseResult;
    }

    Expected<Pair<SharedPtr<simdjson::dom::parser>, simdjson::dom::object>, String> JSON::ParseReadOnly(
        IN CONST String &json)
    {
        auto parser = std::make_shared<simdjson::dom::parser>();

        simdjson::error_code error{};
        simdjson::dom::object object;
        if ((error = parser->parse(json).get(object)))
            return MakeUnexpected(std::format("Failed to parse JSON : {}", simdjson::error_message(error)));
        return std::make_pair(IA_MOVE(parser), IA_MOVE(object));
    }

    String JSON::Encode(IN nlohmann::json data)
    {
        return data.dump();
    }
} // namespace IACore