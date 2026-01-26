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

#include <IACore/AsyncOps.hpp>

namespace IACore
{
  Mut<std::mutex> AsyncOps::s_queue_mutex;
  Mut<std::condition_variable> AsyncOps::s_wake_condition;
  Mut<Vec<std::jthread>> AsyncOps::s_schedule_workers;
  Mut<std::deque<AsyncOps::ScheduledTask>> AsyncOps::s_high_priority_queue;
  Mut<std::deque<AsyncOps::ScheduledTask>> AsyncOps::s_normal_priority_queue;

  auto AsyncOps::run_task(Mut<std::function<void()>> task) -> void
  {
    std::jthread(std::move(task)).detach();
  }

  auto AsyncOps::initialize_scheduler(Mut<u8> worker_count) -> Result<void>
  {
    if (worker_count == 0)
    {
      const u32 hw_concurrency = std::thread::hardware_concurrency();
      Mut<u32> threads = 2;
      if (hw_concurrency > 2)
      {
        threads = hw_concurrency - 2;
      }

      if (threads > 255)
      {
        threads = 255;
      }
      worker_count = static_cast<u8>(threads);
    }

    for (Mut<u32> i = 0; i < worker_count; ++i)
    {
      s_schedule_workers.emplace_back(schedule_worker_loop, static_cast<WorkerId>(i + 1));
    }

    return {};
  }

  auto AsyncOps::terminate_scheduler() -> void
  {
    for (MutRef<std::jthread> worker : s_schedule_workers)
    {
      worker.request_stop();
    }

    s_wake_condition.notify_all();

    for (MutRef<std::jthread> worker : s_schedule_workers)
    {
      if (worker.joinable())
      {
        worker.join();
      }
    }

    s_schedule_workers.clear();
  }

  auto AsyncOps::schedule_task(Mut<std::function<void(WorkerId worker_id)>> task, const TaskTag tag, Schedule *schedule,
                               const Priority priority) -> void
  {
    ensure(!s_schedule_workers.empty(), "Scheduler must be initialized before calling schedule_task");

    schedule->counter.fetch_add(1);
    {
      const std::lock_guard<std::mutex> lock(s_queue_mutex);
      if (priority == Priority::High)
      {
        s_high_priority_queue.emplace_back(ScheduledTask{tag, schedule, std::move(task)});
      }
      else
      {
        s_normal_priority_queue.emplace_back(ScheduledTask{tag, schedule, std::move(task)});
      }
    }
    s_wake_condition.notify_one();
  }

  auto AsyncOps::cancel_tasks_of_tag(const TaskTag tag) -> void
  {
    const std::lock_guard<std::mutex> lock(s_queue_mutex);

    {
      MutRef<std::deque<ScheduledTask>> queue = s_high_priority_queue;
      for (Mut<std::deque<ScheduledTask>::iterator> it = queue.begin(); it != queue.end();
           /* no incr */)
      {
        if (it->tag == tag)
        {
          if (it->schedule_handle->counter.fetch_sub(1) == 1)
          {
            it->schedule_handle->counter.notify_all();
          }
          it = queue.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }

    {
      MutRef<std::deque<ScheduledTask>> queue = s_normal_priority_queue;
      for (Mut<std::deque<ScheduledTask>::iterator> it = queue.begin(); it != queue.end();
           /* no incr */)
      {
        if (it->tag == tag)
        {
          if (it->schedule_handle->counter.fetch_sub(1) == 1)
          {
            it->schedule_handle->counter.notify_all();
          }
          it = queue.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
  }

  auto AsyncOps::wait_for_schedule_completion(Schedule *schedule) -> void
  {
    ensure(!s_schedule_workers.empty(), "Scheduler must be initialized before "
                                        "calling wait_for_schedule_completion");

    while (schedule->counter.load() > 0)
    {
      Mut<ScheduledTask> task;
      Mut<bool> found_task = false;
      {
        Mut<std::unique_lock<std::mutex>> lock(s_queue_mutex);
        if (!s_high_priority_queue.empty())
        {
          task = std::move(s_high_priority_queue.front());
          s_high_priority_queue.pop_front();
          found_task = true;
        }
        else if (!s_normal_priority_queue.empty())
        {
          task = std::move(s_normal_priority_queue.front());
          s_normal_priority_queue.pop_front();
          found_task = true;
        }
      }

      if (found_task)
      {
        task.task(MAIN_THREAD_WORKER_ID);
        if (task.schedule_handle->counter.fetch_sub(1) == 1)
        {
          task.schedule_handle->counter.notify_all();
        }
      }
      else
      {
        const u32 current_val = schedule->counter.load();
        if (current_val > 0)
        {
          schedule->counter.wait(current_val);
        }
      }
    }
  }

  auto AsyncOps::get_worker_count() -> WorkerId
  {
    return static_cast<WorkerId>(s_schedule_workers.size());
  }

  auto AsyncOps::schedule_worker_loop(const std::stop_token stop_token, const WorkerId worker_id) -> void
  {
    while (!stop_token.stop_requested())
    {
      Mut<ScheduledTask> task;
      Mut<bool> found_task = false;
      {
        Mut<std::unique_lock<std::mutex>> lock(s_queue_mutex);

        s_wake_condition.wait(lock, [&stop_token] {
          return !s_high_priority_queue.empty() || !s_normal_priority_queue.empty() || stop_token.stop_requested();
        });

        if (stop_token.stop_requested() && s_high_priority_queue.empty() && s_normal_priority_queue.empty())
        {
          return;
        }

        if (!s_high_priority_queue.empty())
        {
          task = std::move(s_high_priority_queue.front());
          s_high_priority_queue.pop_front();
          found_task = true;
        }
        else if (!s_normal_priority_queue.empty())
        {
          task = std::move(s_normal_priority_queue.front());
          s_normal_priority_queue.pop_front();
          found_task = true;
        }
      }

      if (found_task)
      {
        task.task(worker_id);
        if (task.schedule_handle->counter.fetch_sub(1) == 1)
        {
          task.schedule_handle->counter.notify_all();
        }
      }
    }
  }

} // namespace IACore