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

#include <IACore/FileOps.hpp>

namespace IACore
{
    UnorderedMap<PCUINT8, Tuple<PVOID, PVOID, PVOID>> FileOps::s_mappedFiles;

    VOID FileOps::UnmapFile(IN PCUINT8 mappedPtr)
    {
        if (!s_mappedFiles.contains(mappedPtr))
            return;
        const auto handles = s_mappedFiles.extract(mappedPtr)->second;
#if IA_PLATFORM_WINDOWS
        ::UnmapViewOfFile(std::get<1>(handles));
        ::CloseHandle(std::get<2>(handles));

        if (std::get<0>(handles) != INVALID_HANDLE_VALUE)
            ::CloseHandle(std::get<0>(handles));
#elif IA_PLATFORM_UNIX
        ::munmap(std::get<1>(handles), (SIZE_T) std::get<2>(handles));
        const auto fd = (INT32) ((UINT64) std::get<0>(handles));
        if (fd != -1)
            ::close(fd);
#endif
    }

    Expected<PUINT8, String> FileOps::MapSharedMemory(IN CONST String &name, IN SIZE_T size, IN BOOL isOwner)
    {
#if IA_PLATFORM_WINDOWS
        int wchars_num = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, NULL, 0);
        std::wstring wName(wchars_num, 0);
        MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, &wName[0], wchars_num);

        HANDLE hMap = NULL;
        if (isOwner)
            hMap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD) (size >> 32),
                                      (DWORD) (size & 0xFFFFFFFF), wName.c_str());
        else
            hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, wName.c_str());

        if (hMap == NULL)
            return MakeUnexpected(
                std::format("Failed to {} shared memory '{}'", isOwner ? "owner" : "consumer", name.c_str()));

        const auto result = static_cast<PUINT8>(MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size));
        if (result == NULL)
        {
            CloseHandle(hMap);
            return MakeUnexpected(std::format("Failed to map view of shared memory '{}'", name.c_str()));
        }

        s_mappedFiles[result] = std::make_tuple((PVOID) INVALID_HANDLE_VALUE, (PVOID) result, (PVOID) hMap);
        return result;

#elif IA_PLATFORM_UNIX
        int fd = -1;
        if (isOwner)
        {
            fd = shm_open(name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
            if (fd != -1)
            {
                if (ftruncate(fd, size) == -1)
                {
                    close(fd);
                    shm_unlink(name.c_str());
                    return MakeUnexpected(std::format("Failed to truncate shared memory '{}'", name.c_str()));
                }
            }
        }
        else
            fd = shm_open(name.c_str(), O_RDWR, 0666);

        if (fd == -1)
            return MakeUnexpected(
                std::format("Failed to {} shared memory '{}'", isOwner ? "owner" : "consumer", name.c_str()));

        void *addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED)
        {
            close(fd);
            return MakeUnexpected(std::format("Failed to mmap shared memory '{}'", name.c_str()));
        }

        const auto result = static_cast<PUINT8>(addr);

        s_mappedFiles[result] = std::make_tuple((PVOID) ((UINT64) fd), (PVOID) addr, (PVOID) size);
        return result;

#endif
    }

    VOID FileOps::UnlinkSharedMemory(IN CONST String &name)
    {
        if (name.empty())
            return;
#if IA_PLATFORM_UNIX
        shm_unlink(name.c_str());
#endif
    }

    Expected<PCUINT8, String> FileOps::MapFile(IN CONST FilePath &path, OUT SIZE_T &size)
    {
#if IA_PLATFORM_WINDOWS

        const auto handle = CreateFileA(path.string().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (handle == INVALID_HANDLE_VALUE)
            return MakeUnexpected(std::format("Failed to open {} for memory mapping", path.string().c_str()));

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(handle, &fileSize))
        {
            CloseHandle(handle);
            return MakeUnexpected(std::format("Failed to get size of {} for memory mapping", path.string().c_str()));
        }
        size = static_cast<size_t>(fileSize.QuadPart);
        if (size == 0)
        {
            CloseHandle(handle);
            return MakeUnexpected(std::format("Failed to get size of {} for memory mapping", path.string().c_str()));
        }

        auto hmap = CreateFileMappingW(handle, NULL, PAGE_READONLY, 0, 0, NULL);
        if (hmap == NULL)
        {
            CloseHandle(handle);
            return MakeUnexpected(std::format("Failed to memory map {}", path.string().c_str()));
        }

        const auto result = static_cast<PCUINT8>(MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0));
        if (result == NULL)
        {
            CloseHandle(handle);
            CloseHandle(hmap);
            return MakeUnexpected(std::format("Failed to memory map {}", path.string().c_str()));
        }
        s_mappedFiles[result] = std::make_tuple((PVOID) handle, (PVOID) result, (PVOID) hmap);
        return result;

#elif IA_PLATFORM_UNIX

        const auto handle = open(path.string().c_str(), O_RDONLY);
        if (handle == -1)
            return MakeUnexpected(std::format("Failed to open {} for memory mapping", path.string().c_str()));
        struct stat sb;
        if (fstat(handle, &sb) == -1)
        {
            close(handle);
            return MakeUnexpected(std::format("Failed to get stats of {} for memory mapping", path.string().c_str()));
        }
        size = static_cast<size_t>(sb.st_size);
        if (size == 0)
        {
            close(handle);
            return MakeUnexpected(std::format("Failed to get size of {} for memory mapping", path.string().c_str()));
        }
        void *addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, handle, 0);
        if (addr == MAP_FAILED)
        {
            close(handle);
            return MakeUnexpected(std::format("Failed to memory map {}", path.string().c_str()));
        }
        const auto result = static_cast<PCUINT8>(addr);
        madvise(addr, size, MADV_SEQUENTIAL);
        s_mappedFiles[result] = std::make_tuple((PVOID) ((UINT64) handle), (PVOID) addr, (PVOID) size);
        return result;
#endif
    }

    Expected<StreamWriter, String> FileOps::StreamToFile(IN CONST FilePath &path, IN BOOL overwrite)
    {
        if (!overwrite && FileSystem::exists(path))
            return MakeUnexpected(std::format("File aready exists: {}", path.string().c_str()));
        return StreamWriter(path);
    }

    Expected<StreamReader, String> FileOps::StreamFromFile(IN CONST FilePath &path)
    {
        if (!FileSystem::exists(path))
            return MakeUnexpected(std::format("File does not exist: {}", path.string().c_str()));
        return StreamReader(path);
    }

    Expected<String, String> FileOps::ReadTextFile(IN CONST FilePath &path)
    {
        const auto f = fopen(path.string().c_str(), "r");
        if (!f)
            return MakeUnexpected(std::format("Failed to open file: {}", path.string().c_str()));
        String result;
        fseek(f, 0, SEEK_END);
        result.resize(ftell(f));
        fseek(f, 0, SEEK_SET);
        fread(result.data(), 1, result.size(), f);
        fclose(f);
        return result;
    }

    Expected<Vector<UINT8>, String> FileOps::ReadBinaryFile(IN CONST FilePath &path)
    {
        const auto f = fopen(path.string().c_str(), "rb");
        if (!f)
            return MakeUnexpected(std::format("Failed to open file: {}", path.string().c_str()));
        Vector<UINT8> result;
        fseek(f, 0, SEEK_END);
        result.resize(ftell(f));
        fseek(f, 0, SEEK_SET);
        fread(result.data(), 1, result.size(), f);
        fclose(f);
        return result;
    }

    Expected<SIZE_T, String> FileOps::WriteTextFile(IN CONST FilePath &path, IN CONST String &contents,
                                                    IN BOOL overwrite)
    {
        const char *mode = overwrite ? "w" : "wx";
        const auto f = fopen(path.string().c_str(), mode);
        if (!f)
        {
            if (!overwrite && errno == EEXIST)
                return MakeUnexpected(std::format("File already exists: {}", path.string().c_str()));
            return MakeUnexpected(std::format("Failed to write to file: {}", path.string().c_str()));
        }
        const auto result = fwrite(contents.data(), 1, contents.size(), f);
        fputc(0, f);
        fclose(f);
        return result;
    }

    Expected<SIZE_T, String> FileOps::WriteBinaryFile(IN CONST FilePath &path, IN Span<UINT8> contents,
                                                      IN BOOL overwrite)
    {
        const char *mode = overwrite ? "w" : "wx";
        const auto f = fopen(path.string().c_str(), mode);
        if (!f)
        {
            if (!overwrite && errno == EEXIST)
                return MakeUnexpected(std::format("File already exists: {}", path.string().c_str()));
            return MakeUnexpected(std::format("Failed to write to file: {}", path.string().c_str()));
        }
        const auto result = fwrite(contents.data(), 1, contents.size(), f);
        fclose(f);
        return result;
    }

    FilePath FileOps::NormalizeExecutablePath(IN CONST FilePath &path)
    {
        FilePath result = path;

#if IA_PLATFORM_WINDOWS
        if (!result.has_extension())
            result.replace_extension(".exe");

#elif IA_PLATFORM_UNIX
        if (result.extension() == ".exe")
            result.replace_extension("");

        if (result.is_relative())
        {
            String pathStr = result.string();
            if (!pathStr.starts_with("./") && !pathStr.starts_with("../"))
                result = "./" + pathStr;
        }
#endif
        return result;
    }
} // namespace IACore