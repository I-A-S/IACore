include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Force static libs")

find_package(zlib-ng CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(OpenSSL 3.0.0 REQUIRED)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.28.0
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0 
  SYSTEM
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG        v5.0.0
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG        v4.2.2
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    mimalloc
    GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
    GIT_TAG        v3.0.10
    SYSTEM
    EXCLUDE_FROM_ALL
    PATCH_COMMAND ${CMAKE_COMMAND} 
                  -DSOURCE_DIR=<SOURCE_DIR> 
                  -P ${CMAKE_CURRENT_SOURCE_DIR}/CMake/PatchMimalloc.cmake
)

FetchContent_Declare(
    tl-expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
    GIT_TAG        v1.3.1
    SYSTEM
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    unordered_dense
    GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
    GIT_TAG        v4.8.1
    SYSTEM
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG        v1.15
  SYSTEM
  EXCLUDE_FROM_ALL
)

set(MI_OVERRIDE ON CACHE BOOL "" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)

set(EXPECTED_BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(HTTPLIB_REQUIRE_OPENSSL ON CACHE BOOL "" FORCE)
set(HTTPLIB_REQUIRE_ZLIB OFF CACHE BOOL "" FORCE)
set(HTTPLIB_NO_EXCEPTIONS ON CACHE BOOL "" FORCE)
set(HTTPLIB_COMPILE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_TEST OFF CACHE BOOL "" FORCE)
set(HTTPLIB_EXAMPLE OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(httplib pugixml nlohmann_json glaze simdjson tl-expected unordered_dense mimalloc)
