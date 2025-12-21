set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_C_FLAGS "--gcc-toolchain=/usr -march=armv8-a+simd")
set(VCPKG_CXX_FLAGS " --gcc-toolchain=/usr -march=armv8-a+simd")

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../Toolchains/linux-arm64.cmake")
