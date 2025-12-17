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

#ifdef __cplusplus

#    include <exception>

// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------

#    define valid_iatest_runner(type) iatest::_valid_iatest_runner<type>::value_type

#    define __iat_micro_test(call)                                                                                     \
        if (!(call))                                                                                                   \
        return FALSE

#    define IAT_CHECK(v) __iat_micro_test(_test((v), #v))
#    define IAT_CHECK_NOT(v) __iat_micro_test(_test_not((v), "NOT " #v))
#    define IAT_CHECK_EQ(lhs, rhs) __iat_micro_test(_test_eq((lhs), (rhs), #lhs " == " #rhs))
#    define IAT_CHECK_NEQ(lhs, rhs) __iat_micro_test(_test_neq((lhs), (rhs), #lhs " != " #rhs))

#    define IAT_CHECK_APPROX(lhs, rhs) __iat_micro_test(_test_approx((lhs), (rhs), #lhs " ~= " #rhs))

#    define IAT_UNIT(func) _test_unit([this]() { return this->func(); }, #func)
#    define IAT_NAMED_UNIT(n, func) _test_unit([this]() { return this->func(); }, n)

#    define IAT_BLOCK(name) class name : public ia::iatest::block

#    define IAT_BEGIN_BLOCK(_group, _name)                                                                             \
        class _group##_##_name : public ia::iatest::block                                                              \
        {                                                                                                              \
          public:                                                                                                      \
            PCCHAR name() CONST OVERRIDE                                                                               \
            {                                                                                                          \
                return #_group "::" #_name;                                                                            \
            }                                                                                                          \
                                                                                                                       \
          private:

#    define IAT_END_BLOCK()                                                                                            \
        }                                                                                                              \
        ;

#    define IAT_BEGIN_TEST_LIST()                                                                                      \
      public:                                                                                                          \
        VOID declareTests() OVERRIDE                                                                                   \
        {
#    define IAT_ADD_TEST(name) IAT_UNIT(name)
#    define IAT_END_TEST_LIST()                                                                                        \
        }                                                                                                              \
                                                                                                                       \
      private:

namespace ia::iatest
{
    template<typename T> std::string ToString(CONST T &value)
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            return std::to_string(value);
        }
        else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, const char *>)
        {
            return std::string("\"") + value + "\"";
        }
        else
        {
            return "{Object}"; // Fallback for complex types
        }
    }

    template<typename T> std::string ToString(T *value)
    {
        if (value == NULLPTR)
            return "nullptr";
        std::stringstream ss;
        ss << "ptr(" << (void *) value << ")";
        return ss.str();
    }

    DEFINE_TYPE(functor_t, std::function<BOOL()>);

    struct unit_t
    {
        std::string Name;
        functor_t Functor;
    };

    class block
    {
      public:
        virtual ~block() = default;
        PURE_VIRTUAL(PCCHAR name() CONST);
        PURE_VIRTUAL(VOID declareTests());

        std::vector<unit_t> &units()
        {
            return m_units;
        }

      protected:
        template<typename T1, typename T2> BOOL _test_eq(IN CONST T1 &lhs, IN CONST T2 &rhs, IN PCCHAR description)
        {
            if (lhs != rhs)
            {
                print_fail(description, ToString(lhs), ToString(rhs));
                return FALSE;
            }
            return TRUE;
        }

        template<typename T1, typename T2> BOOL _test_neq(IN CONST T1 &lhs, IN CONST T2 &rhs, IN PCCHAR description)
        {
            if (lhs == rhs)
            {
                print_fail(description, ToString(lhs), "NOT " + ToString(rhs));
                return FALSE;
            }
            return TRUE;
        }

        template<typename T> BOOL _test_approx(IN T lhs, IN T rhs, IN PCCHAR description)
        {
            static_assert(std::is_floating_point_v<T>, "Approx only works for floats/doubles");
            T diff = std::abs(lhs - rhs);
            if (diff > static_cast<T>(0.0001))
            {
                print_fail(description, ToString(lhs), ToString(rhs));
                return FALSE;
            }
            return TRUE;
        }

        BOOL _test(IN BOOL value, IN PCCHAR description)
        {
            if (!value)
            {
                printf(__CC_BLUE "    %s... " __CC_RED "FAILED" __CC_DEFAULT "\n", description);
                return FALSE;
            }
            return TRUE;
        }

        BOOL _test_not(IN BOOL value, IN PCCHAR description)
        {
            if (value)
            {
                printf(__CC_BLUE "    %s... " __CC_RED "FAILED" __CC_DEFAULT "\n", description);
                return FALSE;
            }
            return TRUE;
        }

        VOID _test_unit(IN functor_t functor, IN PCCHAR name)
        {
            m_units.push_back({name, functor});
        }

      private:
        VOID print_fail(PCCHAR desc, std::string v1, std::string v2)
        {
            printf(__CC_BLUE "    %s... " __CC_RED "FAILED" __CC_DEFAULT "\n", desc);
            printf(__CC_RED "      Expected: %s" __CC_DEFAULT "\n", v2.c_str());
            printf(__CC_RED "      Actual:   %s" __CC_DEFAULT "\n", v1.c_str());
        }

        std::vector<unit_t> m_units;
    };

    template<typename block_class>
    concept valid_block_class = std::derived_from<block_class, block>;

    template<BOOL stopOnFail = false, BOOL isVerbose = false> class runner
    {
      public:
        runner()
        {
        }

        ~runner()
        {
            summarize();
        }

        template<typename block_class>
            requires valid_block_class<block_class>
        VOID testBlock();

      private:
        VOID summarize();

      private:
        SIZE_T m_testCount{0};
        SIZE_T m_failCount{0};
        SIZE_T m_blockCount{0};
    };

    template<BOOL stopOnFail, BOOL isVerbose>
    template<typename block_class>
        requires valid_block_class<block_class>
    VOID runner<stopOnFail, isVerbose>::testBlock()
    {
        m_blockCount++;
        block_class b;
        b.declareTests();

        printf(__CC_MAGENTA "Testing [%s]..." __CC_DEFAULT "\n", b.name());

        for (auto &v : b.units())
        {
            m_testCount++;
            if constexpr (isVerbose)
            {
                printf(__CC_YELLOW "  Testing %s...\n" __CC_DEFAULT, v.Name.c_str());
            }

            BOOL result = FALSE;
            try
            {
                result = v.Functor();
            }
            catch (const std::exception &e)
            {
                printf(__CC_RED "    CRITICAL EXCEPTION in %s: %s\n" __CC_DEFAULT, v.Name.c_str(), e.what());
                result = FALSE;
            }
            catch (...)
            {
                printf(__CC_RED "    UNKNOWN CRITICAL EXCEPTION in %s\n" __CC_DEFAULT, v.Name.c_str());
                result = FALSE;
            }

            if (!result)
            {
                m_failCount++;
                if constexpr (stopOnFail)
                {
                    summarize();
                    exit(-1);
                }
            }
        }
        fputs("\n", stdout);
    }

    template<BOOL stopOnFail, BOOL isVerbose> VOID runner<stopOnFail, isVerbose>::summarize()
    {
        printf(__CC_GREEN
               "\n-----------------------------------\n\t      SUMMARY\n-----------------------------------\n");

        if (!m_failCount)
        {
            printf("\n\tALL TESTS PASSED!\n\n");
        }
        else
        {
            FLOAT64 successRate =
                (100.0 * static_cast<FLOAT64>(m_testCount - m_failCount) / static_cast<FLOAT64>(m_testCount));
            printf(__CC_RED "%zu OUT OF %zu TESTS FAILED\n" __CC_YELLOW "Success Rate: %.2f%%\n", m_failCount,
                   m_testCount, successRate);
        }

        printf(__CC_MAGENTA "Ran %zu test(s) across %zu block(s)\n" __CC_GREEN
                            "-----------------------------------" __CC_DEFAULT "\n",
               m_testCount, m_blockCount);
    }

    template<typename> struct _valid_iatest_runner : std::false_type
    {
    };

    template<BOOL stopOnFail, BOOL isVerbose>
    struct _valid_iatest_runner<runner<stopOnFail, isVerbose>> : std::true_type
    {
    };

    using DefaultRunner = runner<false, true>;

    class TestRegistry
    {
      public:
        using TestEntry = std::function<void(DefaultRunner &)>;

        static std::vector<TestEntry> &GetEntries()
        {
            static std::vector<TestEntry> entries;
            return entries;
        }

        static int RunAll()
        {
            DefaultRunner r;
            auto &entries = GetEntries();
            printf(__CC_CYAN "[IATest] Discovered %zu Test Blocks\n\n" __CC_DEFAULT, entries.size());

            for (auto &entry : entries)
            {
                entry(r);
            }
            // The destructor of 'r' will automatically print the summary
            return 0;
        }
    };

    template<typename BlockType> struct AutoRegister
    {
        AutoRegister()
        {
            TestRegistry::GetEntries().push_back([](DefaultRunner &r) { r.testBlock<BlockType>(); });
        }
    };
} // namespace ia::iatest

#    define IAT_REGISTER_ENTRY(Group, Name) static ia::iatest::AutoRegister<Group##_##Name> _iat_reg_##Group##_##Name;

#endif // __cplusplus