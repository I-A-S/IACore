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

#if IA_PLATFORM_WINDOWS
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <afunix.h>
#  pragma comment(lib, "ws2_32.lib")
#elif IA_PLATFORM_UNIX
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <sys/un.h>
#  ifndef INVALID_SOCKET
#    define INVALID_SOCKET -1
#  endif
#else
#  error "IACore SocketOps is not supported on this platform."
#endif

namespace IACore
{
#if IA_PLATFORM_WINDOWS
  using SocketHandle = SOCKET;
#elif IA_PLATFORM_UNIX
  using SocketHandle = i32;
#endif

  class SocketOps
  {
public:
    // SocketOps correctly handles multiple calls to initialize and terminate.
    // Make sure every initialize call is paired with a corresponding terminate
    // call.
    static auto initialize() -> Result<void>
    {
      s_init_count++;
      if (s_init_count > 1)
      {
        return {};
      }
#if IA_PLATFORM_WINDOWS
      Mut<WSADATA> wsa_data;
      const i32 res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
      if (res != 0)
      {
        s_init_count--;
        return fail("WSAStartup failed with error: {}", res);
      }
#endif
      return {};
    }

    // SocketOps correctly handles multiple calls to initialize and terminate.
    // Make sure every initialize call is paired with a corresponding terminate
    // call.
    static auto terminate() -> void
    {
      s_init_count--;
      if (s_init_count > 0)
      {
        return;
      }
#if IA_PLATFORM_WINDOWS
      WSACleanup();
#endif
    }

    static auto is_initialized() -> bool
    {
      return s_init_count > 0;
    }

    static auto is_port_available_tcp(const u16 port) -> bool
    {
      return is_port_available(port, SOCK_STREAM);
    }

    static auto is_port_available_udp(const u16 port) -> bool
    {
      return is_port_available(port, SOCK_DGRAM);
    }

    static auto is_would_block() -> bool;

    static auto close(const SocketHandle sock) -> void;

    static auto listen(const SocketHandle sock, const i32 queue_size = 5) -> Result<void>;

    static auto create_unix_socket() -> Result<SocketHandle>;

    static auto bind_unix_socket(const SocketHandle sock, const char *path) -> Result<void>;
    static auto connect_unix_socket(const SocketHandle sock, const char *path) -> Result<void>;

    static auto unlink_file(const char *path) -> void
    {
#if IA_PLATFORM_WINDOWS
      DeleteFileA(path);
#elif IA_PLATFORM_UNIX
      unlink(path);
#endif
    }

private:
    static auto is_port_available(const u16 port, const i32 type) -> bool;

private:
    static Mut<i32> s_init_count;
  };
} // namespace IACore