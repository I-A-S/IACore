cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(IACore LANGUAGES C CXX)
enable_testing()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(IACORE_IS_TOP_LEVEL ON)
else()
    set(IACORE_IS_TOP_LEVEL OFF)
endif()

set(IACORE_ROOT "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")
set(IACORE_CMAKE_DIR "${IACORE_ROOT}/CMake" CACHE INTERNAL "")
list(APPEND CMAKE_MODULE_PATH "${IACORE_CMAKE_DIR}")

include(IAProjectConfig)
iacore_setup_project()

message(STATUS "[IACore] Detected Compiler: ${CMAKE_CXX_COMPILER_ID}")

if (MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR 
        "\n-------------------------------------------------------------\n"
        "CRITICAL ERROR: Unsupported Compiler (MSVC/cl.exe)\n"
        "IACore requires GCC or Clang. On Windows, use 'Clang-cl' or MinGW.\n"
        "-------------------------------------------------------------\n"
    )
endif()

include(FindDeps)

option(IACore_BUILD_TESTS "Build unit tests" ${IACORE_IS_TOP_LEVEL})

add_subdirectory(Src)

if(IACore_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
