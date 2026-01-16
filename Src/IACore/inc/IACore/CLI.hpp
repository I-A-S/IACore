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

namespace IACore
{
    class CLIParser
    {
        /*
         * PLEASE READ
         *
         * CLIParser is still very much in it's baby stages.
         * Subject to heavy and frequent changes, use with
         * caution!
         */

      public:
        CLIParser(IN Span<CONST String> args);
        ~CLIParser() = default;

      public:
        BOOL Remaining() CONST
        {
            return m_currentArg < m_argList.end();
        }

        StringView Peek() CONST
        {
            if (!Remaining())
                return "";
            return *m_currentArg;
        }

        StringView Next()
        {
            if (!Remaining())
                return "";
            return *m_currentArg++;
        }

        BOOL Consume(IN CONST StringView &expected)
        {
            if (Peek() == expected)
            {
                Next();
                return true;
            }
            return false;
        }

      private:
        CONST Span<CONST String> m_argList;
        Span<CONST String>::const_iterator m_currentArg;
    };
} // namespace IACore