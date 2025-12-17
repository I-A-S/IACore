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

#include <IACore/IPC.hpp>

#include <IACore/FileOps.hpp>
#include <IACore/StringOps.hpp>

namespace IACore
{
    struct IPC_ConnectionDescriptor
    {
        String SocketPath;
        String SharedMemPath;
        UINT32 SharedMemSize;

        String Serialize() CONST
        {
            return std::format("{}|{}|{}|", SocketPath, SharedMemPath, SharedMemSize);
        }

        STATIC IPC_ConnectionDescriptor Deserialize(IN CONST String &data)
        {
            enum class EParseState
            {
                SocketPath,
                SharedMemPath,
                SharedMemSize
            };

            IPC_ConnectionDescriptor result{};

            SIZE_T t{};
            EParseState state{EParseState::SocketPath};
            for (SIZE_T i = 0; i < data.size(); i++)
            {
                if (data[i] != '|')
                    continue;

                switch (state)
                {
                case EParseState::SocketPath:
                    result.SocketPath = data.substr(t, i - t);
                    state = EParseState::SharedMemPath;
                    break;

                case EParseState::SharedMemPath:
                    result.SharedMemPath = data.substr(t, i - t);
                    state = EParseState::SharedMemSize;
                    break;

                case EParseState::SharedMemSize: {
                    if (std::from_chars(&data[t], &data[i], result.SharedMemSize).ec != std::errc{})
                        return {};
                    goto done_parsing;
                }
                }
                t = i + 1;
            }

        done_parsing:
            return result;
        }
    };
} // namespace IACore

namespace IACore
{
    IPC_Node::~IPC_Node()
    {
        SocketOps::Close(m_socket); // SocketOps gracefully handles INVALID_SOCKET
    }

    Expected<VOID, String> IPC_Node::Connect(IN PCCHAR connectionString)
    {
        auto desc = IPC_ConnectionDescriptor::Deserialize(connectionString);
        m_shmName = desc.SharedMemPath;

        m_socket = SocketOps::CreateUnixSocket();
        if (!SocketOps::ConnectUnixSocket(m_socket, desc.SocketPath.c_str()))
            return MakeUnexpected("Failed to create an unix socket");

        auto mapRes = FileOps::MapSharedMemory(desc.SharedMemPath, desc.SharedMemSize, FALSE);
        if (!mapRes.has_value())
            return MakeUnexpected("Failed to map the shared memory");

        m_sharedMemory = mapRes.value();

        auto *layout = reinterpret_cast<IPC_SharedMemoryLayout *>(m_sharedMemory);

        if (layout->Meta.Magic != 0x49414950) // "IAIP"
            return MakeUnexpected("Invalid shared memory header signature");

        if (layout->Meta.Version != 1)
            return MakeUnexpected("IPC version mismatch");

        PUINT8 moniDataPtr = m_sharedMemory + layout->MONI_DataOffset;
        PUINT8 minoDataPtr = m_sharedMemory + layout->MINO_DataOffset;

        MONI = std::make_unique<RingBufferView>(
            &layout->MONI_Control, Span<UINT8>(moniDataPtr, static_cast<size_t>(layout->MONI_DataSize)), FALSE);

        MINO = std::make_unique<RingBufferView>(
            &layout->MINO_Control, Span<UINT8>(minoDataPtr, static_cast<size_t>(layout->MINO_DataSize)), FALSE);

#if IA_PLATFORM_WINDOWS
        u_long mode = 1;
        ioctlsocket(m_socket, FIONBIO, &mode);
#else
        fcntl(m_socket, F_SETFL, O_NONBLOCK);
#endif

        m_receiveBuffer.resize(UINT16_MAX + 1);

        return {};
    }

    VOID IPC_Node::Update()
    {
        if (!MONI)
            return;

        RingBufferView::PacketHeader header;

        // Process all available messages from Manager
        while (MONI->Pop(header, Span<UINT8>(m_receiveBuffer.data(), m_receiveBuffer.size())))
            OnPacket(header.ID, {m_receiveBuffer.data(), header.PayloadSize});

        UINT8 signal;
        const auto res = recv(m_socket, (CHAR *) &signal, 1, 0);
        if (res == 1)
            OnSignal(signal);
        else if (res == 0 || (res < 0 && !SocketOps::IsWouldBlock()))
        {
            SocketOps::Close(m_socket);
            FileOps::UnlinkSharedMemory(m_shmName);

            // Manager disconnected, exit immediately
            exit(-1);
        }
    }

    VOID IPC_Node::SendSignal(IN UINT8 signal)
    {
        if (IS_VALID_SOCKET(m_socket))
            send(m_socket, (const char *) &signal, sizeof(signal), 0);
    }

    VOID IPC_Node::SendPacket(IN UINT16 packetID, IN Span<CONST UINT8> payload)
    {
        MINO->Push(packetID, payload);
    }
} // namespace IACore

namespace IACore
{
    VOID IPC_Manager::NodeSession::SendSignal(IN UINT8 signal)
    {
        if (IS_VALID_SOCKET(DataSocket))
            send(DataSocket, (const char *) &signal, sizeof(signal), 0);
    }

    VOID IPC_Manager::NodeSession::SendPacket(IN UINT16 packetID, IN Span<CONST UINT8> payload)
    {
        // Protect the RingBuffer write cursor from concurrent threads
        ScopedLock lock(SendMutex);
        MONI->Push(packetID, payload);
    }

    IPC_Manager::IPC_Manager()
    {
        // SocketOps is smart enough to track multiple inits
        SocketOps::Initialize();

        m_receiveBuffer.resize(UINT16_MAX + 1);
    }

    IPC_Manager::~IPC_Manager()
    {
        for (auto &session : m_activeSessions)
        {
            ProcessOps::TerminateProcess(session->ProcessHandle);
            FileOps::UnmapFile(session->MappedPtr);
            FileOps::UnlinkSharedMemory(session->SharedMemName);
            SocketOps::Close(session->DataSocket);
        }
        m_activeSessions.clear();

        for (auto &session : m_pendingSessions)
        {
            ProcessOps::TerminateProcess(session->ProcessHandle);
            FileOps::UnmapFile(session->MappedPtr);
            FileOps::UnlinkSharedMemory(session->SharedMemName);
            SocketOps::Close(session->ListenerSocket);
        }
        m_pendingSessions.clear();

        // SocketOps is smart enough to track multiple terminates
        SocketOps::Terminate();
    }

    VOID IPC_Manager::Update()
    {
        const auto now = SteadyClock::now();

        for (INT32 i = m_pendingSessions.size() - 1; i >= 0; i--)
        {
            auto &session = m_pendingSessions[i];

            if (now - session->CreationTime > std::chrono::seconds(5))
            {
                ProcessOps::TerminateProcess(session->ProcessHandle);

                FileOps::UnmapFile(session->MappedPtr);
                FileOps::UnlinkSharedMemory(session->SharedMemName);
                SocketOps::Close(session->DataSocket);

                m_pendingSessions.erase(m_pendingSessions.begin() + i);
                continue;
            }

            SocketHandle newSock = accept(session->ListenerSocket, NULL, NULL);

            if (IS_VALID_SOCKET(newSock))
            {
                session->DataSocket = newSock;
                session->IsReady = TRUE;

                // Set Data Socket to Non-Blocking
#if IA_PLATFORM_WINDOWS
                u_long mode = 1;
                ioctlsocket(session->DataSocket, FIONBIO, &mode);
#else
                fcntl(session->DataSocket, F_SETFL, O_NONBLOCK);
#endif

                SocketOps::Close(session->ListenerSocket);
                session->ListenerSocket = INVALID_SOCKET;

                const auto sessionID = session->ProcessHandle->ID.load();
                const auto sessionPtr = session.get();
                m_activeSessions.push_back(std::move(session));
                m_pendingSessions.erase(m_pendingSessions.begin() + i);
                m_activeSessionMap[sessionID] = sessionPtr;
            }
        }

        for (INT32 i = m_activeSessions.size() - 1; i >= 0; i--)
        {
            auto &node = m_activeSessions[i];

            auto nodeID = node->ProcessHandle->ID.load();

            RingBufferView::PacketHeader header;

            while (node->MINO->Pop(header, Span<UINT8>(m_receiveBuffer.data(), m_receiveBuffer.size())))
                OnPacket(nodeID, header.ID, {m_receiveBuffer.data(), header.PayloadSize});

            UINT8 signal;
            const auto res = recv(node->DataSocket, (CHAR *) &signal, 1, 0);

            if (res == 1)
                OnSignal(nodeID, signal);
            else if (res == 0 || (res < 0 && !SocketOps::IsWouldBlock()))
            {
                ProcessOps::TerminateProcess(node->ProcessHandle);

                FileOps::UnmapFile(node->MappedPtr);
                FileOps::UnlinkSharedMemory(node->SharedMemName);
                SocketOps::Close(node->DataSocket);

                m_activeSessions.erase(m_activeSessions.begin() + i);
                m_activeSessionMap.erase(nodeID);
            }
        }
    }

    Expected<NativeProcessID, String> IPC_Manager::SpawnNode(IN CONST FilePath &executablePath,
                                                             IN UINT32 sharedMemorySize)
    {
        auto session = std::make_unique<NodeSession>();

        static Atomic<UINT32> s_idGen{0};
        UINT32 sid = ++s_idGen;

#if IA_PLATFORM_WINDOWS
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        String sockPath = std::format("{}\\ia_sess_{}.sock", tempPath, sid);
#else
        String sockPath = std::format("/tmp/ia_sess_{}.sock", sid);
#endif

        session->ListenerSocket = SocketOps::CreateUnixSocket();
        if (!SocketOps::BindUnixSocket(session->ListenerSocket, sockPath.c_str()))
            return MakeUnexpected("Failed to bind unique socket");

        if (listen(session->ListenerSocket, 1) != 0)
            return MakeUnexpected("Failed to listen on unique socket");

#if IA_PLATFORM_WINDOWS
        u_long mode = 1;
        ioctlsocket(session->ListenerSocket, FIONBIO, &mode);
#else
        fcntl(session->ListenerSocket, F_SETFL, O_NONBLOCK);
#endif

        String shmName = std::format("ia_shm_{}", sid);
        auto mapRes = FileOps::MapSharedMemory(shmName, sharedMemorySize, TRUE);
        if (!mapRes.has_value())
            return MakeUnexpected("Failed to map shared memory");

        PUINT8 mappedPtr = mapRes.value();

        auto *layout = reinterpret_cast<IPC_SharedMemoryLayout *>(mappedPtr);

        layout->Meta.Magic = 0x49414950;
        layout->Meta.Version = 1;
        layout->Meta.TotalSize = sharedMemorySize;

        UINT64 headerSize = IPC_SharedMemoryLayout::GetHeaderSize();
        UINT64 usableBytes = sharedMemorySize - headerSize;

        UINT64 halfSize = (usableBytes / 2);
        halfSize -= (halfSize % 64);

        layout->MONI_DataOffset = headerSize;
        layout->MONI_DataSize = halfSize;

        layout->MINO_DataOffset = headerSize + halfSize;
        layout->MINO_DataSize = halfSize;

        session->MONI = std::make_unique<RingBufferView>(
            &layout->MONI_Control, Span<UINT8>(mappedPtr + layout->MONI_DataOffset, layout->MONI_DataSize), TRUE);

        session->MINO = std::make_unique<RingBufferView>(
            &layout->MINO_Control, Span<UINT8>(mappedPtr + layout->MINO_DataOffset, layout->MINO_DataSize), TRUE);

        IPC_ConnectionDescriptor desc;
        desc.SocketPath = sockPath;
        desc.SharedMemPath = shmName;
        desc.SharedMemSize = sharedMemorySize;

        String args = std::format("\"{}\"", desc.Serialize());

        session->ProcessHandle = ProcessOps::SpawnProcessAsync(
            FileOps::NormalizeExecutablePath(executablePath).string(), args,
            [sid](IN StringView line) {
                UNUSED(sid);
                UNUSED(line);
#if __IA_DEBUG
                puts(std::format(__CC_MAGENTA "[Node:{}:STDOUT|STDERR]: {}" __CC_DEFAULT, sid, line).c_str());
#endif
            },
            [sid](IN Expected<INT32, String> result) {
                UNUSED(sid);
                UNUSED(result);
#if __IA_DEBUG
                if (!result)
                    puts(std::format(__CC_RED "Failed to spawn Node: {} with error '{}'" __CC_DEFAULT, sid,
                                     result.error())
                             .c_str());
                else
                    puts(std::format(__CC_RED "[Node: {}]: Exited with code {}" __CC_DEFAULT, sid, *result).c_str());
#endif
            });

        // Give some time for child node to stablize
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!session->ProcessHandle->IsActive())
            return MakeUnexpected(std::format("Failed to spawn the child process \"{}\"", executablePath.string()));

        auto processID = session->ProcessHandle->ID.load();

        session->CreationTime = SteadyClock::now();
        m_pendingSessions.push_back(std::move(session));

        return processID;
    }

    BOOL IPC_Manager::WaitTillNodeIsOnline(IN NativeProcessID nodeID)
    {
        BOOL isPending = true;
        while (isPending)
        {
            isPending = false;
            for (auto it = m_pendingSessions.begin(); it != m_pendingSessions.end(); it++)
            {
                if (it->get()->ProcessHandle->ID.load() == nodeID)
                {
                    isPending = true;
                    break;
                }
            }
            Update();
        }
        return m_activeSessionMap.contains(nodeID);
    }

    VOID IPC_Manager::ShutdownNode(IN NativeProcessID nodeID)
    {
        const auto itNode = m_activeSessionMap.find(nodeID);
        if (itNode == m_activeSessionMap.end())
            return;

        auto &node = itNode->second;

        ProcessOps::TerminateProcess(node->ProcessHandle);
        FileOps::UnmapFile(node->MappedPtr);
        FileOps::UnlinkSharedMemory(node->SharedMemName);
        SocketOps::Close(node->DataSocket);

        for (auto it = m_activeSessions.begin(); it != m_activeSessions.end(); it++)
        {
            if (it->get() == node)
            {
                m_activeSessions.erase(it);
                break;
            }
        }

        m_activeSessionMap.erase(itNode);
    }

    VOID IPC_Manager::SendSignal(IN NativeProcessID node, IN UINT8 signal)
    {
        const auto itNode = m_activeSessionMap.find(node);
        if (itNode == m_activeSessionMap.end())
            return;
        itNode->second->SendSignal(signal);
    }

    VOID IPC_Manager::SendPacket(IN NativeProcessID node, IN UINT16 packetID, IN Span<CONST UINT8> payload)
    {
        const auto itNode = m_activeSessionMap.find(node);
        if (itNode == m_activeSessionMap.end())
            return;
        itNode->second->SendPacket(packetID, payload);
    }
} // namespace IACore