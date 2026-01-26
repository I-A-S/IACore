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
#include <IACore/StreamReader.hpp>
#include <IACore/StreamWriter.hpp>
#include <tuple>

#if IA_PLATFORM_WINDOWS
using NativeFileHandle = HANDLE;
static const NativeFileHandle INVALID_FILE_HANDLE = INVALID_HANDLE_VALUE;
#else
using NativeFileHandle = int;
static const NativeFileHandle INVALID_FILE_HANDLE = -1;
#endif

namespace IACore
{

  class FileOps
  {
public:
    class MemoryMappedRegion;

    enum class FileAccess : u8
    {
      Read,     // Read-only
      Write,    // Write-only
      ReadWrite // Read and Write
    };

    enum class FileMode : u8
    {
      OpenExisting,    // Fails if file doesn't exist
      OpenAlways,      // Opens if exists, creates if not
      CreateNew,       // Fails if file exists
      CreateAlways,    // Overwrites existing
      TruncateExisting // Opens existing and clears it
    };

    static auto native_open_file(Ref<Path> path, const FileAccess access, const FileMode mode,
                                 const u32 permissions = 0644) -> Result<NativeFileHandle>;

    static auto native_close_file(const NativeFileHandle handle) -> void;

public:
    static auto normalize_executable_path(Ref<Path> path) -> Path;

public:
    static auto unmap_file(const u8 *mapped_ptr) -> void;

    static auto map_file(Ref<Path> path, MutRef<usize> size) -> Result<const u8 *>;

    // @param `is_owner` true to allocate/truncate. false to just open.
    static auto map_shared_memory(Ref<String> name, const usize size, const bool is_owner) -> Result<u8 *>;

    static auto unlink_shared_memory(Ref<String> name) -> void;

    static auto stream_from_file(Ref<Path> path) -> Result<StreamReader>;

    static auto stream_to_file(Ref<Path> path, const bool overwrite = false) -> Result<StreamWriter>;

    static auto read_text_file(Ref<Path> path) -> Result<String>;

    static auto read_binary_file(Ref<Path> path) -> Result<Vec<u8>>;

    static auto write_text_file(Ref<Path> path, Ref<String> contents, const bool overwrite = false) -> Result<usize>;

    static auto write_binary_file(Ref<Path> path, const Span<const u8> contents, const bool overwrite = false)
        -> Result<usize>;

private:
    static Mut<HashMap<const u8 *, std::tuple<void *, void *, void *>>> s_mapped_files;
  };

  class FileOps::MemoryMappedRegion
  {
public:
    MemoryMappedRegion() = default;
    ~MemoryMappedRegion();

    MemoryMappedRegion(Ref<MemoryMappedRegion>) = delete;
    auto operator=(Ref<MemoryMappedRegion>) -> MemoryMappedRegion & = delete;

    MemoryMappedRegion(ForwardRef<MemoryMappedRegion> other) noexcept;
    auto operator=(ForwardRef<MemoryMappedRegion> other) noexcept -> MemoryMappedRegion &;

    auto map(const NativeFileHandle handle, const u64 offset, const usize size) -> Result<void>;

    auto unmap() -> void;
    auto flush() -> void;

    [[nodiscard]] auto get_ptr() const -> u8 *
    {
      return m_ptr;
    }

    [[nodiscard]] auto get_size() const -> usize
    {
      return m_size;
    }

    [[nodiscard]] auto is_valid() const -> bool
    {
      return m_ptr != nullptr;
    }

private:
    Mut<u8 *> m_ptr = nullptr;
    Mut<usize> m_size = 0;

#if IA_PLATFORM_WINDOWS
    Mut<HANDLE> m_map_handle = NULL;
#endif
  };

} // namespace IACore