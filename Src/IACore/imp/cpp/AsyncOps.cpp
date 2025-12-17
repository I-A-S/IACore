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

#include <IACore/AsyncOps.hpp>

namespace IACore
{
    Mutex AsyncOps::s_queueMutex;
    ConditionVariable AsyncOps::s_wakeCondition;
    Vector<JoiningThread> AsyncOps::s_scheduleWorkers;
    Deque<AsyncOps::ScheduledTask> AsyncOps::s_highPriorityQueue;
    Deque<AsyncOps::ScheduledTask> AsyncOps::s_normalPriorityQueue;

    VOID AsyncOps::RunTask(IN Function<VOID()> task)
    {
        JoiningThread(task).detach();
    }

    VOID AsyncOps::InitializeScheduler(IN UINT8 workerCount)
    {
        if (!workerCount)
            workerCount = std::max((UINT32) 2, std::thread::hardware_concurrency() - 2);
        for (UINT32 i = 0; i < workerCount; i++)
            s_scheduleWorkers.emplace_back(AsyncOps::ScheduleWorkerLoop, i + 1);
    }

    VOID AsyncOps::TerminateScheduler()
    {
        for (auto &w : s_scheduleWorkers)
        {
            w.request_stop();
        }

        s_wakeCondition.notify_all();

        for (auto &w : s_scheduleWorkers)
        {
            if (w.joinable())
            {
                w.join();
            }
        }

        s_scheduleWorkers.clear();
    }

    VOID AsyncOps::ScheduleTask(IN Function<VOID(IN WorkerID workerID)> task, IN TaskTag tag, IN Schedule *schedule,
                                IN Priority priority)
    {
        IA_ASSERT(s_scheduleWorkers.size() && "Scheduler must be initialized before calling this function");

        schedule->Counter.fetch_add(1);
        {
            ScopedLock lock(s_queueMutex);
            if (priority == Priority::High)
                s_highPriorityQueue.emplace_back(ScheduledTask{tag, schedule, IA_MOVE(task)});
            else
                s_normalPriorityQueue.emplace_back(ScheduledTask{tag, schedule, IA_MOVE(task)});
        }
        s_wakeCondition.notify_one();
    }

    VOID AsyncOps::CancelTasksOfTag(IN TaskTag tag)
    {
        ScopedLock lock(s_queueMutex);

        auto cancelFromQueue = [&](Deque<ScheduledTask> &queue) {
            for (auto it = queue.begin(); it != queue.end(); /* no increment here */)
            {
                if (it->Tag == tag)
                {
                    if (it->ScheduleHandle->Counter.fetch_sub(1) == 1)
                        it->ScheduleHandle->Counter.notify_all();

                    it = queue.erase(it);
                }
                else
                    ++it;
            }
        };

        cancelFromQueue(s_highPriorityQueue);
        cancelFromQueue(s_normalPriorityQueue);
    }

    VOID AsyncOps::WaitForScheduleCompletion(IN Schedule *schedule)
    {
        IA_ASSERT(s_scheduleWorkers.size() && "Scheduler must be initialized before calling this function");

        while (schedule->Counter.load() > 0)
        {
            ScheduledTask task;
            BOOL foundTask{FALSE};
            {
                UniqueLock lock(s_queueMutex);
                if (!s_highPriorityQueue.empty())
                {
                    task = IA_MOVE(s_highPriorityQueue.front());
                    s_highPriorityQueue.pop_front();
                    foundTask = TRUE;
                }
                else if (!s_normalPriorityQueue.empty())
                {
                    task = IA_MOVE(s_normalPriorityQueue.front());
                    s_normalPriorityQueue.pop_front();
                    foundTask = TRUE;
                }
            }
            if (foundTask)
            {
                task.Task(MainThreadWorkerID);
                if (task.ScheduleHandle->Counter.fetch_sub(1) == 1)
                    task.ScheduleHandle->Counter.notify_all();
            }
            else
            {
                auto currentVal = schedule->Counter.load();
                if (currentVal > 0)
                    schedule->Counter.wait(currentVal);
            }
        }
    }

    AsyncOps::WorkerID AsyncOps::GetWorkerCount()
    {
        return static_cast<WorkerID>(s_scheduleWorkers.size() + 1); // +1 for MainThread (Work Stealing)
    }

    VOID AsyncOps::ScheduleWorkerLoop(IN StopToken stopToken, IN WorkerID workerID)
    {
        while (!stopToken.stop_requested())
        {
            ScheduledTask task;
            BOOL foundTask{FALSE};
            {
                UniqueLock lock(s_queueMutex);

                s_wakeCondition.wait(lock, [&stopToken] {
                    return !s_highPriorityQueue.empty() || !s_normalPriorityQueue.empty() || stopToken.stop_requested();
                });

                if (stopToken.stop_requested() && s_highPriorityQueue.empty() && s_normalPriorityQueue.empty())
                    return;

                if (!s_highPriorityQueue.empty())
                {
                    task = IA_MOVE(s_highPriorityQueue.front());
                    s_highPriorityQueue.pop_front();
                    foundTask = TRUE;
                }
                else if (!s_normalPriorityQueue.empty())
                {
                    task = IA_MOVE(s_normalPriorityQueue.front());
                    s_normalPriorityQueue.pop_front();
                    foundTask = TRUE;
                }
            }
            if (foundTask)
            {
                task.Task(workerID);
                if (task.ScheduleHandle->Counter.fetch_sub(1) == 1)
                    task.ScheduleHandle->Counter.notify_all();
            }
        }
    }
} // namespace IACore