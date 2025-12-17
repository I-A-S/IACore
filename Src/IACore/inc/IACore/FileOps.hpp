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

#include <IACore/StreamReader.hpp>
#include <IACore/StreamWriter.hpp>

namespace IACore
{
    class FileOps
    {
      public:
        STATIC FilePath NormalizeExecutablePath(IN CONST FilePath &path);

      public:
        STATIC VOID UnmapFile(IN PCUINT8 mappedPtr);
        STATIC Expected<PCUINT8, String> MapFile(IN CONST FilePath &path, OUT SIZE_T &size);

        // @param `isOwner` TRUE to allocate/truncate. FALSE to just open.
        STATIC Expected<PUINT8, String> MapSharedMemory(IN CONST String &name, IN SIZE_T size, IN BOOL isOwner);
        STATIC VOID UnlinkSharedMemory(IN CONST String &name);

        STATIC Expected<StreamReader, String> StreamFromFile(IN CONST FilePath &path);
        STATIC Expected<StreamWriter, String> StreamToFile(IN CONST FilePath &path, IN BOOL overwrite = false);

        STATIC Expected<String, String> ReadTextFile(IN CONST FilePath &path);
        STATIC Expected<Vector<UINT8>, String> ReadBinaryFile(IN CONST FilePath &path);
        STATIC Expected<SIZE_T, String> WriteTextFile(IN CONST FilePath &path, IN CONST String &contents,
                                                      IN BOOL overwrite = false);
        STATIC Expected<SIZE_T, String> WriteBinaryFile(IN CONST FilePath &path, IN Span<UINT8> contents,
                                                        IN BOOL overwrite = false);

      private:
        STATIC UnorderedMap<PCUINT8, Tuple<PVOID, PVOID, PVOID>> s_mappedFiles;
    };
} // namespace IACore