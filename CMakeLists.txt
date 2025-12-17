cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

project(IACore)

enable_language(C)

include(CMake/FindDeps.cmake)

# Default to ON if root, OFF if dependency
option(IACore_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})

message(STATUS "Configured IACore for Multi-Config (Debug/Release rules generated)")

message(STATUS "Detected Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
# Check if the compiler is MSVC (cl.exe), but allow Clang acting like MSVC (clang-cl)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR 
            "\n\n"
            "-------------------------------------------------------------\n"
            "CRITICAL ERROR: Unsupported Compiler Detected (MSVC/cl.exe)\n"
            "-------------------------------------------------------------\n"
            "IACore requires GCC or Clang to compile.\n"
            "For compiling with IACore on Windows, please use Clang Tools for MSVC.\n"
            "-------------------------------------------------------------\n"
        )
    endif()
endif()

if(MSVC)
    add_compile_options(/W4)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(
            -Wno-c++98-compat 
            -Wno-c++98-compat-pedantic
        )
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options( 
        -Wall -Wextra -Wpedantic
        -Wno-language-extension-token 
    )
endif()

add_subdirectory(Src/)

if(IACore_BUILD_TESTS)
    add_subdirectory(Tests)
endif()

# Local Development Sandbox (Not tracked in git)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Sandbox")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Sandbox")
endif()
