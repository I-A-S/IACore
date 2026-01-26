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
    CLIParser(const Span<const String> args);
    ~CLIParser() = default;

public:
    IA_NODISCARD auto remaining() const -> bool
    {
      return m_current_arg < m_arg_list.end();
    }

    IA_NODISCARD auto peek() const -> StringView
    {
      if (!remaining())
        return "";
      return *m_current_arg;
    }

    auto next() -> StringView
    {
      if (!remaining())
        return "";
      return *m_current_arg++;
    }

    auto consume(Ref<StringView> expected) -> bool
    {
      if (peek() == expected)
      {
        next();
        return true;
      }
      return false;
    }

private:
    const Span<const String> m_arg_list;
    Mut<Span<const String>::const_iterator> m_current_arg;
  };
} // namespace IACore