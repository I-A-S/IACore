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

#include <IACore/FileOps.hpp>
#include <cerrno>
#include <cstdio>

#if IA_PLATFORM_UNIX
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

namespace IACore
{

  Mut<HashMap<const u8 *, std::tuple<void *, void *, void *>>> FileOps::s_mapped_files;

  auto FileOps::unmap_file(const u8 *mapped_ptr) -> void
  {
    if (!s_mapped_files.contains(mapped_ptr))
    {
      return;
    }

    Mut<decltype(s_mapped_files)::iterator> it = s_mapped_files.find(mapped_ptr);
    const std::tuple<void *, void *, void *> handles = it->second;
    s_mapped_files.erase(it);

#if IA_PLATFORM_WINDOWS
    ::UnmapViewOfFile(std::get<1>(handles));
    ::CloseHandle(static_cast<HANDLE>(std::get<2>(handles)));

    const HANDLE handle = static_cast<HANDLE>(std::get<0>(handles));
    if (handle != INVALID_HANDLE_VALUE)
    {
      ::CloseHandle(handle);
    }
#elif IA_PLATFORM_UNIX
    ::munmap(std::get<1>(handles), (usize) std::get<2>(handles));
    const i32 fd = (i32) ((u64) std::get<0>(handles));
    if (fd != -1)
    {
      ::close(fd);
    }
#endif
  }

  auto FileOps::map_shared_memory(Ref<String> name, const usize size, const bool is_owner) -> Result<u8 *>
  {
#if IA_PLATFORM_WINDOWS
    const int wchars_num = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, NULL, 0);
    Mut<std::wstring> w_name(wchars_num, 0);
    MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, &w_name[0], wchars_num);

    Mut<HANDLE> h_map = NULL;
    if (is_owner)
    {
      h_map = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD) (size >> 32),
                                 (DWORD) (size & 0xFFFFFFFF), w_name.c_str());
    }
    else
    {
      h_map = OpenFileMappingW(FILE_MAP_ALL_ACCESS, false, w_name.c_str());
    }

    if (h_map == NULL)
    {
      return fail("Failed to {} shared memory '{}'", is_owner ? "owner" : "consumer", name);
    }

    Mut<u8 *> result = static_cast<u8 *>(MapViewOfFile(h_map, FILE_MAP_ALL_ACCESS, 0, 0, size));
    if (result == NULL)
    {
      CloseHandle(h_map);
      return fail("Failed to map view of shared memory '{}'", name);
    }

    s_mapped_files[result] = std::make_tuple((void *) INVALID_HANDLE_VALUE, (void *) result, (void *) h_map);
    return result;

#elif IA_PLATFORM_UNIX
    Mut<int> fd = -1;
    if (is_owner)
    {
      fd = shm_open(name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
      if (fd != -1)
      {
        if (ftruncate(fd, size) == -1)
        {
          close(fd);
          shm_unlink(name.c_str());
          return fail("Failed to truncate shared memory '{}'", name);
        }
      }
    }
    else
    {
      fd = shm_open(name.c_str(), O_RDWR, 0666);
    }

    if (fd == -1)
    {
      return fail("Failed to {} shared memory '{}'", is_owner ? "owner" : "consumer", name);
    }

    Mut<void *> addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
    {
      close(fd);
      return fail("Failed to mmap shared memory '{}'", name);
    }

    Mut<u8 *> result = static_cast<u8 *>(addr);

    s_mapped_files[result] = std::make_tuple((void *) ((u64) fd), (void *) addr, (void *) size);
    return result;
#endif
  }

  auto FileOps::unlink_shared_memory(Ref<String> name) -> void
  {
    if (name.empty())
    {
      return;
    }
#if IA_PLATFORM_UNIX
    shm_unlink(name.c_str());
#endif
  }

  auto FileOps::map_file(Ref<Path> path, MutRef<usize> size) -> Result<const u8 *>
  {
#if IA_PLATFORM_WINDOWS
    const HANDLE handle = CreateFileA(path.string().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (handle == INVALID_HANDLE_VALUE)
    {
      return fail("Failed to open {} for memory mapping", path.string());
    }

    Mut<LARGE_INTEGER> file_size;
    if (!GetFileSizeEx(handle, &file_size))
    {
      CloseHandle(handle);
      return fail("Failed to get size of {} for memory mapping", path.string());
    }
    size = static_cast<usize>(file_size.QuadPart);
    if (size == 0)
    {
      CloseHandle(handle);
      return fail("Failed to get size of {} for memory mapping", path.string());
    }

    Mut<HANDLE> h_map = CreateFileMappingW(handle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (h_map == NULL)
    {
      CloseHandle(handle);
      return fail("Failed to memory map {}", path.string());
    }

    const u8 *result = static_cast<const u8 *>(MapViewOfFile(h_map, FILE_MAP_READ, 0, 0, 0));
    if (result == NULL)
    {
      CloseHandle(handle);
      CloseHandle(h_map);
      return fail("Failed to memory map {}", path.string());
    }
    s_mapped_files[result] = std::make_tuple((void *) handle, (void *) const_cast<u8 *>(result), (void *) h_map);
    return result;

#elif IA_PLATFORM_UNIX
    const int handle = open(path.string().c_str(), O_RDONLY);
    if (handle == -1)
    {
      return fail("Failed to open {} for memory mapping", path.string());
    }
    Mut<struct stat> sb;
    if (fstat(handle, &sb) == -1)
    {
      close(handle);
      return fail("Failed to get stats of {} for memory mapping", path.string());
    }
    size = static_cast<usize>(sb.st_size);
    if (size == 0)
    {
      close(handle);
      return fail("Failed to get size of {} for memory mapping", path.string());
    }
    Mut<void *> addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, handle, 0);
    if (addr == MAP_FAILED)
    {
      close(handle);
      return fail("Failed to memory map {}", path.string());
    }
    const u8 *result = static_cast<const u8 *>(addr);
    madvise(addr, size, MADV_SEQUENTIAL);
    s_mapped_files[result] = std::make_tuple((void *) ((u64) handle), (void *) addr, (void *) size);
    return result;
#endif
  }

  auto FileOps::stream_to_file(Ref<Path> path, const bool overwrite) -> Result<StreamWriter>
  {
    if (!overwrite && std::filesystem::exists(path))
    {
      return fail("File already exists: {}", path.string());
    }
    return StreamWriter::create_from_file(path);
  }

  auto FileOps::stream_from_file(Ref<Path> path) -> Result<StreamReader>
  {
    if (!std::filesystem::exists(path))
    {
      return fail("File does not exist: {}", path.string());
    }
    return StreamReader::create_from_file(path);
  }

  auto FileOps::read_text_file(Ref<Path> path) -> Result<String>
  {
    Mut<FILE *> f = fopen(path.string().c_str(), "r");
    if (!f)
    {
      return fail("Failed to open file: {}", path.string());
    }
    Mut<String> result;
    fseek(f, 0, SEEK_END);
    const long len = ftell(f);
    if (len > 0)
    {
      result.resize(static_cast<usize>(len));
      fseek(f, 0, SEEK_SET);
      const usize read_length = fread(result.data(), 1, result.size(), f);
      result.resize(read_length);
    }
    fclose(f);
    return result;
  }

  auto FileOps::read_binary_file(Ref<Path> path) -> Result<Vec<u8>>
  {
    Mut<FILE *> f = fopen(path.string().c_str(), "rb");
    if (!f)
    {
      return fail("Failed to open file: {}", path.string());
    }
    Mut<Vec<u8>> result;
    fseek(f, 0, SEEK_END);
    const long len = ftell(f);
    if (len > 0)
    {
      result.resize(static_cast<usize>(len));
      fseek(f, 0, SEEK_SET);
      fread(result.data(), 1, result.size(), f);
    }
    fclose(f);
    return result;
  }

  auto FileOps::write_text_file(Ref<Path> path, Ref<String> contents, const bool overwrite) -> Result<usize>
  {
    const char *mode = overwrite ? "w" : "wx";
    Mut<FILE *> f = fopen(path.string().c_str(), mode);
    if (!f)
    {
      if (!overwrite && errno == EEXIST)
      {
        return fail("File already exists: {}", path.string());
      }
      return fail("Failed to write to file: {}", path.string());
    }
    const usize result = fwrite(contents.data(), 1, contents.size(), f);
    fclose(f);
    return result;
  }

  auto FileOps::write_binary_file(Ref<Path> path, const Span<const u8> contents, const bool overwrite) -> Result<usize>
  {
    const char *mode = overwrite ? "w" : "wx";
    Mut<FILE *> f = fopen(path.string().c_str(), mode);
    if (!f)
    {
      if (!overwrite && errno == EEXIST)
      {
        return fail("File already exists: {}", path.string());
      }
      return fail("Failed to write to file: {}", path.string());
    }
    const usize result = fwrite(contents.data(), 1, contents.size(), f);
    fclose(f);
    return result;
  }

  auto FileOps::normalize_executable_path(Ref<Path> path) -> Path
  {
    Mut<Path> result = path;

#if IA_PLATFORM_WINDOWS
    if (!result.has_extension())
    {
      result.replace_extension(".exe");
    }
#elif IA_PLATFORM_UNIX
    if (result.extension() == ".exe")
    {
      result.replace_extension("");
    }

    if (result.is_relative())
    {
      Mut<String> path_str = result.string();
      if (!path_str.starts_with("./") && !path_str.starts_with("../"))
      {
        result = "./" + path_str;
      }
    }
#endif
    return result;
  }

  auto FileOps::native_open_file(Ref<Path> path, const FileAccess access, const FileMode mode, const u32 permissions)
      -> Result<NativeFileHandle>
  {
#if IA_PLATFORM_WINDOWS
    AU_UNUSED(permissions);

    Mut<DWORD> dw_access = 0;
    Mut<DWORD> dw_share = FILE_SHARE_READ;
    Mut<DWORD> dw_disposition = 0;
    Mut<DWORD> dw_flags_and_attributes = FILE_ATTRIBUTE_NORMAL;

    switch (access)
    {
    case FileAccess::Read:
      dw_access = GENERIC_READ;
      break;
    case FileAccess::Write:
      dw_access = GENERIC_WRITE;
      break;
    case FileAccess::ReadWrite:
      dw_access = GENERIC_READ | GENERIC_WRITE;
      break;
    }

    switch (mode)
    {
    case FileMode::OpenExisting:
      dw_disposition = OPEN_EXISTING;
      break;
    case FileMode::OpenAlways:
      dw_disposition = OPEN_ALWAYS;
      break;
    case FileMode::CreateNew:
      dw_disposition = CREATE_NEW;
      break;
    case FileMode::CreateAlways:
      dw_disposition = CREATE_ALWAYS;
      break;
    case FileMode::TruncateExisting:
      dw_disposition = TRUNCATE_EXISTING;
      break;
    }

    Mut<HANDLE> h_file =
        CreateFileA(path.string().c_str(), dw_access, dw_share, NULL, dw_disposition, dw_flags_and_attributes, NULL);

    if (h_file == INVALID_HANDLE_VALUE)
    {
      return fail("Failed to open file '{}': {}", path.string(), GetLastError());
    }

    return h_file;

#elif IA_PLATFORM_UNIX
    Mut<int> flags = 0;

    switch (access)
    {
    case FileAccess::Read:
      flags = O_RDONLY;
      break;
    case FileAccess::Write:
      flags = O_WRONLY;
      break;
    case FileAccess::ReadWrite:
      flags = O_RDWR;
      break;
    }

    switch (mode)
    {
    case FileMode::OpenExisting:
      break;
    case FileMode::OpenAlways:
      flags |= O_CREAT;
      break;
    case FileMode::CreateNew:
      flags |= O_CREAT | O_EXCL;
      break;
    case FileMode::CreateAlways:
      flags |= O_CREAT | O_TRUNC;
      break;
    case FileMode::TruncateExisting:
      flags |= O_TRUNC;
      break;
    }

    Mut<int> fd = open(path.string().c_str(), flags, permissions);

    if (fd == -1)
    {
      return fail("Failed to open file '{}': {}", path.string(), errno);
    }

    return fd;
#endif
  }

  auto FileOps::native_close_file(const NativeFileHandle handle) -> void
  {
    if (handle == INVALID_FILE_HANDLE)
    {
      return;
    }

#if IA_PLATFORM_WINDOWS
    CloseHandle(handle);
#elif IA_PLATFORM_UNIX
    close(handle);
#endif
  }

  FileOps::MemoryMappedRegion::~MemoryMappedRegion()
  {
    unmap();
  }

  FileOps::MemoryMappedRegion::MemoryMappedRegion(ForwardRef<MemoryMappedRegion> other) noexcept
  {
    *this = std::move(other);
  }

  auto FileOps::MemoryMappedRegion::operator=(ForwardRef<MemoryMappedRegion> other) noexcept
      -> MutRef<MemoryMappedRegion>
  {
    if (this != &other)
    {
      unmap();
      m_ptr = other.m_ptr;
      m_size = other.m_size;
#if IA_PLATFORM_WINDOWS
      m_map_handle = other.m_map_handle;
      other.m_map_handle = NULL;
#endif
      other.m_ptr = nullptr;
      other.m_size = 0;
    }
    return *this;
  }

  auto FileOps::MemoryMappedRegion::map(const NativeFileHandle handle, const u64 offset, const usize size)
      -> Result<void>
  {
    unmap();

    if (handle == INVALID_FILE_HANDLE)
    {
      return fail("Invalid file handle provided to Map");
    }

    if (size == 0)
    {
      return fail("Cannot map region of size 0");
    }

#if IA_PLATFORM_WINDOWS
    Mut<LARGE_INTEGER> file_size;
    if (!GetFileSizeEx(handle, &file_size))
    {
      return fail("Failed to get file size");
    }

    const u64 end_offset = offset + size;
    if (static_cast<u64>(file_size.QuadPart) < end_offset)
    {
      Mut<LARGE_INTEGER> new_size;
      new_size.QuadPart = static_cast<LONGLONG>(end_offset);
      if (!SetFilePointerEx(handle, new_size, NULL, FILE_BEGIN))
      {
        return fail("Failed to seek to new end of file");
      }

      if (!SetEndOfFile(handle))
      {
        return fail("Failed to extend file for mapping");
      }
    }

    m_map_handle = CreateFileMappingW(handle, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (m_map_handle == NULL)
    {
      return fail("CreateFileMapping failed: {}", GetLastError());
    }

    const DWORD offset_high = static_cast<DWORD>(offset >> 32);
    const DWORD offset_low = static_cast<DWORD>(offset & 0xFFFFFFFF);

    m_ptr = static_cast<u8 *>(MapViewOfFile(m_map_handle, FILE_MAP_WRITE, offset_high, offset_low, size));
    if (m_ptr == NULL)
    {
      CloseHandle(m_map_handle);
      m_map_handle = NULL;
      return fail("MapViewOfFile failed (Offset: {}, Size: {}): {}", offset, size, GetLastError());
    }
    m_size = size;

#elif IA_PLATFORM_UNIX
    Mut<struct stat> sb;
    if (fstat(handle, &sb) == -1)
    {
      return fail("Failed to fstat file");
    }

    const u64 end_offset = offset + size;
    if (static_cast<u64>(sb.st_size) < end_offset)
    {
      if (ftruncate(handle, static_cast<off_t>(end_offset)) == -1)
      {
        return fail("Failed to ftruncate (extend) file");
      }
    }

    Mut<void *> ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle, static_cast<off_t>(offset));
    if (ptr == MAP_FAILED)
    {
      return fail("mmap failed: {}", errno);
    }

    m_ptr = static_cast<u8 *>(ptr);
    m_size = size;

    madvise(m_ptr, m_size, MADV_SEQUENTIAL);
#endif

    return {};
  }

  auto FileOps::MemoryMappedRegion::unmap() -> void
  {
    if (!m_ptr)
    {
      return;
    }

#if IA_PLATFORM_WINDOWS
    UnmapViewOfFile(m_ptr);
    if (m_map_handle)
    {
      CloseHandle(m_map_handle);
      m_map_handle = NULL;
    }
#elif IA_PLATFORM_UNIX
    munmap(m_ptr, m_size);
#endif
    m_ptr = nullptr;
    m_size = 0;
  }

  auto FileOps::MemoryMappedRegion::flush() -> void
  {
    if (!m_ptr)
    {
      return;
    }

#if IA_PLATFORM_WINDOWS
    FlushViewOfFile(m_ptr, m_size);
#elif IA_PLATFORM_UNIX
    msync(m_ptr, m_size, MS_SYNC);
#endif
  }

} // namespace IACore