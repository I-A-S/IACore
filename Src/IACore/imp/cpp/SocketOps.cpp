// IACore-OSS; The Core Library for All IA Open Source Projects
// Copyright (C) 2025 IAS (ias@iasoft.dev)
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
    INT32 SocketOps::s_initCount{0};

    VOID SocketOps::Close(IN SocketHandle sock)
    {
        if (sock == INVALID_SOCKET)
            return;
        CLOSE_SOCKET(sock);
    }

    BOOL SocketOps::Listen(IN SocketHandle sock, IN INT32 queueSize)
    {
        return listen(sock, queueSize) == 0;
    }

    SocketHandle SocketOps::CreateUnixSocket()
    {
        return socket(AF_UNIX, SOCK_STREAM, 0);
    }

    BOOL SocketOps::BindUnixSocket(IN SocketHandle sock, IN PCCHAR path)
    {
        if (!IS_VALID_SOCKET(sock))
            return FALSE;

        UNLINK_FILE(path);

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;

        size_t maxLen = sizeof(addr.sun_path) - 1;

        strncpy(addr.sun_path, path, maxLen);

        if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
            return FALSE;

        return TRUE;
    }

    BOOL SocketOps::ConnectUnixSocket(IN SocketHandle sock, IN PCCHAR path)
    {
        if (!IS_VALID_SOCKET(sock))
            return FALSE;

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;

        size_t maxLen = sizeof(addr.sun_path) - 1;

        strncpy(addr.sun_path, path, maxLen);

        if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
            return FALSE;

        return TRUE;
    }

    BOOL SocketOps::IsPortAvailable(IN UINT16 port, IN INT32 type)
    {
        SocketHandle sock = socket(AF_INET, type, IPPROTO_UDP);
        if (!IS_VALID_SOCKET(sock))
            return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        bool isFree = false;
        if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == 0)
            isFree = true;

        CLOSE_SOCKET(sock);

        return isFree;
    }

    BOOL SocketOps::IsWouldBlock()
    {
#if IA_PLATFORM_WINDOWS
        return WSAGetLastError() == WSAEWOULDBLOCK;
#else
        return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
    }
} // namespace IACore