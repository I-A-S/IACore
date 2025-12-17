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
    class AsyncOps
    {
      public:
        using TaskTag = UINT64;
        using WorkerID = UINT16;

        STATIC CONSTEXPR WorkerID MainThreadWorkerID = 0;

        enum class Priority : UINT8
        {
            High,
            Normal
        };

        struct Schedule
        {
            Atomic<INT32> Counter{0};
        };

      public:
        STATIC VOID InitializeScheduler(IN UINT8 workerCount = 0);
        STATIC VOID TerminateScheduler();

        STATIC VOID ScheduleTask(IN Function<VOID(IN WorkerID workerID)> task, IN TaskTag tag, IN Schedule *schedule,
                                 IN Priority priority = Priority::Normal);

        STATIC VOID CancelTasksOfTag(IN TaskTag tag);

        STATIC VOID WaitForScheduleCompletion(IN Schedule *schedule);

        STATIC VOID RunTask(IN Function<VOID()> task);

        STATIC WorkerID GetWorkerCount();

      private:
        struct ScheduledTask
        {
            TaskTag Tag{};
            Schedule *ScheduleHandle{};
            Function<VOID(IN WorkerID workerID)> Task{};
        };

        STATIC VOID ScheduleWorkerLoop(IN StopToken stopToken, IN WorkerID workerID);

      private:
        STATIC Mutex s_queueMutex;
        STATIC ConditionVariable s_wakeCondition;
        STATIC Vector<JoiningThread> s_scheduleWorkers;
        STATIC Deque<ScheduledTask> s_highPriorityQueue;
        STATIC Deque<ScheduledTask> s_normalPriorityQueue;
    };
} // namespace IACore