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

#include <IACore/ADT/RingBuffer.hpp>
#include <IACore/IATest.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, RingBuffer)

// -------------------------------------------------------------------------
// 1. Basic Push Pop
// -------------------------------------------------------------------------
BOOL TestPushPop()
{
    // Allocate raw memory for the ring buffer
    // ControlBlock (128 bytes) + Data
    std::vector<UINT8> memory(sizeof(RingBufferView::ControlBlock) + 1024);

    // Initialize as OWNER (Producer)
    RingBufferView producer(std::span<UINT8>(memory), TRUE);

    // Initialize as CONSUMER (Pointer to same memory)
    RingBufferView consumer(std::span<UINT8>(memory), FALSE);

    // Data to send
    String msg = "Hello RingBuffer";
    BOOL pushed = producer.Push(1, {(const UINT8 *) msg.data(), msg.size()});
    IAT_CHECK(pushed);

    // Read back
    RingBufferView::PacketHeader header;
    UINT8 readBuf[128];

    INT32 bytesRead = consumer.Pop(header, std::span<UINT8>(readBuf, 128));

    IAT_CHECK_EQ(header.ID, (UINT16) 1);
    IAT_CHECK_EQ(bytesRead, (INT32) msg.size());

    String readMsg((char *) readBuf, bytesRead);
    IAT_CHECK_EQ(readMsg, msg);

    return TRUE;
}

// -------------------------------------------------------------------------
// 2. Wrap Around
// -------------------------------------------------------------------------
BOOL TestWrapAround()
{
    // Small buffer to force wrapping quickly
    // Capacity will be 100 bytes
    std::vector<UINT8> memory(sizeof(RingBufferView::ControlBlock) + 100);
    RingBufferView rb(std::span<UINT8>(memory), TRUE);

    // Fill buffer to near end
    // Push 80 bytes
    std::vector<UINT8> junk(80, 0xFF);
    rb.Push(1, junk);

    // Pop them to advance READ cursor
    RingBufferView::PacketHeader header;
    UINT8 outBuf[100];
    rb.Pop(header, outBuf);

    // Now READ and WRITE are near index 80.
    // Pushing 40 bytes should trigger a wrap-around (split write)
    std::vector<UINT8> wrapData(40, 0xAA);
    BOOL pushed = rb.Push(2, wrapData);
    IAT_CHECK(pushed);

    // Pop and verify integrity
    INT32 popSize = rb.Pop(header, outBuf);
    IAT_CHECK_EQ(popSize, 40);

    // Check if data is intact
    BOOL match = TRUE;
    for (int i = 0; i < 40; i++)
    {
        if (outBuf[i] != 0xAA)
            match = FALSE;
    }
    IAT_CHECK(match);

    return TRUE;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(TestPushPop);
IAT_ADD_TEST(TestWrapAround);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, RingBuffer)
