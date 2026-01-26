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

#define __iat_micro_test(call)                                                                                         \
  if (!(call))                                                                                                         \
  return false

#define IAT_CHECK(v) __iat_micro_test(_test((v), #v))
#define IAT_CHECK_NOT(v) __iat_micro_test(_test_not((v), "NOT " #v))
#define IAT_CHECK_EQ(lhs, rhs) __iat_micro_test(_test_eq((lhs), (rhs), #lhs " == " #rhs))
#define IAT_CHECK_NEQ(lhs, rhs) __iat_micro_test(_test_neq((lhs), (rhs), #lhs " != " #rhs))

#define IAT_CHECK_APPROX(lhs, rhs) __iat_micro_test(_test_approx((lhs), (rhs), #lhs " ~= " #rhs))
#define IAT_CHECK_APPROX_EPS(lhs, rhs, eps) __iat_micro_test(_test_approx((lhs), (rhs), #lhs " ~= " #rhs, (eps)))

#define IAT_UNIT(func) _test_unit([this]() { return this->func(); }, #func)
#define IAT_NAMED_UNIT(n, func) _test_unit([this]() { return this->func(); }, n)

#define IAT_BLOCK(name) class name : public IACore::Test::Block

#define IAT_BEGIN_BLOCK(_group, _name)                                                                                 \
  class _group##_##_name : public IACore::Test::Block                                                                  \
  {                                                                                                                    \
public:                                                                                                                \
    [[nodiscard]] auto get_name() const -> const char * override                                                       \
    {                                                                                                                  \
      return #_group "::" #_name;                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
private:

#define IAT_END_BLOCK()                                                                                                \
  }                                                                                                                    \
  ;

#define IAT_BEGIN_TEST_LIST()                                                                                          \
  public:                                                                                                              \
  auto declare_tests() -> void override                                                                                \
  {
#define IAT_ADD_TEST(name) IAT_UNIT(name)
#define IAT_END_TEST_LIST()                                                                                            \
  }                                                                                                                    \
                                                                                                                       \
  private:

namespace IACore::Test
{
  template<typename T>
  concept Streamable = requires(std::ostream &os, const T &t) { os << t; };

  template<typename T> auto to_string(const T &value) -> String
  {
    if constexpr (std::is_arithmetic_v<T>)
    {
      return std::to_string(value);
    }
    else if constexpr (std::is_convertible_v<T, String> || std::is_same_v<T, const char *> || std::is_same_v<T, char *>)
    {
      return String("\"") + String(value) + "\"";
    }
    else if constexpr (Streamable<T>)
    {
      std::stringstream ss;
      ss << value;
      return ss.str();
    }
    else
    {
      return "{Object}";
    }
  }

  template<typename T> auto to_string(T *value) -> String
  {
    if (value == nullptr)
    {
      return "nullptr";
    }
    return std::format("ptr({})", static_cast<const void *>(value));
  }

  using TestFunctor = std::function<bool()>;

  struct TestUnit
  {
    Mut<String> name;
    Mut<TestFunctor> functor;
  };

  class Block
  {
public:
    virtual ~Block() = default;
    [[nodiscard]] virtual auto get_name() const -> const char * = 0;
    virtual auto declare_tests() -> void = 0;

    auto units() -> MutRef<Vec<TestUnit>>
    {
      return m_units;
    }

protected:
    template<typename T1, typename T2> auto _test_eq(Ref<T1> lhs, Ref<T2> rhs, const char *description) -> bool
    {
      if (lhs != rhs)
      {
        print_fail(description, to_string(lhs), to_string(rhs));
        return false;
      }
      return true;
    }

    template<typename T1, typename T2> auto _test_neq(Ref<T1> lhs, Ref<T2> rhs, const char *description) -> bool
    {
      if (lhs == rhs)
      {
        print_fail(description, to_string(lhs), "NOT " + to_string(rhs));
        return false;
      }
      return true;
    }

    template<typename T>
    auto _test_approx(const T lhs, const T rhs, const char *description, const T epsilon = static_cast<T>(0.001))
        -> bool
    {
      static_assert(std::is_floating_point_v<T>, "Approx only works for floats/doubles");

      if (lhs == static_cast<T>(0.0) || rhs == static_cast<T>(0.0))
      {
        if (std::abs(lhs - rhs) > epsilon)
        {
          print_fail(description, to_string(lhs), to_string(rhs));
          return false;
        }
        return true;
      }

      const T diff = std::abs(lhs - rhs);
      const T larger = std::max(std::abs(lhs), std::abs(rhs));

      if (diff > (larger * epsilon))
      {
        print_fail(description, to_string(lhs), to_string(rhs));
        return false;
      }
      return true;
    }

    auto _test(const bool value, const char *description) -> bool
    {
      if (!value)
      {
        std::cout << console::BLUE << "    " << description << "... " << console::RED << "FAILED" << console::RESET
                  << "\n";
        return false;
      }
      return true;
    }

    auto _test_not(const bool value, const char *description) -> bool
    {
      if (value)
      {
        std::cout << console::BLUE << "    " << description << "... " << console::RED << "FAILED" << console::RESET
                  << "\n";
        return false;
      }
      return true;
    }

    auto _test_unit(Mut<TestFunctor> functor, const char *name) -> void
    {
      m_units.push_back({name, std::move(functor)});
    }

private:
    auto print_fail(const char *desc, Ref<String> v1, Ref<String> v2) -> void
    {
      std::cout << console::BLUE << "    " << desc << "... " << console::RED << "FAILED" << console::RESET << "\n";
      std::cout << console::RED << "      Expected: " << v2 << console::RESET << "\n";
      std::cout << console::RED << "      Actual:   " << v1 << console::RESET << "\n";
    }

    Mut<Vec<TestUnit>> m_units;
  };

  template<typename T>
  concept ValidBlockClass = std::derived_from<T, Block>;

  template<bool StopOnFail = false, bool IsVerbose = false> class Runner
  {
public:
    Runner() = default;

    ~Runner()
    {
      summarize();
    }

    template<typename BlockClass>
      requires ValidBlockClass<BlockClass>
    auto test_block() -> void;

private:
    auto summarize() -> void;

    Mut<usize> m_test_count{0};
    Mut<usize> m_fail_count{0};
    Mut<usize> m_block_count{0};
  };

  template<bool StopOnFail, bool IsVerbose>
  template<typename BlockClass>
    requires ValidBlockClass<BlockClass>
  auto Runner<StopOnFail, IsVerbose>::test_block() -> void
  {
    m_block_count++;
    Mut<BlockClass> b;
    b.declare_tests();

    std::cout << console::MAGENTA << "Testing [" << b.get_name() << "]..." << console::RESET << "\n";

    for (MutRef<TestUnit> v : b.units())
    {
      m_test_count++;
      if constexpr (IsVerbose)
      {
        std::cout << console::YELLOW << "  Testing " << v.name << "...\n" << console::RESET;
      }

      bool result = false;

      try
      {
        result = v.functor();
      }
      catch (const std::exception &e)
      {
        std::cout << console::RED << "    [EXCEPTION] " << v.name << ": " << e.what() << console::RESET << "\n";
        result = false;
      }
      catch (...)
      {
        std::cout << console::RED << "    [UNKNOWN EXCEPTION] " << v.name << console::RESET << "\n";
        result = false;
      }

      if (!result)
      {
        m_fail_count++;
        if constexpr (StopOnFail)
        {
          summarize();
          std::exit(-1);
        }
      }
    }
    std::cout << "\n";
  }

  template<bool StopOnFail, bool IsVerbose> auto Runner<StopOnFail, IsVerbose>::summarize() -> void
  {
    std::cout << console::GREEN
              << "\n-----------------------------------\n\t      "
                 "SUMMARY\n-----------------------------------\n";

    if (m_fail_count == 0)
    {
      std::cout << "\n\tALL TESTS PASSED!\n\n";
    }
    else
    {
      const f64 success_rate = (100.0 * static_cast<f64>(m_test_count - m_fail_count) / static_cast<f64>(m_test_count));
      std::cout << console::RED << m_fail_count << " OF " << m_test_count << " TESTS FAILED\n"
                << console::YELLOW << std::format("Success Rate: {:.2f}%\n", success_rate);
    }

    std::cout << console::MAGENTA << "Ran " << m_test_count << " test(s) across " << m_block_count << " block(s)\n"
              << console::GREEN << "-----------------------------------" << console::RESET << "\n";
  }

  using DefaultRunner = Runner<false, true>;

  class TestRegistry
  {
public:
    using TestEntry = std::function<void(MutRef<DefaultRunner>)>;

    static auto get_entries() -> MutRef<Vec<TestEntry>>
    {
      static Mut<Vec<TestEntry>> entries;
      return entries;
    }

    static auto run_all() -> i32
    {
      Mut<DefaultRunner> r;
      MutRef<Vec<TestEntry>> entries = get_entries();
      std::cout << console::CYAN << "[IATest] Discovered " << entries.size() << " Test Blocks\n\n" << console::RESET;

      for (MutRef<TestEntry> entry : entries)
      {
        entry(r);
      }

      return 0;
    }
  };

  template<typename BlockType> struct AutoRegister
  {
    AutoRegister()
    {
      TestRegistry::get_entries().push_back([](MutRef<DefaultRunner> r) { r.test_block<BlockType>(); });
    }
  };
} // namespace IACore::Test

#define IAT_REGISTER_ENTRY(Group, Name) static IACore::Test::AutoRegister<Group##_##Name> _iat_reg_##Group##_##Name;