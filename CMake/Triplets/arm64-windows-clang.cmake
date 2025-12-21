set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Windows)

set(VCPKG_POLICY_EMPTY_PACKAGE enabled) 

set(VCPKG_C_FLAGS "--target=arm64-pc-windows-msvc")
set(VCPKG_CXX_FLAGS "--target=arm64-pc-windows-msvc")
