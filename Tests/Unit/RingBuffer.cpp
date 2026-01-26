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

#include <IACore/ADT/RingBuffer.hpp>
#include <IACore/IATest.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, RingBuffer)

auto test_push_pop() -> bool
{

  Vec<u8> memory(sizeof(RingBufferView::ControlBlock) + 1024);

  auto producer_res = RingBufferView::create(Span<u8>(memory), true);
  IAT_CHECK(producer_res.has_value());
  auto producer = std::move(*producer_res);

  auto consumer_res = RingBufferView::create(Span<u8>(memory), false);
  IAT_CHECK(consumer_res.has_value());
  auto consumer = std::move(*consumer_res);

  String msg = "Hello RingBuffer";
  const auto push_res = producer.push(1, Span<const u8>(reinterpret_cast<const u8 *>(msg.data()), msg.size()));
  IAT_CHECK(push_res.has_value());

  RingBufferView::PacketHeader header;
  u8 read_buf[128];

  const auto pop_res = consumer.pop(header, Span<u8>(read_buf, 128));
  IAT_CHECK(pop_res.has_value());

  const auto bytes_read_opt = *pop_res;
  IAT_CHECK(bytes_read_opt.has_value());

  const usize bytes_read = *bytes_read_opt;

  IAT_CHECK_EQ(header.id, static_cast<u16>(1));
  IAT_CHECK_EQ(bytes_read, static_cast<usize>(msg.size()));

  String read_msg(reinterpret_cast<char *>(read_buf), bytes_read);
  IAT_CHECK_EQ(read_msg, msg);

  return true;
}

auto test_wrap_around() -> bool
{

  Vec<u8> memory(sizeof(RingBufferView::ControlBlock) + 100);

  auto rb_res = RingBufferView::create(Span<u8>(memory), true);
  IAT_CHECK(rb_res.has_value());
  auto rb = std::move(*rb_res);

  Vec<u8> junk(80, 0xFF);
  const auto push1 = rb.push(1, junk);
  IAT_CHECK(push1.has_value());

  RingBufferView::PacketHeader header;
  u8 out_buf[100];
  const auto pop1 = rb.pop(header, out_buf);
  IAT_CHECK(pop1.has_value());
  IAT_CHECK(pop1->has_value());

  Vec<u8> wrap_data(40, 0xAA);
  const auto push2 = rb.push(2, wrap_data);
  IAT_CHECK(push2.has_value());

  const auto pop2 = rb.pop(header, out_buf);
  IAT_CHECK(pop2.has_value());
  IAT_CHECK(pop2->has_value());

  const usize pop_size = *pop2.value();
  IAT_CHECK_EQ(pop_size, static_cast<usize>(40));

  bool match = true;
  for (usize i = 0; i < 40; i++)
  {
    if (out_buf[i] != 0xAA)
    {
      match = false;
    }
  }
  IAT_CHECK(match);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_push_pop);
IAT_ADD_TEST(test_wrap_around);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, RingBuffer)