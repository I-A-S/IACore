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

#include <IACore/SocketOps.hpp>

namespace IACore
{
  Mut<i32> SocketOps::s_init_count = 0;

  auto SocketOps::close(const SocketHandle sock) -> void
  {
    if (sock == INVALID_SOCKET)
    {
      return;
    }
#if IA_PLATFORM_WINDOWS
    closesocket(sock);
#else
    ::close(sock);
#endif
  }

  auto SocketOps::listen(const SocketHandle sock, const i32 queue_size) -> Result<void>
  {
    if (::listen(sock, queue_size) == 0)
    {
      return {};
    }

#if IA_PLATFORM_WINDOWS
    return fail("listen failed: {}", WSAGetLastError());
#else
    return fail("listen failed: {}", errno);
#endif
  }

  auto SocketOps::create_unix_socket() -> Result<SocketHandle>
  {
    const SocketHandle sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
#if IA_PLATFORM_WINDOWS
      return fail("socket(AF_UNIX) failed: {}", WSAGetLastError());
#else
      return fail("socket(AF_UNIX) failed: {}", errno);
#endif
    }
    return sock;
  }

  auto SocketOps::bind_unix_socket(const SocketHandle sock, const char *path) -> Result<void>
  {
    if (sock == INVALID_SOCKET)
    {
      return fail("Invalid socket handle");
    }

    unlink_file(path);

    Mut<sockaddr_un> addr{};
    addr.sun_family = AF_UNIX;

    const usize max_len = sizeof(addr.sun_path) - 1;
#if IA_PLATFORM_WINDOWS
    strncpy_s(addr.sun_path, sizeof(addr.sun_path), path, max_len);
#else
    std::strncpy(addr.sun_path, path, max_len);
#endif

    if (::bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1)
    {
#if IA_PLATFORM_WINDOWS
      return fail("bind failed: {}", WSAGetLastError());
#else
      return fail("bind failed: {}", errno);
#endif
    }

    return {};
  }

  auto SocketOps::connect_unix_socket(const SocketHandle sock, const char *path) -> Result<void>
  {
    if (sock == INVALID_SOCKET)
    {
      return fail("Invalid socket handle");
    }

    Mut<sockaddr_un> addr{};
    addr.sun_family = AF_UNIX;

    const usize max_len = sizeof(addr.sun_path) - 1;
#if IA_PLATFORM_WINDOWS
    strncpy_s(addr.sun_path, sizeof(addr.sun_path), path, max_len);
#else
    std::strncpy(addr.sun_path, path, max_len);
#endif

    if (::connect(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1)
    {
#if IA_PLATFORM_WINDOWS
      return fail("connect failed: {}", WSAGetLastError());
#else
      return fail("connect failed: {}", errno);
#endif
    }

    return {};
  }

  auto SocketOps::is_port_available(const u16 port, const i32 type) -> bool
  {
    const SocketHandle sock = socket(AF_INET, type, 0);
    if (sock == INVALID_SOCKET)
    {
      return false;
    }

    Mut<sockaddr_in> addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    Mut<bool> is_free = false;
    if (::bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0)
    {
      is_free = true;
    }

    close(sock);

    return is_free;
  }

  auto SocketOps::is_would_block() -> bool
  {
#if IA_PLATFORM_WINDOWS
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
  }

} // namespace IACore