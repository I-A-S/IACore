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
#include <IACore/StringOps.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, StringOps)

auto test_base64_encode() -> bool
{

  {
    const String s = "Hello World";
    const Span<const u8> data(reinterpret_cast<const u8 *>(s.data()), s.size());
    const String encoded = StringOps::encode_base64(data);
    IAT_CHECK_EQ(encoded, String("SGVsbG8gV29ybGQ="));
  }

  {
    const String s = "M";
    const Span<const u8> data(reinterpret_cast<const u8 *>(s.data()), s.size());
    const String encoded = StringOps::encode_base64(data);
    IAT_CHECK_EQ(encoded, String("TQ=="));
  }

  {
    const String s = "Ma";
    const Span<const u8> data(reinterpret_cast<const u8 *>(s.data()), s.size());
    const String encoded = StringOps::encode_base64(data);
    IAT_CHECK_EQ(encoded, String("TWE="));
  }

  {
    const String s = "Man";
    const Span<const u8> data(reinterpret_cast<const u8 *>(s.data()), s.size());
    const String encoded = StringOps::encode_base64(data);
    IAT_CHECK_EQ(encoded, String("TWFu"));
  }

  {
    const String encoded = StringOps::encode_base64({});
    IAT_CHECK(encoded.empty());
  }

  return true;
}

auto test_base64_decode() -> bool
{

  {
    const String encoded = "SGVsbG8gV29ybGQ=";
    const Vec<u8> decoded = StringOps::decode_base64(encoded);
    const String result(reinterpret_cast<const char *>(decoded.data()), decoded.size());
    IAT_CHECK_EQ(result, String("Hello World"));
  }

  {
    const Vec<u8> decoded = StringOps::decode_base64("");
    IAT_CHECK(decoded.empty());
  }

  return true;
}

auto test_base64_round_trip() -> bool
{
  Vec<u8> original;
  original.reserve(256);
  for (usize i = 0; i < 256; ++i)
  {
    original.push_back(static_cast<u8>(i));
  }

  const String encoded = StringOps::encode_base64(original);
  const Vec<u8> decoded = StringOps::decode_base64(encoded);

  IAT_CHECK_EQ(original.size(), decoded.size());

  bool match = true;
  for (usize i = 0; i < original.size(); ++i)
  {
    if (original[i] != decoded[i])
    {
      match = false;
      break;
    }
  }
  IAT_CHECK(match);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_base64_encode);
IAT_ADD_TEST(test_base64_decode);
IAT_ADD_TEST(test_base64_round_trip);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, StringOps)