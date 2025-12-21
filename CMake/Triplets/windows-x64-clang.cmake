set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Windows)

set(VCPKG_C_FLAGS "/arch:AVX2 /clang:-mfma")
set(VCPKG_CXX_FLAGS "/arch:AVX2 /clang:-mfma")

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../Toolchains/windows-x64.cmake")
