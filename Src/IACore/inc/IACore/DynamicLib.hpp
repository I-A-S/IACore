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

#if !IA_PLATFORM_WINDOWS
#  include <dlfcn.h>
#endif

namespace IACore
{

  class DynamicLib
  {
public:
    IA_NODISCARD static auto load(Ref<String> search_path, Ref<String> name) -> Result<DynamicLib>
    {
      namespace fs = std::filesystem;
      Mut<Path> full_path = fs::path(search_path) / name;

      if (!full_path.has_extension())
      {
#if IA_PLATFORM_WINDOWS
        full_path += ".dll";
#elif IA_PLATFORM_APPLE
        full_path += ".dylib";
#else
        full_path += ".so";
#endif
      }

      Mut<DynamicLib> lib;

#if IA_PLATFORM_WINDOWS
      const HMODULE h = LoadLibraryA(full_path.string().c_str());
      if (!h)
      {
        return fail(get_windows_error());
      }
      lib.m_handle = static_cast<void *>(h);
#else
      Mut<void *> h = dlopen(full_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
      if (!h)
      {
        const char *err = dlerror();
        return fail(err ? err : "Unknown dlopen error");
      }
      lib.m_handle = h;
#endif

      return lib;
    }

    DynamicLib() = default;

    DynamicLib(ForwardRef<DynamicLib> other) noexcept : m_handle(other.m_handle)
    {
      other.m_handle = nullptr;
    }

    auto operator=(ForwardRef<DynamicLib> other) noexcept -> MutRef<DynamicLib>
    {
      if (this != &other)
      {
        unload();
        m_handle = other.m_handle;
        other.m_handle = nullptr;
      }
      return *this;
    }

    DynamicLib(Ref<DynamicLib>) = delete;
    auto operator=(Ref<DynamicLib>) -> MutRef<DynamicLib> = delete;

    ~DynamicLib()
    {
      unload();
    }

    IA_NODISCARD auto get_symbol(Ref<String> name) const -> Result<void *>
    {
      if (!m_handle)
      {
        return fail("Library not loaded");
      }

      Mut<void *> sym = nullptr;

#if IA_PLATFORM_WINDOWS
      sym = static_cast<void *>(GetProcAddress(static_cast<HMODULE>(m_handle), name.c_str()));
      if (!sym)
      {
        return fail(get_windows_error());
      }
#else
      dlerror(); // Clear prev errors
      sym = dlsym(m_handle, name.c_str());
      if (const char *err = dlerror())
      {
        return fail(err);
      }
#endif

      return sym;
    }

    template<typename FuncT> IA_NODISCARD auto get_function(Ref<String> name) const -> Result<FuncT>
    {
      Mut<void *> sym = nullptr;
      sym = AU_TRY(get_symbol(name));
      return reinterpret_cast<FuncT>(sym);
    }

    void unload()
    {
      if (m_handle)
      {
#if IA_PLATFORM_WINDOWS
        FreeLibrary(static_cast<HMODULE>(m_handle));
#else
        dlclose(m_handle);
#endif
        m_handle = nullptr;
      }
    }

    IA_NODISCARD auto is_loaded() const -> bool
    {
      return m_handle != nullptr;
    }

private:
    Mut<void *> m_handle = nullptr;

#if IA_PLATFORM_WINDOWS
    static auto get_windows_error() -> String
    {
      const DWORD error_id = ::GetLastError();
      if (error_id == 0)
      {
        return String();
      }

      Mut<LPSTR> message_buffer = nullptr;
      const usize size = FormatMessageA(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
          error_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr);

      const String message(message_buffer, size);
      LocalFree(message_buffer);
      return "Win32 Error: " + message;
    }
#endif
  };

} // namespace IACore