include(FetchContent)

find_package(OpenSSL 3.0.0 REQUIRED)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Force static libs")

set(HWY_ENABLE_TESTS OFF CACHE BOOL "Disable Highway tests" FORCE)
set(HWY_ENABLE_EXAMPLES OFF CACHE BOOL "Disable Highway examples" FORCE)
set(HWY_ENABLE_CONTRIB OFF CACHE BOOL "Disable Highway contrib" FORCE)
set(HWY_ENABLE_INSTALL OFF CACHE BOOL "Disable Highway install rules" FORCE)

set(ZLIB_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(WITH_GZFILEOP ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    Auxid
    GIT_REPOSITORY https://github.com/I-A-S/Auxid
    GIT_TAG        main
    OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
    GIT_TAG        2.1.6
    OVERRIDE_FIND_PACKAGE
)

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG        v1.5.5
    SOURCE_SUBDIR  build/cmake
    OVERRIDE_FIND_PACKAGE
)

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
    unordered_dense
    GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
    GIT_TAG        v4.8.1
    SYSTEM
    EXCLUDE_FROM_ALL
)

set(PUGIXML_NO_EXCEPTIONS ON)

FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG        v1.15
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  highway
  GIT_REPOSITORY https://github.com/google/highway.git
  GIT_TAG        1.3.0
  SYSTEM
)

set(MI_OVERRIDE ON CACHE BOOL "" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_DEBUG OFF CACHE BOOL "" FORCE)
set(MI_SHOW_ERRORS OFF CACHE BOOL "" FORCE)

set(EXPECTED_BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(HTTPLIB_REQUIRE_OPENSSL ON CACHE BOOL "" FORCE)
set(HTTPLIB_REQUIRE_ZLIB OFF CACHE BOOL "" FORCE)
set(HTTPLIB_NO_EXCEPTIONS ON CACHE BOOL "" FORCE)
set(HTTPLIB_COMPILE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_TEST OFF CACHE BOOL "" FORCE)
set(HTTPLIB_EXAMPLE OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(zlib zstd)

target_include_directories(libzstd_static INTERFACE
    $<BUILD_INTERFACE:${zstd_SOURCE_DIR}/lib>
)

if(NOT TARGET zstd::libzstd)
    add_library(zstd::libzstd ALIAS libzstd_static)
endif()

FetchContent_MakeAvailable(Auxid httplib pugixml nlohmann_json glaze simdjson unordered_dense mimalloc highway)

if(NOT TARGET simdjson::simdjson)
    add_library(simdjson::simdjson ALIAS simdjson)
endif()

target_compile_options(hwy PRIVATE -w)
target_compile_options(libzstd_static PRIVATE -w)
