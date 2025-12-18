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

#include <IACore/XML.hpp>

namespace IACore
{
    Expected<XML::Document, String> XML::ParseFromString(IN CONST String &data)
    {
        Document doc;
        const auto parseResult = doc.load_string(data.data());
        if (!parseResult)
            return MakeUnexpected(std::format("Failed to parse XML: {}", parseResult.description()));
        return IA_MOVE(doc);
    }

    Expected<XML::Document, String> XML::ParseFromFile(IN CONST FilePath &path)
    {
        Document doc;
        const auto parseResult = doc.load_file(path.string().c_str());
        if (!parseResult)
            return MakeUnexpected(std::format("Failed to parse XML: {}", parseResult.description()));
        return IA_MOVE(doc);
    }

    String XML::SerializeToString(IN CONST Node &node, IN BOOL escape)
    {
        std::ostringstream oss;
        node.print(oss);
        return escape ? EscapeXMLString(oss.str()) : oss.str();
    }

    String XML::SerializeToString(IN CONST Document &doc, IN BOOL escape)
    {
        std::ostringstream oss;
        doc.save(oss);
        return escape ? EscapeXMLString(oss.str()) : oss.str();
    }

    String XML::EscapeXMLString(IN CONST String &xml)
    {
        String buffer;
        buffer.reserve(xml.size() + (xml.size() / 10));

        for (char c : xml)
        {
            switch (c)
            {
            case '&':
                buffer.append("&amp;");
                break;
            case '\"':
                buffer.append("&quot;");
                break;
            case '\'':
                buffer.append("&apos;");
                break;
            case '<':
                buffer.append("&lt;");
                break;
            case '>':
                buffer.append("&gt;");
                break;
            default:
                buffer.push_back(c);
                break;
            }
        }

        return buffer;
    }
} // namespace IACore