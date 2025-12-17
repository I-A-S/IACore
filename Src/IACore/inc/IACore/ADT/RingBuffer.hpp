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

namespace IACore
{
    class RingBufferView
    {
      public:
        STATIC CONSTEXPR UINT16 PACKET_ID_SKIP = 0;

        struct ControlBlock
        {
            struct alignas(64)
            {
                Atomic<UINT32> WriteOffset{0};
            } Producer;

            struct alignas(64)
            {
                Atomic<UINT32> ReadOffset{0};
                // Capacity is effectively constant after init,
                // so it doesn't cause false sharing invalidations.
                UINT32 Capacity{0};
            } Consumer;
        };

        static_assert(offsetof(ControlBlock, Consumer) == 64, "False sharing detected in ControlBlock");

        // All of the data in ring buffer will be stored as packets
        struct PacketHeader
        {
            PacketHeader() : ID(0), PayloadSize(0)
            {
            }

            PacketHeader(IN UINT16 id) : ID(id), PayloadSize(0)
            {
            }

            PacketHeader(IN UINT16 id, IN UINT16 payloadSize) : ID(id), PayloadSize(payloadSize)
            {
            }

            UINT16 ID{};
            UINT16 PayloadSize{};
        };

      public:
        INLINE RingBufferView(IN Span<UINT8> buffer, IN BOOL isOwner);
        INLINE RingBufferView(IN ControlBlock *controlBlock, IN Span<UINT8> buffer, IN BOOL isOwner);

        INLINE INT32 Pop(OUT PacketHeader &outHeader, OUT Span<UINT8> outBuffer);
        INLINE BOOL Push(IN UINT16 packetID, IN Span<CONST UINT8> data);

        INLINE ControlBlock *GetControlBlock();

      private:
        PUINT8 m_dataPtr{};
        UINT32 m_capacity{};
        ControlBlock *m_controlBlock{};

      private:
        INLINE VOID WriteWrapped(IN UINT32 offset, IN PCVOID data, IN UINT32 size);
        INLINE VOID ReadWrapped(IN UINT32 offset, OUT PVOID outData, IN UINT32 size);
    };
} // namespace IACore

namespace IACore
{
    RingBufferView::RingBufferView(IN Span<UINT8> buffer, IN BOOL isOwner)
    {
        IA_ASSERT(buffer.size() > sizeof(ControlBlock));

        m_controlBlock = reinterpret_cast<ControlBlock *>(buffer.data());
        m_dataPtr = buffer.data() + sizeof(ControlBlock);

        m_capacity = static_cast<UINT32>(buffer.size()) - sizeof(ControlBlock);

        if (isOwner)
        {
            m_controlBlock->Consumer.Capacity = m_capacity;
            m_controlBlock->Producer.WriteOffset.store(0, std::memory_order_release);
            m_controlBlock->Consumer.ReadOffset.store(0, std::memory_order_release);
        }
        else
            IA_ASSERT(m_controlBlock->Consumer.Capacity == m_capacity);
    }

    RingBufferView::RingBufferView(IN ControlBlock *controlBlock, IN Span<UINT8> buffer, IN BOOL isOwner)
    {
        IA_ASSERT(controlBlock != nullptr);
        IA_ASSERT(buffer.size() > 0);

        m_controlBlock = controlBlock;
        m_dataPtr = buffer.data();
        m_capacity = static_cast<UINT32>(buffer.size());

        if (isOwner)
        {
            m_controlBlock->Consumer.Capacity = m_capacity;
            m_controlBlock->Producer.WriteOffset.store(0, std::memory_order_release);
            m_controlBlock->Consumer.ReadOffset.store(0, std::memory_order_release);
        }
    }

    INT32 RingBufferView::Pop(OUT PacketHeader &outHeader, OUT Span<UINT8> outBuffer)
    {
        UINT32 write = m_controlBlock->Producer.WriteOffset.load(std::memory_order_acquire);
        UINT32 read = m_controlBlock->Consumer.ReadOffset.load(std::memory_order_relaxed);
        UINT32 cap = m_capacity;

        if (read == write)
            return 0; // Empty

        ReadWrapped(read, &outHeader, sizeof(PacketHeader));

        if (outHeader.PayloadSize > outBuffer.size())
            return -static_cast<INT32>(outHeader.PayloadSize);

        if (outHeader.PayloadSize > 0)
        {
            UINT32 dataReadOffset = (read + sizeof(PacketHeader)) % cap;
            ReadWrapped(dataReadOffset, outBuffer.data(), outHeader.PayloadSize);
        }

        // Move read pointer forward
        UINT32 newReadOffset = (read + sizeof(PacketHeader) + outHeader.PayloadSize) % cap;
        m_controlBlock->Consumer.ReadOffset.store(newReadOffset, std::memory_order_release);

        return outHeader.PayloadSize;
    }

    BOOL RingBufferView::Push(IN UINT16 packetID, IN Span<CONST UINT8> data)
    {
        IA_ASSERT(data.size() <= UINT16_MAX);

        const UINT32 totalSize = sizeof(PacketHeader) + static_cast<UINT32>(data.size());

        UINT32 read = m_controlBlock->Consumer.ReadOffset.load(std::memory_order_acquire);
        UINT32 write = m_controlBlock->Producer.WriteOffset.load(std::memory_order_relaxed);
        UINT32 cap = m_capacity;

        UINT32 freeSpace = (read <= write) ? (m_capacity - write) + read : (read - write);

        // Ensure to always leave 1 byte empty to prevent Read == Write ambiguity (Wait-Free Ring Buffer standard)
        if (freeSpace <= totalSize)
            return FALSE;

        PacketHeader header{packetID, static_cast<UINT16>(data.size())};
        WriteWrapped(write, &header, sizeof(PacketHeader));

        UINT32 dataWriteOffset = (write + sizeof(PacketHeader)) % cap;

        if (data.size() > 0)
        {
            WriteWrapped(dataWriteOffset, data.data(), static_cast<UINT32>(data.size()));
        }

        UINT32 newWriteOffset = (dataWriteOffset + data.size()) % cap;
        m_controlBlock->Producer.WriteOffset.store(newWriteOffset, std::memory_order_release);

        return TRUE;
    }

    RingBufferView::ControlBlock *RingBufferView::GetControlBlock()
    {
        return m_controlBlock;
    }

    VOID RingBufferView::WriteWrapped(IN UINT32 offset, IN PCVOID data, IN UINT32 size)
    {
        if (offset + size <= m_capacity)
        {
            // Contiguous write
            memcpy(m_dataPtr + offset, data, size);
        }
        else
        {
            // Split write
            UINT32 firstChunk = m_capacity - offset;
            UINT32 secondChunk = size - firstChunk;

            const UINT8 *src = static_cast<const UINT8 *>(data);

            memcpy(m_dataPtr + offset, src, firstChunk);
            memcpy(m_dataPtr, src + firstChunk, secondChunk);
        }
    }

    VOID RingBufferView::ReadWrapped(IN UINT32 offset, OUT PVOID outData, IN UINT32 size)
    {
        if (offset + size <= m_capacity)
        {
            // Contiguous read
            memcpy(outData, m_dataPtr + offset, size);
        }
        else
        {
            // Split read
            UINT32 firstChunk = m_capacity - offset;
            UINT32 secondChunk = size - firstChunk;

            UINT8 *dst = static_cast<UINT8 *>(outData);

            memcpy(dst, m_dataPtr + offset, firstChunk);
            memcpy(dst + firstChunk, m_dataPtr, secondChunk);
        }
    }
} // namespace IACore