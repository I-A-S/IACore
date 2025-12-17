// IACore-OSS; The Core Library for All IA Open Source Projects 
// Copyright (C) 2025 IAS (ias@iasoft.dev)
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#if defined(__cplusplus)
#error "CRITICAL: This file MUST be compiled as C to test ABI compatibility."
#endif

#include <IACore/IACore.hpp>

#if TRUE != 1
#error "TRUE macro is broken in C mode"
#endif

int main(void) {
    IA_VERSION_TYPE version = IA_MAKE_VERSION(1, 0, 0);

    IA_ASSERT(version > 0);

    UNUSED(version);

    int32_t myNumber = 10;
    
    (void)myNumber; 

    return 0;
}
