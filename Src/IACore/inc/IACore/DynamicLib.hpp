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

#if IA_PLATFORM_WINDOWS
#    include <libloaderapi.h>
#    include <errhandlingapi.h>
#else
#    include <dlfcn.h>
#endif

namespace IACore
{

    class DynamicLib
    {
      public:
        DynamicLib() : m_handle(nullptr)
        {
        }

        DynamicLib(DynamicLib &&other) NOEXCEPT : m_handle(other.m_handle)
        {
            other.m_handle = nullptr;
        }

        DynamicLib &operator=(DynamicLib &&other) NOEXCEPT
        {
            if (this != &other)
            {
                Unload(); // Free current if exists
                m_handle = other.m_handle;
                other.m_handle = nullptr;
            }
            return *this;
        }

        DynamicLib(CONST DynamicLib &) = delete;
        DynamicLib &operator=(CONST DynamicLib &) = delete;

        ~DynamicLib()
        {
            Unload();
        }

        // Automatically detects extension (.dll/.so) if not provided
        NO_DISCARD("Check for load errors")

        STATIC tl::expected<DynamicLib, String> Load(CONST String &searchPath, CONST String &name)
        {
            namespace fs = std::filesystem;

            fs::path fullPath = fs::path(searchPath) / name;

            if (!fullPath.has_extension())
            {
#if IA_PLATFORM_WINDOWS
                fullPath += ".dll";
#elif IA_PLATFORM_MAC
                fullPath += ".dylib";
#else
                fullPath += ".so";
#endif
            }

            DynamicLib lib;

#if IA_PLATFORM_WINDOWS
            HMODULE h = LoadLibraryA(fullPath.string().c_str());
            if (!h)
            {
                return tl::make_unexpected(GetWindowsError());
            }
            lib.m_handle = CAST(h, PVOID);
#else
            // RTLD_LAZY: Resolve symbols only as code executes (Standard for plugins)
            void *h = dlopen(fullPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (!h)
            {
                // dlerror returns a string describing the last error
                const char *err = dlerror();
                return tl::make_unexpected(String(err ? err : "Unknown dlopen error"));
            }
            lib.m_handle = h;
#endif

            return IA_MOVE(lib);
        }

        NO_DISCARD("Check if symbol exists")

        tl::expected<PVOID, String> GetSymbol(CONST String &name) CONST
        {
            if (!m_handle)
                return tl::make_unexpected(String("Library not loaded"));

            PVOID sym = nullptr;

#if IA_PLATFORM_WINDOWS
            sym = CAST(GetProcAddress(CAST(m_handle, HMODULE), name.c_str()), PVOID);
            if (!sym)
                return tl::make_unexpected(GetWindowsError());
#else
            // Clear any previous error
            dlerror();
            sym = dlsym(m_handle, name.c_str());
            const char *err = dlerror();
            if (err)
                return tl::make_unexpected(String(err));
#endif

            return sym;
        }

        // Template helper for casting
        template<typename FuncT> tl::expected<FuncT, String> GetFunction(CONST String &name) CONST
        {
            auto res = GetSymbol(name);
            if (!res)
                return tl::make_unexpected(res.error());
            return REINTERPRET(*res, FuncT);
        }

        VOID Unload()
        {
            if (m_handle)
            {
#if IA_PLATFORM_WINDOWS
                FreeLibrary(CAST(m_handle, HMODULE));
#else
                dlclose(m_handle);
#endif
                m_handle = nullptr;
            }
        }

        BOOL IsLoaded() CONST
        {
            return m_handle != nullptr;
        }

      private:
        PVOID m_handle;

#if IA_PLATFORM_WINDOWS
        STATIC String GetWindowsError()
        {
            DWORD errorID = ::GetLastError();
            if (errorID == 0)
                return String();

            LPSTR messageBuffer = nullptr;
            size_t size = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                errorID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &messageBuffer, 0, NULL);

            String message(messageBuffer, size);
            LocalFree(messageBuffer);
            return String("Win32 Error: ") + message;
        }
#endif
    };
} // namespace IACore