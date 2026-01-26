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
#include <IACore/IATest.hpp>
#include <chrono>
#include <thread>

using namespace IACore;

struct SchedulerGuard
{
  SchedulerGuard(u8 worker_count = 2)
  {

    (void) AsyncOps::initialize_scheduler(worker_count);
  }

  ~SchedulerGuard()
  {
    AsyncOps::terminate_scheduler();
  }
};

IAT_BEGIN_BLOCK(Core, AsyncOps)

auto test_initialization() -> bool
{

  AsyncOps::terminate_scheduler();

  const auto res = AsyncOps::initialize_scheduler(4);
  IAT_CHECK(res.has_value());

  IAT_CHECK_EQ(AsyncOps::get_worker_count(), static_cast<u16>(4));

  AsyncOps::terminate_scheduler();

  const auto res2 = AsyncOps::initialize_scheduler(1);
  IAT_CHECK(res2.has_value());
  IAT_CHECK_EQ(AsyncOps::get_worker_count(), static_cast<u16>(1));

  AsyncOps::terminate_scheduler();
  return true;
}

auto test_basic_execution() -> bool
{
  SchedulerGuard guard(2);

  AsyncOps::Schedule schedule;
  std::atomic<i32> run_count{0};

  AsyncOps::schedule_task([&](AsyncOps::WorkerId) { run_count++; }, 0, &schedule);

  AsyncOps::wait_for_schedule_completion(&schedule);

  IAT_CHECK_EQ(run_count.load(), 1);

  return true;
}

auto test_concurrency() -> bool
{
  SchedulerGuard guard(4);

  AsyncOps::Schedule schedule;
  std::atomic<i32> run_count{0};
  const i32 total_tasks = 100;

  for (i32 i = 0; i < total_tasks; ++i)
  {
    AsyncOps::schedule_task(
        [&](AsyncOps::WorkerId) {
          std::this_thread::sleep_for(std::chrono::microseconds(10));
          run_count++;
        },
        0, &schedule);
  }

  AsyncOps::wait_for_schedule_completion(&schedule);

  IAT_CHECK_EQ(run_count.load(), total_tasks);

  return true;
}

auto test_priorities() -> bool
{
  SchedulerGuard guard(2);
  AsyncOps::Schedule schedule;
  std::atomic<i32> high_priority_ran{0};
  std::atomic<i32> normal_priority_ran{0};

  AsyncOps::schedule_task([&](AsyncOps::WorkerId) { high_priority_ran++; }, 0, &schedule, AsyncOps::Priority::High);

  AsyncOps::schedule_task([&](AsyncOps::WorkerId) { normal_priority_ran++; }, 0, &schedule, AsyncOps::Priority::Normal);

  AsyncOps::wait_for_schedule_completion(&schedule);

  IAT_CHECK_EQ(high_priority_ran.load(), 1);
  IAT_CHECK_EQ(normal_priority_ran.load(), 1);

  return true;
}

auto test_run_task_fire_and_forget() -> bool
{
  SchedulerGuard guard(2);

  std::atomic<bool> executed{false};

  AsyncOps::run_task([&]() { executed = true; });

  for (int i = 0; i < 100; ++i)
  {
    if (executed.load())
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  IAT_CHECK(executed.load());

  return true;
}

auto test_cancellation_safety() -> bool
{
  SchedulerGuard guard(2);

  AsyncOps::cancel_tasks_of_tag(999);

  AsyncOps::Schedule schedule;
  std::atomic<i32> counter{0};

  AsyncOps::schedule_task([&](AsyncOps::WorkerId) { counter++; }, 10, &schedule);

  AsyncOps::wait_for_schedule_completion(&schedule);
  IAT_CHECK_EQ(counter.load(), 1);

  AsyncOps::cancel_tasks_of_tag(10);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_initialization);
IAT_ADD_TEST(test_basic_execution);
IAT_ADD_TEST(test_concurrency);
IAT_ADD_TEST(test_priorities);
IAT_ADD_TEST(test_run_task_fire_and_forget);
IAT_ADD_TEST(test_cancellation_safety);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, AsyncOps)