macro(iacore_setup_project)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

    if(MSVC)
        add_compile_options(/W4)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-Wno-c++98-compat -Wno-c++98-compat-pedantic)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(-Wall -Wextra -Wpedantic -Wno-language-extension-token)
    endif()

    add_compile_options(-Wno-missing-field-initializers -Wno-missing-designated-field-initializers)

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64|x86_64|AMD64")
        set(IACORE_ARCH_X64 TRUE CACHE INTERNAL "")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
        set(IACORE_ARCH_ARM64 TRUE CACHE INTERNAL "")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "wasm32|emscripten")
        set(IACORE_ARCH_WASM TRUE CACHE INTERNAL "")
    endif()

endmacro()
