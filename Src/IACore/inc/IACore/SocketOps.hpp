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

#pragma once

#include <IACore/PCH.hpp>

#if IA_PLATFORM_WINDOWS

#    include <winsock2.h>
#    include <ws2tcpip.h>
#    include <afunix.h>
#    pragma comment(lib, "ws2_32.lib")
#    define CLOSE_SOCKET(s) closesocket(s)
#    define IS_VALID_SOCKET(s) (s != INVALID_SOCKET)
#    define UNLINK_FILE(p) DeleteFileA(p)
using SocketHandle = SOCKET;

#elif IA_PLATFORM_UNIX

#    include <sys/un.h>
#    include <sys/types.h>
#    include <sys/socket.h>
#    include <netinet/in.h>
#    define CLOSE_SOCKET(s) close(s)
#    define IS_VALID_SOCKET(s) (s >= 0)
#    define INVALID_SOCKET -1
#    define UNLINK_FILE(p) unlink(p)
using SocketHandle = int;

#else

#    error "IACore SocketOps is not supported on this platform."

#endif

namespace IACore
{
    class SocketOps
    {
      public:
        // SocketOps correctly handles multiple calls to Initialize and Terminate. Make sure
        // every Initialize call is paired with a corresponding Terminate call
        STATIC VOID Initialize()
        {
            s_initCount++;
            if (s_initCount > 1)
                return;
#if IA_PLATFORM_WINDOWS
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
        }

        // SocketOps correctly handles multiple calls to Initialize and Terminate. Make sure
        // every Initialize call is paired with a corresponding Terminate call
        STATIC VOID Terminate()
        {
            s_initCount--;
            if (s_initCount > 0)
                return;
#if IA_PLATFORM_WINDOWS
            WSACleanup();
#endif
        }

        STATIC BOOL IsPortAvailableTCP(IN UINT16 port)
        {
            return IsPortAvailable(port, SOCK_STREAM);
        }

        STATIC BOOL IsPortAvailableUDP(IN UINT16 port)
        {
            return IsPortAvailable(port, SOCK_DGRAM);
        }

        STATIC BOOL IsWouldBlock();

        STATIC VOID Close(IN SocketHandle sock);

        STATIC BOOL Listen(IN SocketHandle sock, IN INT32 queueSize = 5);

        STATIC SocketHandle CreateUnixSocket();

        STATIC BOOL BindUnixSocket(IN SocketHandle sock, IN PCCHAR path);
        STATIC BOOL ConnectUnixSocket(IN SocketHandle sock, IN PCCHAR path);

      private:
        STATIC BOOL IsPortAvailable(IN UINT16 port, IN INT32 type);

      private:
        STATIC INT32 s_initCount;
    };
} // namespace IACore