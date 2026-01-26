set(TEST_SOURCES
    AsyncOps.cpp
    CLI.cpp
    DataOps.cpp
    Environment.cpp
    FileOps.cpp
    IPC.cpp
    JSON.cpp
    Logger.cpp
    Main.cpp
    Platform.cpp
    ProcessOps.cpp
    RingBuffer.cpp
    SocketOps.cpp
    StreamReader.cpp
    StreamWriter.cpp
    StringOps.cpp
    Utils.cpp
    XML.cpp

    Http/Client.cpp

    SIMD/IntVec4.cpp
    SIMD/FloatVec4.cpp
)

add_executable(IACore_Test_Suite ${TEST_SOURCES})

if(MSVC)
    target_compile_options(IACore_Test_Suite PRIVATE "/EHsc")
else()
    target_compile_options(IACore_Test_Suite PRIVATE "-fexceptions")
endif()
set_target_properties(IACore_Test_Suite PROPERTIES USE_EXCEPTIONS ON)

target_link_libraries(IACore_Test_Suite PRIVATE IACore)

add_custom_command(TARGET IACore_Test_Suite POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:LongProcess>
    $<TARGET_FILE_DIR:IACore_Test_Suite>/LongProcess${CMAKE_EXECUTABLE_SUFFIX}
)