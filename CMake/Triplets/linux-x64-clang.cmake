set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_C_FLAGS "-mavx2 -mfma -mxsave -msse4.2")
set(VCPKG_CXX_FLAGS "-mavx2 -mfma -mxsave -msse4.2")

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../Toolchains/linux-x64.cmake")