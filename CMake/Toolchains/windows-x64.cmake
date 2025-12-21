set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)

set(CMAKE_LINKER lld-link)

string(APPEND CMAKE_C_FLAGS " /arch:AVX2 -fuse-ld=lld-link")
string(APPEND CMAKE_CXX_FLAGS " /arch:AVX2 -fuse-ld=lld-link")
