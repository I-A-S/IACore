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

#pragma once

#include <IACore/PCH.hpp>
#include <deque>
#include <functional>
#include <stop_token>

namespace IACore
{
  class AsyncOps
  {
public:
    using TaskTag = u64;
    using WorkerId = u16;

    static constexpr const WorkerId MAIN_THREAD_WORKER_ID = 0;

    enum class Priority : u8
    {
      High,
      Normal
    };

    struct Schedule
    {
      Mut<std::atomic<i32>> counter{0};
    };

public:
    static auto initialize_scheduler(const u8 worker_count = 0) -> Result<void>;
    static auto terminate_scheduler() -> void;

    static auto schedule_task(Mut<std::function<void(const WorkerId)>> task, const TaskTag tag,
                              Mut<Schedule *> schedule, const Priority priority = Priority::Normal) -> void;

    static auto cancel_tasks_of_tag(const TaskTag tag) -> void;

    static auto wait_for_schedule_completion(Mut<Schedule *> schedule) -> void;

    static auto run_task(Mut<std::function<void()>> task) -> void;

    IA_NODISCARD static auto get_worker_count() -> WorkerId;

private:
    struct ScheduledTask
    {
      Mut<TaskTag> tag{};
      Mut<Schedule *> schedule_handle{};
      Mut<std::function<void(const WorkerId)>> task{};
    };

    static auto schedule_worker_loop(Mut<std::stop_token> stop_token, const WorkerId worker_id) -> void;

private:
    static Mut<std::mutex> s_queue_mutex;
    static Mut<std::condition_variable> s_wake_condition;
    static Mut<Vec<std::jthread>> s_schedule_workers;
    static Mut<std::deque<ScheduledTask>> s_high_priority_queue;
    static Mut<std::deque<ScheduledTask>> s_normal_priority_queue;
  };
} // namespace IACore