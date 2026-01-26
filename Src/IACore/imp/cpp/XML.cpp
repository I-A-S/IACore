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

#include <IACore/XML.hpp>
#include <sstream>

namespace IACore
{

  auto XML::parse_from_string(Ref<String> data) -> Result<Document>
  {
    Mut<Document> doc;
    const pugi::xml_parse_result parse_result = doc.load_string(data.c_str());
    if (!parse_result)
    {
      return fail("Failed to parse XML {}", parse_result.description());
    }
    return std::move(doc);
  }

  auto XML::parse_from_file(Ref<Path> path) -> Result<Document>
  {
    Mut<Document> doc;
    const pugi::xml_parse_result parse_result = doc.load_file(path.string().c_str());
    if (!parse_result)
    {
      return fail("Failed to parse XML {}", parse_result.description());
    }
    return std::move(doc);
  }

  auto XML::serialize_to_string(Ref<Node> node, const bool escape) -> String
  {
    Mut<std::ostringstream> oss;
    node.print(oss);
    return escape ? escape_xml_string(oss.str()) : oss.str();
  }

  auto XML::serialize_to_string(Ref<Document> doc, const bool escape) -> String
  {
    Mut<std::ostringstream> oss;
    doc.save(oss);
    return escape ? escape_xml_string(oss.str()) : oss.str();
  }

  auto XML::escape_xml_string(Ref<String> xml) -> String
  {
    Mut<String> buffer;
    buffer.reserve(xml.size() + (xml.size() / 10));

    for (const char c : xml)
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