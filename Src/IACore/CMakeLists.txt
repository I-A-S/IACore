set(SRC_FILES
    "imp/cpp/CLI.cpp"
    "imp/cpp/IPC.cpp"
    "imp/cpp/XML.cpp"
    "imp/cpp/SIMD.cpp"
    "imp/cpp/JSON.cpp"
    "imp/cpp/IACore.cpp"
    "imp/cpp/Logger.cpp"
    "imp/cpp/Utils.cpp"
    "imp/cpp/FileOps.cpp"
    "imp/cpp/DataOps.cpp"
    "imp/cpp/AsyncOps.cpp"
    "imp/cpp/Platform.cpp"
    "imp/cpp/SocketOps.cpp"
    "imp/cpp/StringOps.cpp"
    "imp/cpp/ProcessOps.cpp"
    "imp/cpp/StreamReader.cpp"
    "imp/cpp/StreamWriter.cpp"

    "imp/cpp/Http/Common.cpp"
    "imp/cpp/Http/Client.cpp"
)

add_library(IACore STATIC ${SRC_FILES})

target_include_directories(IACore PUBLIC inc/)
target_include_directories(IACore PRIVATE imp/hpp/)

target_link_libraries(IACore PUBLIC
    hwy
    zlib   
    Auxid
    zstd::libzstd
    glaze::glaze
    mimalloc-static
    pugixml::pugixml
    simdjson::simdjson
    nlohmann_json::nlohmann_json
    unordered_dense::unordered_dense
)

target_link_libraries(IACore PRIVATE 
    ${LIBCURL_TARGET}
)

if(WIN32)
    if(MSVC)
        target_link_options(IACore PUBLIC "/INCLUDE:mi_version")
    else()
        target_link_options(IACore PUBLIC "")
    endif()

    target_link_libraries(IACore PRIVATE ws2_32 crypt32)
endif()

target_precompile_headers(IACore PUBLIC inc/IACore/PCH.hpp)

if(MSVC)
    set(NO_EXCEPT_FLAG "/EHs-c-")
else()
    set(NO_EXCEPT_FLAG "-fno-exceptions")
endif()
target_compile_options(IACore PRIVATE ${NO_EXCEPT_FLAG})
target_compile_options(IACore INTERFACE
    $<$<NOT:$<BOOL:$<TARGET_PROPERTY:USE_EXCEPTIONS>>>:${NO_EXCEPT_FLAG}>
)

define_property(TARGET PROPERTY USE_EXCEPTIONS
    BRIEF_DOCS "If ON, this target is allowed to use C++ exceptions."
    FULL_DOCS "Prevents IACore from propagating -fno-exceptions to this target."
)

target_compile_definitions(IACore PRIVATE 
    CPPHTTPLIB_OPENSSL_SUPPORT 
    CPPHTTPLIB_ZLIB_SUPPORT
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
)

target_compile_definitions(IACore PUBLIC 
    $<$<CONFIG:Debug>:__IA_DEBUG=1>
    $<$<CONFIG:Release>:__IA_DEBUG=0>
)

if(IACORE_ARCH_X64)
    if(MSVC)
        target_compile_options(IACore INTERFACE /arch:AVX2)

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(IACore INTERFACE /clang:-maes)
        endif()
        
    else()
        target_compile_options(IACore INTERFACE -mavx2 -mfma -mpclmul -maes -mbmi -mbmi2 -mf16c)
    endif()
    target_compile_definitions(IACore INTERFACE HWY_BASELINE_TARGETS=HWY_AVX2)
elseif(IACORE_ARCH_ARM64)
    if(NOT MSVC)
        target_compile_options(IACore INTERFACE -march=armv8-a+simd)
    endif()
    target_compile_definitions(IACore INTERFACE HWY_BASELINE_TARGETS=HWY_NEON)
elseif(IACORE_ARCH_WASM)
    target_compile_options(IACore INTERFACE -msimd128)
    target_compile_definitions(IACore INTERFACE HWY_BASELINE_TARGETS=HWY_WASM)
endif()