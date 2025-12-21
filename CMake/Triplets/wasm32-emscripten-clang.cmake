set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)

set(VCPKG_C_FLAGS "-msimd128 -pthread")
set(VCPKG_CXX_FLAGS "-msimd128 -pthread")
