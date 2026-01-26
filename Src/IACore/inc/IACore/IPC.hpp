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

#include <IACore/ADT/RingBuffer.hpp>
#include <IACore/ProcessOps.hpp>
#include <IACore/SocketOps.hpp>

namespace IACore
{
  using IpcPacketHeader = RingBufferView::PacketHeader;

  struct alignas(64) IpcSharedMemoryLayout
  {
    // =========================================================
    // METADATA & HANDSHAKE
    // =========================================================
    struct Header
    {
      Mut<u32> magic;      // 0x49414950 ("IAIP")
      Mut<u32> version;    // 1
      Mut<u64> total_size; // Total size of SHM block
    };

    Mut<Header> meta;

    // Pad to ensure MONI starts on a fresh cache line (64 bytes)
    const Array<u8, 64 - sizeof(Header)> _pad0;

    // =========================================================
    // RING BUFFER CONTROL BLOCKS
    // =========================================================

    // RingBufferView ControlBlock is already 64-byte aligned internally.
    Mut<RingBufferView::ControlBlock> moni_control;
    Mut<RingBufferView::ControlBlock> mino_control;

    // =========================================================
    // DATA BUFFER OFFSETS
    // =========================================================

    Mut<u64> moni_data_offset;
    Mut<u64> moni_data_size;

    Mut<u64> mino_data_offset;
    Mut<u64> mino_data_size;

    // Pad to ensure the actual Data Buffer starts on a fresh cache line
    const Array<u8, 64 - (sizeof(u64) * 4)> _pad1;

    static constexpr auto get_header_size() -> usize
    {
      return sizeof(IpcSharedMemoryLayout);
    }
  };

  // Check padding logic is gucci
  static_assert(sizeof(IpcSharedMemoryLayout) % 64 == 0, "IPC Layout is not cache-line aligned!");

  class IpcNode
  {
public:
    virtual ~IpcNode();

    // When Manager spawns a node, `connection_string` is passed
    // as the first command line argument
    auto connect(const char *connection_string) -> Result<void>;

    auto update() -> void;

    auto send_signal(const u8 signal) -> void;
    auto send_packet(const u16 packet_id, const Span<const u8> payload) -> Result<void>;

protected:
    virtual auto on_signal(const u8 signal) -> void = 0;
    virtual auto on_packet(const u16 packet_id, const Span<const u8> payload) -> void = 0;

private:
    Mut<String> m_shm_name;
    Mut<u8 *> m_shared_memory{};
    Mut<Vec<u8>> m_receive_buffer;
    Mut<SocketHandle> m_socket{INVALID_SOCKET};

    Mut<RingBufferView> m_moni; // Manager Out, Node In
    Mut<RingBufferView> m_mino; // Manager In, Node Out
  };

  class IpcManager
  {
    struct NodeSession
    {
      Mut<std::chrono::system_clock::time_point> creation_time{};
      Mut<Box<ProcessHandle>> node_process;

      Mut<std::mutex> send_mutex;

      Mut<String> shared_mem_name;
      Mut<u8 *> mapped_ptr{};

      Mut<SocketHandle> listener_socket{INVALID_SOCKET};
      Mut<SocketHandle> data_socket{INVALID_SOCKET};

      Mut<RingBufferView> moni = RingBufferView::default_instance(); // Manager Out, Node In
      Mut<RingBufferView> mino = RingBufferView::default_instance(); // Manager In, Node Out

      Mut<bool> is_ready{false};

      auto send_signal(const u8 signal) -> void;
      auto send_packet(const u16 packet_id, const Span<const u8> payload) -> Result<void>;
    };

public:
    static constexpr const u32 DEFAULT_NODE_SHARED_MEMORY_SIZE = 4 * 1024 * 1024;

public:
    virtual ~IpcManager();

    auto update() -> void;

    auto spawn_node(Ref<Path> executable_path, const u32 shared_memory_size = DEFAULT_NODE_SHARED_MEMORY_SIZE)
        -> Result<NativeProcessID>;

    auto wait_till_node_is_online(const NativeProcessID node) -> bool;

    auto shutdown_node(const NativeProcessID node) -> void;

    auto send_signal(const NativeProcessID node, const u8 signal) -> void;
    auto send_packet(const NativeProcessID node, const u16 packet_id, const Span<const u8> payload) -> Result<void>;

protected:
    virtual auto on_signal(const NativeProcessID node, const u8 signal) -> void = 0;
    virtual auto on_packet(const NativeProcessID node, const u16 packet_id, const Span<const u8> payload) -> void = 0;

private:
    Mut<Vec<u8>> m_receive_buffer;
    Mut<Vec<Box<NodeSession>>> m_active_sessions;
    Mut<Vec<Box<NodeSession>>> m_pending_sessions;
    Mut<HashMap<NativeProcessID, NodeSession *>> m_active_session_map;

protected:
    IpcManager();
  };
} // namespace IACore