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
#include <IACore/IPC.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, IPC)

auto test_layout_constraints() -> bool
{

  IAT_CHECK_EQ(alignof(IpcSharedMemoryLayout), static_cast<usize>(64));

  IAT_CHECK_EQ(offsetof(IpcSharedMemoryLayout, meta), static_cast<usize>(0));

  IAT_CHECK_EQ(offsetof(IpcSharedMemoryLayout, moni_control), static_cast<usize>(64));

  IAT_CHECK_EQ(offsetof(IpcSharedMemoryLayout, mino_control), static_cast<usize>(192));

  IAT_CHECK_EQ(offsetof(IpcSharedMemoryLayout, moni_data_offset), static_cast<usize>(320));

  IAT_CHECK_EQ(sizeof(IpcSharedMemoryLayout) % 64, static_cast<usize>(0));

  return true;
}

auto test_manual_shm_ringbuffer() -> bool
{
  const String shm_name = "IA_TEST_IPC_LAYOUT_CHECK";
  const usize shm_size = 16 * 1024;

  FileOps::unlink_shared_memory(shm_name);

  auto map_res = FileOps::map_shared_memory(shm_name, shm_size, true);
  IAT_CHECK(map_res.has_value());

  u8 *base_ptr = *map_res;
  auto *layout = reinterpret_cast<IpcSharedMemoryLayout *>(base_ptr);

  layout->meta.magic = 0xDEADBEEF;
  layout->meta.version = 1;
  layout->meta.total_size = shm_size;

  const usize header_size = IpcSharedMemoryLayout::get_header_size();
  const usize data_available = shm_size - header_size;
  const usize half_data = data_available / 2;

  layout->moni_data_offset = header_size;
  layout->moni_data_size = half_data;

  layout->mino_data_offset = header_size + half_data;
  layout->mino_data_size = half_data;

  Span<u8> moni_data_span(base_ptr + layout->moni_data_offset, static_cast<usize>(layout->moni_data_size));
  auto moni_res = RingBufferView::create(&layout->moni_control, moni_data_span, true);
  IAT_CHECK(moni_res.has_value());
  auto moni = std::move(*moni_res);

  Span<u8> mino_data_span(base_ptr + layout->mino_data_offset, static_cast<usize>(layout->mino_data_size));
  auto mino_res = RingBufferView::create(&layout->mino_control, mino_data_span, true);
  IAT_CHECK(mino_res.has_value());
  auto _ = std::move(*mino_res);

  String msg = "IPC_TEST_MESSAGE";
  IAT_CHECK(moni.push(100, Span<const u8>(reinterpret_cast<const u8 *>(msg.data()), msg.size())).has_value());

  auto moni_reader_res = RingBufferView::create(&layout->moni_control, moni_data_span, false);
  IAT_CHECK(moni_reader_res.has_value());
  auto moni_reader = std::move(*moni_reader_res);

  RingBufferView::PacketHeader header;
  u8 buffer[128];
  auto pop_res = moni_reader.pop(header, Span<u8>(buffer, 128));
  IAT_CHECK(pop_res.has_value());
  IAT_CHECK(pop_res->has_value());
  IAT_CHECK_EQ(header.id, static_cast<u16>(100));

  String received((char *) buffer, *pop_res.value());
  IAT_CHECK_EQ(received, msg);

  FileOps::unmap_file(base_ptr);
  FileOps::unlink_shared_memory(shm_name);

  return true;
}

class TestManager : public IpcManager
{
  public:
  void on_signal(NativeProcessID, u8) override
  {
  }

  void on_packet(NativeProcessID, u16, Span<const u8>) override
  {
  }
};

auto test_manager_instantiation() -> bool
{
  TestManager mgr;
  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_layout_constraints);
IAT_ADD_TEST(test_manual_shm_ringbuffer);
IAT_ADD_TEST(test_manager_instantiation);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, IPC)