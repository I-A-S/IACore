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

using namespace IACore;

IAT_BEGIN_BLOCK(Core, FileOps)

void cleanup_file(const Path &path)
{
  std::error_code ec;
  if (std::filesystem::exists(path, ec))
  {
    std::filesystem::remove(path, ec);
  }
}

auto test_text_io() -> bool
{
  const Path path = "iatest_fileops_text.txt";
  const String content = "Hello IACore FileOps!\nLine 2";

  const auto write_res = FileOps::write_text_file(path, content, true);
  IAT_CHECK(write_res.has_value());
  IAT_CHECK_EQ(*write_res, content.size());

  const auto read_res = FileOps::read_text_file(path);
  IAT_CHECK(read_res.has_value());
  IAT_CHECK_EQ(*read_res, content);

  cleanup_file(path);
  return true;
}

auto test_binary_io() -> bool
{
  const Path path = "iatest_fileops_bin.bin";
  const Vec<u8> content = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xFF};

  const auto write_res = FileOps::write_binary_file(path, content, true);
  IAT_CHECK(write_res.has_value());
  IAT_CHECK_EQ(*write_res, content.size());

  const auto read_res = FileOps::read_binary_file(path);
  IAT_CHECK(read_res.has_value());
  IAT_CHECK_EQ(read_res->size(), content.size());

  for (usize i = 0; i < content.size(); ++i)
  {
    IAT_CHECK_EQ((*read_res)[i], content[i]);
  }

  cleanup_file(path);
  return true;
}

auto test_file_mapping() -> bool
{
  const Path path = "iatest_fileops_map.txt";
  const String content = "MappedContent";

  (void) FileOps::write_text_file(path, content, true);

  usize size = 0;
  const auto map_res = FileOps::map_file(path, size);
  IAT_CHECK(map_res.has_value());
  IAT_CHECK_EQ(size, content.size());

  const u8 *ptr = *map_res;
  IAT_CHECK(ptr != nullptr);

  String read_back(reinterpret_cast<const char *>(ptr), size);
  IAT_CHECK_EQ(read_back, content);

  FileOps::unmap_file(ptr);

  cleanup_file(path);
  return true;
}

auto test_shared_memory() -> bool
{
  const String shm_name = "iatest_shm_block";
  const usize shm_size = 4096;

  auto owner_res = FileOps::map_shared_memory(shm_name, shm_size, true);
  IAT_CHECK(owner_res.has_value());
  u8 *owner_ptr = *owner_res;

  std::memset(owner_ptr, 0, shm_size);
  const String msg = "Shared Memory Message";
  std::memcpy(owner_ptr, msg.data(), msg.size());

  auto client_res = FileOps::map_shared_memory(shm_name, shm_size, false);
  IAT_CHECK(client_res.has_value());
  u8 *client_ptr = *client_res;

  String read_msg(reinterpret_cast<const char *>(client_ptr), msg.size());
  IAT_CHECK_EQ(read_msg, msg);

  FileOps::unmap_file(owner_ptr);
  FileOps::unmap_file(client_ptr);
  FileOps::unlink_shared_memory(shm_name);

  return true;
}

auto test_stream_integration() -> bool
{
  const Path path = "iatest_fileops_stream.bin";
  cleanup_file(path);

  {
    auto writer_res = FileOps::stream_to_file(path, true);
    IAT_CHECK(writer_res.has_value());
    auto &writer = *writer_res;

    (void) writer.write<u32>(0x12345678);
    (void) writer.write<u8>(0xFF);
  }

  {
    auto reader_res = FileOps::stream_from_file(path);
    IAT_CHECK(reader_res.has_value());
    auto &reader = *reader_res;

    auto val_u32 = reader.read<u32>();
    IAT_CHECK(val_u32.has_value());
    IAT_CHECK_EQ(*val_u32, 0x12345678u);

    auto val_u8 = reader.read<u8>();
    IAT_CHECK(val_u8.has_value());
    IAT_CHECK_EQ(*val_u8, 0xFF);
  }

  cleanup_file(path);
  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_text_io);
IAT_ADD_TEST(test_binary_io);
IAT_ADD_TEST(test_file_mapping);
IAT_ADD_TEST(test_shared_memory);
IAT_ADD_TEST(test_stream_integration);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, FileOps)