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

namespace IACore
{
    // Must be called from main thread
    VOID Initialize();
    // Must be called from same thread as Initialize
    VOID Terminate();

    UINT64 GetUnixTime();
    UINT64 GetTicksCount();
    FLOAT64 GetSecondsCount();

    FLOAT32 GetRandom();
    UINT32 GetRandom(IN UINT32 seed);
    INT64 GetRandom(IN INT64 min, IN INT64 max);

    BOOL IsMainThread();
    VOID Sleep(IN UINT64 milliseconds);
} // namespace IACore

#endif
