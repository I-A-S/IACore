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

#pragma once

#include <IACore/PCH.hpp>

namespace IACore
{
  class RingBufferView
  {
public:
    static constexpr const u16 PACKET_ID_SKIP = 0;

    struct ControlBlock
    {
      struct alignas(64)
      {
        Mut<std::atomic<u32>> write_offset{0};
      } producer;

      struct alignas(64)
      {
        Mut<std::atomic<u32>> read_offset{0};
        Mut<u32> capacity{0};
      } consumer;
    };

    static_assert(offsetof(ControlBlock, consumer) == 64, "False sharing detected in ControlBlock");

    struct PacketHeader
    {
      PacketHeader() : id(0), payload_size(0)
      {
      }

      PacketHeader(const u16 id) : id(id), payload_size(0)
      {
      }

      PacketHeader(const u16 id, const u16 payload_size) : id(id), payload_size(payload_size)
      {
      }

      Mut<u16> id{};
      Mut<u16> payload_size{};
    };

public:
    static auto default_instance() -> RingBufferView;

    static auto create(Ref<Span<u8>> buffer, const bool is_owner) -> Result<RingBufferView>;
    static auto create(ControlBlock *control_block, Ref<Span<u8>> buffer, const bool is_owner)
        -> Result<RingBufferView>;

    // Returns:
    // - nullopt if empty
    // - bytes_read if success
    // - Error if buffer too small
    auto pop(MutRef<PacketHeader> out_header, Ref<Span<u8>> out_buffer) -> Result<Option<usize>>;

    auto push(const u16 packet_id, Ref<Span<const u8>> data) -> Result<void>;

    auto get_control_block() -> ControlBlock *;

    [[nodiscard]] auto is_valid() const -> bool;

protected:
    RingBufferView(Ref<Span<u8>> buffer, const bool is_owner);
    RingBufferView(ControlBlock *control_block, Ref<Span<u8>> buffer, const bool is_owner);

private:
    Mut<u8 *> m_data_ptr{};
    Mut<u32> m_capacity{};
    Mut<ControlBlock *> m_control_block{};

private:
    auto write_wrapped(const u32 offset, const void *data, const u32 size) -> void;
    auto read_wrapped(const u32 offset, void *out_data, const u32 size) -> void;
  };

  inline auto RingBufferView::default_instance() -> RingBufferView
  {
    return RingBufferView(nullptr, {}, false);
  }

  inline auto RingBufferView::create(Ref<Span<u8>> buffer, const bool is_owner) -> Result<RingBufferView>
  {
    if (buffer.size() <= sizeof(ControlBlock))
    {
      return fail("Buffer too small for ControlBlock");
    }

    if (!is_owner)
    {
      const ControlBlock *cb = reinterpret_cast<ControlBlock *>(buffer.data());
      const u32 capacity = static_cast<u32>(buffer.size()) - sizeof(ControlBlock);
      if (cb->consumer.capacity != capacity)
      {
        return fail("Capacity mismatch");
      }
    }

    return RingBufferView(buffer, is_owner);
  }

  inline auto RingBufferView::create(ControlBlock *control_block, Ref<Span<u8>> buffer, const bool is_owner)
      -> Result<RingBufferView>
  {
    if (control_block == nullptr)
    {
      return fail("ControlBlock is null");
    }
    if (buffer.empty())
    {
      return fail("Buffer is empty");
    }

    return RingBufferView(control_block, buffer, is_owner);
  }

  inline RingBufferView::RingBufferView(Ref<Span<u8>> buffer, const bool is_owner)
  {
    m_control_block = reinterpret_cast<ControlBlock *>(buffer.data());
    m_data_ptr = buffer.data() + sizeof(ControlBlock);

    m_capacity = static_cast<u32>(buffer.size()) - sizeof(ControlBlock);

    if (is_owner)
    {
      m_control_block->consumer.capacity = m_capacity;
      m_control_block->producer.write_offset.store(0, std::memory_order_release);
      m_control_block->consumer.read_offset.store(0, std::memory_order_release);
    }
  }

  inline RingBufferView::RingBufferView(ControlBlock *control_block, Ref<Span<u8>> buffer, const bool is_owner)
  {
    m_control_block = control_block;
    m_data_ptr = buffer.data();
    m_capacity = static_cast<u32>(buffer.size());

    if (is_owner)
    {
      m_control_block->consumer.capacity = m_capacity;
      m_control_block->producer.write_offset.store(0, std::memory_order_release);
      m_control_block->consumer.read_offset.store(0, std::memory_order_release);
    }
  }

  inline auto RingBufferView::pop(MutRef<PacketHeader> out_header, Ref<Span<u8>> out_buffer) -> Result<Option<usize>>
  {
    const u32 write = m_control_block->producer.write_offset.load(std::memory_order_acquire);
    const u32 read = m_control_block->consumer.read_offset.load(std::memory_order_relaxed);
    const u32 cap = m_capacity;

    if (read == write)
    {
      return std::nullopt;
    }

    read_wrapped(read, &out_header, sizeof(PacketHeader));

    if (out_header.payload_size > out_buffer.size())
    {
      return fail("Buffer too small: needed {}, provided {}", out_header.payload_size, out_buffer.size());
    }

    if (out_header.payload_size > 0)
    {
      const u32 data_read_offset = (read + sizeof(PacketHeader)) % cap;
      read_wrapped(data_read_offset, out_buffer.data(), out_header.payload_size);
    }

    const u32 new_read_offset = (read + sizeof(PacketHeader) + out_header.payload_size) % cap;
    m_control_block->consumer.read_offset.store(new_read_offset, std::memory_order_release);

    return std::make_optional(static_cast<usize>(out_header.payload_size));
  }

  inline auto RingBufferView::push(const u16 packet_id, Ref<Span<const u8>> data) -> Result<void>
  {
    if (data.size() > std::numeric_limits<u16>::max())
    {
      return fail("Data size exceeds u16 limit");
    }

    const u32 total_size = sizeof(PacketHeader) + static_cast<u32>(data.size());

    const u32 read = m_control_block->consumer.read_offset.load(std::memory_order_acquire);
    const u32 write = m_control_block->producer.write_offset.load(std::memory_order_relaxed);
    const u32 cap = m_capacity;

    const u32 free_space = (read <= write) ? (m_capacity - write) + read : (read - write);

    // Leave 1 byte empty (prevent ambiguities)
    if (free_space <= total_size)
    {
      return fail("RingBuffer full");
    }

    const PacketHeader header{packet_id, static_cast<u16>(data.size())};
    write_wrapped(write, &header, sizeof(PacketHeader));

    const u32 data_write_offset = (write + sizeof(PacketHeader)) % cap;

    if (!data.empty())
    {
      write_wrapped(data_write_offset, data.data(), static_cast<u32>(data.size()));
    }

    const u32 new_write_offset = (data_write_offset + data.size()) % cap;
    m_control_block->producer.write_offset.store(new_write_offset, std::memory_order_release);

    return {};
  }

  inline auto RingBufferView::get_control_block() -> ControlBlock *
  {
    return m_control_block;
  }

  inline auto RingBufferView::write_wrapped(const u32 offset, const void *data, const u32 size) -> void
  {
    if (offset + size <= m_capacity)
    {
      std::memcpy(m_data_ptr + offset, data, size);
    }
    else
    {
      const u32 first_chunk = m_capacity - offset;
      const u32 second_chunk = size - first_chunk;

      const u8 *src = static_cast<const u8 *>(data);

      std::memcpy(m_data_ptr + offset, src, first_chunk);
      std::memcpy(m_data_ptr, src + first_chunk, second_chunk);
    }
  }

  inline auto RingBufferView::read_wrapped(const u32 offset, void *out_data, const u32 size) -> void
  {
    if (offset + size <= m_capacity)
    {
      std::memcpy(out_data, m_data_ptr + offset, size);
    }
    else
    {
      const u32 first_chunk = m_capacity - offset;
      const u32 second_chunk = size - first_chunk;

      u8 *dst = static_cast<u8 *>(out_data);

      std::memcpy(dst, m_data_ptr + offset, first_chunk);
      std::memcpy(dst + first_chunk, m_data_ptr, second_chunk);
    }
  }

  [[nodiscard]] inline auto RingBufferView::is_valid() const -> bool
  {
    return m_control_block && m_data_ptr && m_capacity;
  }

} // namespace IACore