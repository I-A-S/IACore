set(SRC_FILES
    "imp/cpp/IPC.cpp"
    "imp/cpp/XML.cpp"
    "imp/cpp/JSON.cpp"
    "imp/cpp/IACore.cpp"
    "imp/cpp/Logger.cpp"
    "imp/cpp/FileOps.cpp"
    "imp/cpp/DataOps.cpp"
    "imp/cpp/AsyncOps.cpp"
    "imp/cpp/Platform.cpp"
    "imp/cpp/SocketOps.cpp"
    "imp/cpp/StringOps.cpp"
    "imp/cpp/ProcessOps.cpp"
    "imp/cpp/HttpClient.cpp"
    "imp/cpp/StreamReader.cpp"
    "imp/cpp/StreamWriter.cpp"
)

add_library(IACore STATIC ${SRC_FILES})

target_include_directories(IACore PUBLIC inc/)
target_include_directories(IACore PRIVATE imp/hpp/)

target_link_libraries(IACore PUBLIC 
    zlib-ng::zlib    
    zstd::libzstd
    tl::expected 
    glaze::glaze
    pugixml::pugixml
    simdjson::simdjson
    nlohmann_json::nlohmann_json
    unordered_dense::unordered_dense
)

target_link_libraries(IACore PRIVATE 
    httplib::httplib
    OpenSSL::SSL 
    OpenSSL::Crypto
)

target_link_libraries(IACore PUBLIC mimalloc-static)

if(WIN32)
    if(MSVC)
        target_link_options(IACore PUBLIC "/INCLUDE:mi_version")
    else()
        target_link_options(IACore PUBLIC "")
    endif()
endif()

target_precompile_headers(IACore PUBLIC inc/IACore/PCH.hpp)

set(NO_EXCEPT_FLAG "$<IF:$<CXX_COMPILER_ID:MSVC>,/EHs-c-,-fno-exceptions>")
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
    NOMINMAX
)

target_compile_definitions(IACore PUBLIC 
    $<$<CONFIG:Debug>:__IA_DEBUG=1>
    $<$<CONFIG:Release>:__IA_DEBUG=0>
)
