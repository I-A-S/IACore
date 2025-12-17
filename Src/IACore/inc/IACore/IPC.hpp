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

#include <IACore/ADT/RingBuffer.hpp>
#include <IACore/ProcessOps.hpp>
#include <IACore/SocketOps.hpp>

namespace IACore
{
    using IPC_PacketHeader = RingBufferView::PacketHeader;

    struct alignas(64) IPC_SharedMemoryLayout
    {
        // =========================================================
        // SECTION 1: METADATA & HANDSHAKE
        // =========================================================
        struct Header
        {
            UINT32 Magic;     // 0x49414950 ("IAIP")
            UINT32 Version;   // 1
            UINT64 TotalSize; // Total size of SHM block
        } Meta;

        // Pad to ensure MONI starts on a fresh cache line (64 bytes)
        UINT8 _pad0[64 - sizeof(Header)];

        // =========================================================
        // SECTION 2: RING BUFFER CONTROL BLOCKS
        // =========================================================

        // RingBufferView ControlBlock is already 64-byte aligned internally.
        RingBufferView::ControlBlock MONI_Control;
        RingBufferView::ControlBlock MINO_Control;

        // =========================================================
        // SECTION 3: DATA BUFFER OFFSETS
        // =========================================================

        UINT64 MONI_DataOffset;
        UINT64 MONI_DataSize;

        UINT64 MINO_DataOffset;
        UINT64 MINO_DataSize;

        // Pad to ensure the actual Data Buffer starts on a fresh cache line
        UINT8 _pad1[64 - (sizeof(UINT64) * 4)];

        static constexpr size_t GetHeaderSize()
        {
            return sizeof(IPC_SharedMemoryLayout);
        }
    };

    // Static assert to ensure manual padding logic is correct
    static_assert(sizeof(IPC_SharedMemoryLayout) % 64 == 0, "IPC Layout is not cache-line aligned!");

    class IPC_Node
    {
      public:
        virtual ~IPC_Node();

        // When Manager spawns a node, `connectionString` is passed
        // as the first command line argument
        Expected<VOID, String> Connect(IN PCCHAR connectionString);

        VOID Update();

        VOID SendSignal(IN UINT8 signal);
        VOID SendPacket(IN UINT16 packetID, IN Span<CONST UINT8> payload);

      protected:
        PURE_VIRTUAL(VOID OnSignal(IN UINT8 signal));
        PURE_VIRTUAL(VOID OnPacket(IN UINT16 packetID, IN Span<CONST UINT8> payload));

      private:
        String m_shmName;
        PUINT8 m_sharedMemory{};
        Vector<UINT8> m_receiveBuffer;
        SocketHandle m_socket{INVALID_SOCKET};

        UniquePtr<RingBufferView> MONI; // Manager Out, Node In
        UniquePtr<RingBufferView> MINO; // Manager In, Node Out
    };

    class IPC_Manager
    {
        struct NodeSession
        {
            SteadyTimePoint CreationTime{};
            SharedPtr<ProcessHandle> ProcessHandle;

            Mutex SendMutex;

            String SharedMemName;
            PUINT8 MappedPtr{};

            SocketHandle ListenerSocket{INVALID_SOCKET};
            SocketHandle DataSocket{INVALID_SOCKET};

            UniquePtr<RingBufferView> MONI; // Manager Out, Node In
            UniquePtr<RingBufferView> MINO; // Manager In, Node Out

            BOOL IsReady{FALSE};

            VOID SendSignal(IN UINT8 signal);
            VOID SendPacket(IN UINT16 packetID, IN Span<CONST UINT8> payload);
        };

      public:
        STATIC CONSTEXPR UINT32 DEFAULT_NODE_SHARED_MEMORY_SIZE = SIZE_MB(4);

      public:
        IPC_Manager();
        virtual ~IPC_Manager();

        VOID Update();

        Expected<NativeProcessID, String> SpawnNode(IN CONST FilePath &executablePath,
                                                    IN UINT32 sharedMemorySize = DEFAULT_NODE_SHARED_MEMORY_SIZE);
        BOOL WaitTillNodeIsOnline(IN NativeProcessID node);

        VOID ShutdownNode(IN NativeProcessID node);

        VOID SendSignal(IN NativeProcessID node, IN UINT8 signal);
        VOID SendPacket(IN NativeProcessID node, IN UINT16 packetID, IN Span<CONST UINT8> payload);

      protected:
        PURE_VIRTUAL(VOID OnSignal(IN NativeProcessID node, IN UINT8 signal));
        PURE_VIRTUAL(VOID OnPacket(IN NativeProcessID node, IN UINT16 packetID, IN Span<CONST UINT8> payload));

      private:
        Vector<UINT8> m_receiveBuffer;
        Vector<UniquePtr<NodeSession>> m_activeSessions;
        Vector<UniquePtr<NodeSession>> m_pendingSessions;
        UnorderedMap<NativeProcessID, NodeSession *> m_activeSessionMap;
    };
} // namespace IACore