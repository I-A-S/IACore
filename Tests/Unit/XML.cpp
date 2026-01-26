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

#include <IACore/FileOps.hpp>
#include <IACore/IATest.hpp>
#include <IACore/XML.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, XML)

auto test_parse_string() -> bool
{
  const String xml_content = R"(
        <root>
            <item id="1">Value1</item>
            <item id="2">Value2</item>
        </root>
    )";

  auto res = XML::parse_from_string(xml_content);
  IAT_CHECK(res.has_value());

  auto &doc = *res;
  auto root = doc.child("root");
  IAT_CHECK(root);

  auto item1 = root.find_child_by_attribute("item", "id", "1");
  IAT_CHECK(item1);
  IAT_CHECK_EQ(String(item1.child_value()), String("Value1"));

  auto item2 = root.find_child_by_attribute("item", "id", "2");
  IAT_CHECK(item2);
  IAT_CHECK_EQ(String(item2.child_value()), String("Value2"));

  return true;
}

auto test_parse_error() -> bool
{
  const String invalid_xml = "<root><unclosed>";
  auto res = XML::parse_from_string(invalid_xml);
  IAT_CHECK_NOT(res.has_value());
  return true;
}

auto test_serialize() -> bool
{
  const String xml_content = "<root><node>Text</node></root>";
  auto res = XML::parse_from_string(xml_content);
  IAT_CHECK(res.has_value());

  String output = XML::serialize_to_string(*res);

  IAT_CHECK(output.find("<root>") != String::npos);
  IAT_CHECK(output.find("<node>Text</node>") != String::npos);

  return true;
}

auto test_escape() -> bool
{
  const String raw = "< & > \" '";
  const String escaped = XML::escape_xml_string(raw);

  IAT_CHECK(escaped.find("&lt;") != String::npos);
  IAT_CHECK(escaped.find("&amp;") != String::npos);
  IAT_CHECK(escaped.find("&gt;") != String::npos);
  IAT_CHECK(escaped.find("&quot;") != String::npos);
  IAT_CHECK(escaped.find("&apos;") != String::npos);

  return true;
}

auto test_file_io() -> bool
{
  const Path path = "test_temp_xml_doc.xml";
  const String content = "<config><ver>1.0</ver></config>";

  auto write_res = FileOps::write_text_file(path, content, true);
  IAT_CHECK(write_res.has_value());

  auto parse_res = XML::parse_from_file(path);
  IAT_CHECK(parse_res.has_value());

  auto &doc = *parse_res;
  IAT_CHECK_EQ(String(doc.child("config").child("ver").child_value()), String("1.0"));

  std::filesystem::remove(path);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_parse_string);
IAT_ADD_TEST(test_parse_error);
IAT_ADD_TEST(test_serialize);
IAT_ADD_TEST(test_escape);
IAT_ADD_TEST(test_file_io);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, XML)