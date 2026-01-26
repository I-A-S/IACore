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
#include <IACore/StreamWriter.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, StreamWriter)

auto test_memory_writer() -> bool
{
  StreamWriter writer;

  IAT_CHECK(writer.write(static_cast<u8>(0xAA), 1).has_value());

  const u32 val = 0x12345678;
  IAT_CHECK(writer.write(val).has_value());

  IAT_CHECK_EQ(writer.cursor(), static_cast<usize>(1 + 4));

  const u8 *ptr = writer.data();
  IAT_CHECK_EQ(ptr[0], 0xAA);
  IAT_CHECK_EQ(ptr[1], 0x78);
  IAT_CHECK_EQ(ptr[4], 0x12);

  return true;
}

auto test_fixed_buffer() -> bool
{
  u8 buffer[4] = {0};
  StreamWriter writer(Span<u8>(buffer, 4));

  IAT_CHECK(writer.write(static_cast<u8>(0xFF), 2).has_value());
  IAT_CHECK_EQ(writer.cursor(), static_cast<usize>(2));

  IAT_CHECK(writer.write(static_cast<u8>(0xEE), 2).has_value());
  IAT_CHECK_EQ(writer.cursor(), static_cast<usize>(4));

  const auto res = writer.write(static_cast<u8>(0x00), 1);
  IAT_CHECK_NOT(res.has_value());

  IAT_CHECK_EQ(buffer[0], 0xFF);
  IAT_CHECK_EQ(buffer[1], 0xFF);
  IAT_CHECK_EQ(buffer[2], 0xEE);
  IAT_CHECK_EQ(buffer[3], 0xEE);

  return true;
}

auto test_file_writer() -> bool
{
  const Path path = "test_stream_writer.bin";

  if (std::filesystem::exists(path))
  {
    std::filesystem::remove(path);
  }

  {
    auto res = StreamWriter::create_from_file(path);
    IAT_CHECK(res.has_value());
    StreamWriter writer = std::move(*res);

    const String hello = "Hello World";
    IAT_CHECK(writer.write(hello.data(), hello.size()).has_value());

    IAT_CHECK(writer.flush().has_value());
  }

  auto read_res = FileOps::read_binary_file(path);
  IAT_CHECK(read_res.has_value());

  const String read_str(reinterpret_cast<const char *>(read_res->data()), read_res->size());
  IAT_CHECK_EQ(read_str, String("Hello World"));

  std::filesystem::remove(path);

  return true;
}

auto test_primitives() -> bool
{
  StreamWriter writer;

  const f32 f = 1.5f;
  const u64 big = 0xDEADBEEFCAFEBABE;

  IAT_CHECK(writer.write(f).has_value());
  IAT_CHECK(writer.write(big).has_value());

  IAT_CHECK_EQ(writer.cursor(), sizeof(f32) + sizeof(u64));

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_memory_writer);
IAT_ADD_TEST(test_fixed_buffer);
IAT_ADD_TEST(test_file_writer);
IAT_ADD_TEST(test_primitives);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, StreamWriter)