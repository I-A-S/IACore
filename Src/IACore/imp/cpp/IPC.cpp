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

#include <IACore/IPC.hpp>

#include <IACore/FileOps.hpp>
#include <IACore/StringOps.hpp>
#include <charconv>
#include <fcntl.h>

namespace IACore
{
  struct IpcConnectionDescriptor
  {
    Mut<String> socket_path;
    Mut<String> shared_mem_path;
    Mut<u32> shared_mem_size;

    [[nodiscard]] auto serialize() const -> String
    {
      return std::format("{}|{}|{}|", socket_path, shared_mem_path, shared_mem_size);
    }

    static auto deserialize(const StringView data) -> Option<IpcConnectionDescriptor>
    {
      enum class ParseState
      {
        SocketPath,
        SharedMemPath,
        SharedMemSize
      };

      Mut<IpcConnectionDescriptor> result{};
      Mut<usize> t = 0;
      Mut<ParseState> state = ParseState::SocketPath;

      for (Mut<usize> i = 0; i < data.size(); ++i)
      {
        if (data[i] != '|')
        {
          continue;
        }

        switch (state)
        {
        case ParseState::SocketPath:
          result.socket_path = String(data.substr(t, i - t));
          state = ParseState::SharedMemPath;
          break;

        case ParseState::SharedMemPath:
          result.shared_mem_path = String(data.substr(t, i - t));
          state = ParseState::SharedMemSize;
          break;

        case ParseState::SharedMemSize: {
          const char *start = data.data() + t;
          const char *end = data.data() + i;
          if (std::from_chars(start, end, result.shared_mem_size).ec != std::errc{})
          {
            return std::nullopt;
          }
          return result;
        }
        }
        t = i + 1;
      }
      return std::nullopt;
    }
  };

  IpcNode::~IpcNode()
  {
    if (m_socket != INVALID_SOCKET)
    {
      SocketOps::close(m_socket);
    }
  }

  auto IpcNode::connect(const char *connection_string) -> Result<void>
  {
    const Option<IpcConnectionDescriptor> desc_opt = IpcConnectionDescriptor::deserialize(connection_string);
    if (!desc_opt)
    {
      return fail("Failed to parse connection string");
    }
    Ref<IpcConnectionDescriptor> desc = *desc_opt;
    m_shm_name = desc.shared_mem_path;

    m_socket = AU_TRY(SocketOps::create_unix_socket());
    AU_TRY_PURE(SocketOps::connect_unix_socket(m_socket, desc.socket_path.c_str()));

    Mut<u8 *> mapped_ptr = AU_TRY(FileOps::map_shared_memory(desc.shared_mem_path, desc.shared_mem_size, false));
    m_shared_memory = mapped_ptr;

    Mut<IpcSharedMemoryLayout *> layout = reinterpret_cast<IpcSharedMemoryLayout *>(m_shared_memory);

    if (layout->meta.magic != 0x49414950)
    {
      return fail("Invalid shared memory header signature");
    }

    if (layout->meta.version != 1)
    {
      return fail("IPC version mismatch");
    }

    Mut<u8 *> moni_ptr = m_shared_memory + layout->moni_data_offset;
    Mut<u8 *> mino_ptr = m_shared_memory + layout->mino_data_offset;

    m_moni = AU_TRY(RingBufferView::create(&layout->moni_control,
                                           Span<u8>(moni_ptr, static_cast<usize>(layout->moni_data_size)), false));

    m_mino = AU_TRY(RingBufferView::create(&layout->mino_control,
                                           Span<u8>(mino_ptr, static_cast<usize>(layout->mino_data_size)), false));

#if IA_PLATFORM_WINDOWS
    Mut<u_long> mode = 1;
    ioctlsocket(m_socket, FIONBIO, &mode);
#else
    fcntl(m_socket, F_SETFL, O_NONBLOCK);
#endif

    m_receive_buffer.resize(UINT16_MAX + 1);

    return {};
  }

  void IpcNode::update()
  {
    if (!m_moni.is_valid())
    {
      return;
    }

    Mut<IpcPacketHeader> header;

    while (m_moni.pop(header, Span<u8>(m_receive_buffer.data(), m_receive_buffer.size())))
    {
      on_packet(header.id, {m_receive_buffer.data(), header.payload_size});
    }

    Mut<u8> signal = 0;
    const isize res = recv(m_socket, reinterpret_cast<char *>(&signal), 1, 0);
    if (res == 1)
    {
      on_signal(signal);
    }
    else if (res == 0 || (res < 0 && !SocketOps::is_would_block()))
    {
      SocketOps::close(m_socket);
      FileOps::unlink_shared_memory(m_shm_name);

      std::exit(-1);
    }
  }

  void IpcNode::send_signal(const u8 signal)
  {
    if (m_socket != INVALID_SOCKET)
    {
      send(m_socket, reinterpret_cast<const char *>(&signal), sizeof(signal), 0);
    }
  }

  auto IpcNode::send_packet(const u16 packet_id, const Span<const u8> payload) -> Result<void>
  {
    if (!m_mino.is_valid())
      return fail("invalid MINO");
    return m_mino.push(packet_id, payload);
  }

  void IpcManager::NodeSession::send_signal(const u8 signal)
  {
    if (data_socket != INVALID_SOCKET)
    {
      send(data_socket, reinterpret_cast<const char *>(&signal), sizeof(signal), 0);
    }
  }

  auto IpcManager::NodeSession::send_packet(const u16 packet_id, const Span<const u8> payload) -> Result<void>
  {
    const std::scoped_lock<std::mutex> lock(send_mutex);
    if (!moni.is_valid())
      return fail("invalid MONI");
    return moni.push(packet_id, payload);
  }

  IpcManager::IpcManager()
  {
    ensure(SocketOps::is_initialized(), "SocketOps must be initialized before using IpcManager");

    m_receive_buffer.resize(UINT16_MAX + 1);
  }

  IpcManager::~IpcManager()
  {
    for (MutRef<Box<NodeSession>> session : m_active_sessions)
    {
      ProcessOps::terminate_process(session->node_process);
      FileOps::unmap_file(session->mapped_ptr);
      FileOps::unlink_shared_memory(session->shared_mem_name);
      SocketOps::close(session->data_socket);
    }
    m_active_sessions.clear();

    for (MutRef<Box<NodeSession>> session : m_pending_sessions)
    {
      ProcessOps::terminate_process(session->node_process);
      FileOps::unmap_file(session->mapped_ptr);
      FileOps::unlink_shared_memory(session->shared_mem_name);
      SocketOps::close(session->listener_socket);
    }
    m_pending_sessions.clear();
  }

  void IpcManager::update()
  {
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    for (Mut<isize> i = static_cast<isize>(m_pending_sessions.size()) - 1; i >= 0; --i)
    {
      MutRef<Box<NodeSession>> session = m_pending_sessions[static_cast<usize>(i)];

      if (now - session->creation_time > std::chrono::seconds(5))
      {
        ProcessOps::terminate_process(session->node_process);

        FileOps::unmap_file(session->mapped_ptr);
        FileOps::unlink_shared_memory(session->shared_mem_name);
        SocketOps::close(session->listener_socket);

        m_pending_sessions.erase(m_pending_sessions.begin() + i);
        continue;
      }

#if IA_PLATFORM_WINDOWS
      Mut<u64> new_sock = accept(session->listener_socket, nullptr, nullptr);
#else
      Mut<i32> new_sock = accept(session->listener_socket, nullptr, nullptr);
#endif

      if (new_sock != INVALID_SOCKET)
      {
        session->data_socket = new_sock;
        session->is_ready = true;

#if IA_PLATFORM_WINDOWS
        Mut<u_long> mode = 1;
        ioctlsocket(session->data_socket, FIONBIO, &mode);
#else
        fcntl(session->data_socket, F_SETFL, O_NONBLOCK);
#endif

        SocketOps::close(session->listener_socket);
        session->listener_socket = INVALID_SOCKET;

        const NativeProcessID session_id = session->node_process->id.load();
        Mut<NodeSession *> session_ptr = session.get();
        m_active_sessions.push_back(std::move(session));
        m_pending_sessions.erase(m_pending_sessions.begin() + i);
        m_active_session_map[session_id] = session_ptr;
      }
    }

    for (Mut<isize> i = static_cast<isize>(m_active_sessions.size()) - 1; i >= 0; --i)
    {
      MutRef<Box<NodeSession>> node = m_active_sessions[static_cast<usize>(i)];

      Mut<NativeProcessID> node_id = node->node_process->id.load();

      Mut<IpcPacketHeader> header;

      while (node->mino.pop(header, Span<u8>(m_receive_buffer.data(), m_receive_buffer.size())))
      {
        on_packet(node_id, header.id, {m_receive_buffer.data(), header.payload_size});
      }

      Mut<u8> signal = 0;
      const isize res = recv(node->data_socket, reinterpret_cast<char *>(&signal), 1, 0);

      if (res == 1)
      {
        on_signal(node_id, signal);
      }
      else if (res == 0 || (res < 0 && !SocketOps::is_would_block()))
      {
        ProcessOps::terminate_process(node->node_process);

        FileOps::unmap_file(node->mapped_ptr);
        FileOps::unlink_shared_memory(node->shared_mem_name);
        SocketOps::close(node->data_socket);

        m_active_sessions.erase(m_active_sessions.begin() + i);
        m_active_session_map.erase(node_id);
      }
    }
  }

  auto IpcManager::spawn_node(Ref<Path> executable_path, const u32 shared_memory_size) -> Result<NativeProcessID>
  {
    Mut<Box<NodeSession>> session = make_box<NodeSession>();

    static Mut<std::atomic<u32>> s_id_gen{0};
    const u32 sid = ++s_id_gen;

    Mut<String> sock_path;
#if IA_PLATFORM_WINDOWS
    Mut<char[MAX_PATH]> temp_path;
    GetTempPathA(MAX_PATH, temp_path);
    sock_path = std::format("{}\\ia_sess_{}.sock", temp_path, sid);
#else
    sock_path = std::format("/tmp/ia_sess_{}.sock", sid);
#endif

    session->listener_socket = AU_TRY(SocketOps::create_unix_socket());
    AU_TRY_PURE(SocketOps::bind_unix_socket(session->listener_socket, sock_path.c_str()));
    AU_TRY_PURE(SocketOps::listen(session->listener_socket, 1));

#if IA_PLATFORM_WINDOWS
    Mut<u_long> mode = 1;
    ioctlsocket(session->listener_socket, FIONBIO, &mode);
#else
    fcntl(session->listener_socket, F_SETFL, O_NONBLOCK);
#endif

    const String shm_name = std::format("ia_shm_{}", sid);
    session->mapped_ptr = AU_TRY(FileOps::map_shared_memory(shm_name, shared_memory_size, true));

    Mut<IpcSharedMemoryLayout *> layout = reinterpret_cast<IpcSharedMemoryLayout *>(session->mapped_ptr);

    layout->meta.magic = 0x49414950;
    layout->meta.version = 1;
    layout->meta.total_size = shared_memory_size;

    const u64 header_size = IpcSharedMemoryLayout::get_header_size();
    const u64 usable_bytes = shared_memory_size - header_size;

    Mut<u64> half_size = (usable_bytes / 2);
    half_size -= (half_size % 64);

    layout->moni_data_offset = header_size;
    layout->moni_data_size = half_size;

    layout->mino_data_offset = header_size + half_size;
    layout->mino_data_size = half_size;

    session->moni = AU_TRY(RingBufferView::create(
        &layout->moni_control,
        Span<u8>(session->mapped_ptr + layout->moni_data_offset, static_cast<usize>(layout->moni_data_size)), true));

    session->mino = AU_TRY(RingBufferView::create(
        &layout->mino_control,
        Span<u8>(session->mapped_ptr + layout->mino_data_offset, static_cast<usize>(layout->mino_data_size)), true));

    Mut<IpcConnectionDescriptor> desc;
    desc.socket_path = sock_path;
    desc.shared_mem_path = shm_name;
    desc.shared_mem_size = shared_memory_size;

    const String args = std::format("\"{}\"", desc.serialize());

    session->node_process = AU_TRY(ProcessOps::spawn_process_async(
        FileOps::normalize_executable_path(executable_path).string(), args,
        [sid](StringView line) {
          if (Env::IS_DEBUG)
          {
            std::cout << std::format("{}[Node:{}:STDOUT|STDERR]: {}{}\n", console::MAGENTA, sid, line, console::RESET);
          }
        },
        [sid](Result<i32> result) {
          if (Env::IS_DEBUG)
          {
            if (!result)
            {
              std::cout << std::format("{}[Node: {}]: Failed to spawn with error '{}'{}\n", console::RED, sid,
                                       result.error(), console::RESET);
            }
            else
            {
              std::cout << std::format("{}[Node: {}]: Exited with code {}{}\n", console::RED, sid, *result,
                                       console::RESET);
            }
          }
        }));

    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (!session->node_process->is_active())
    {
      return fail("Failed to spawn the child process \"{}\"", executable_path.string());
    }

    const NativeProcessID process_id = session->node_process->id.load();

    session->shared_mem_name = shm_name;
    session->creation_time = std::chrono::system_clock::now();
    m_pending_sessions.push_back(std::move(session));

    return process_id;
  }

  auto IpcManager::wait_till_node_is_online(const NativeProcessID node_id) -> bool
  {
    Mut<bool> is_pending = true;
    while (is_pending)
    {
      is_pending = false;
      for (const Box<NodeSession> &session : m_pending_sessions)
      {
        if (session->node_process->id.load() == node_id)
        {
          is_pending = true;
          break;
        }
      }
      update();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return m_active_session_map.contains(node_id);
  }

  void IpcManager::shutdown_node(const NativeProcessID node_id)
  {
    const HashMap<NativeProcessID, NodeSession *>::iterator it_node = m_active_session_map.find(node_id);
    if (it_node == m_active_session_map.end())
    {
      return;
    }

    Mut<NodeSession *> node = it_node->second;

    ProcessOps::terminate_process(node->node_process);
    FileOps::unmap_file(node->mapped_ptr);
    FileOps::unlink_shared_memory(node->shared_mem_name);
    SocketOps::close(node->data_socket);

    std::erase_if(m_active_sessions, [&](Ref<Box<NodeSession>> s) { return s.get() == node; });
    m_active_session_map.erase(it_node);
  }

  void IpcManager::send_signal(const NativeProcessID node, const u8 signal)
  {
    const HashMap<NativeProcessID, NodeSession *>::iterator it_node = m_active_session_map.find(node);
    if (it_node == m_active_session_map.end())
    {
      return;
    }
    it_node->second->send_signal(signal);
  }

  auto IpcManager::send_packet(const NativeProcessID node, const u16 packet_id, const Span<const u8> payload)
      -> Result<void>
  {
    const HashMap<NativeProcessID, NodeSession *>::iterator it_node = m_active_session_map.find(node);
    if (it_node == m_active_session_map.end())
      return fail("no such node");
    return it_node->second->send_packet(packet_id, payload);
  }
} // namespace IACore