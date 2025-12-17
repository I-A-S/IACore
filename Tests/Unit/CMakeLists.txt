# ------------------------------------------------
#   C Compile Test (Keep separate to verify C ABI)
# ------------------------------------------------
add_executable(IACore_Test_C_ABI "CCompile.c")
set_target_properties(IACore_Test_C_ABI PROPERTIES
    C_STANDARD 99           
    C_STANDARD_REQUIRED ON
    LINKER_LANGUAGE C      
)
target_link_libraries(IACore_Test_C_ABI PRIVATE IACore)

# ------------------------------------------------
#   The Unified C++ Test Suite
# ------------------------------------------------
set(TEST_SOURCES
    Main.cpp
    Utils.cpp
    Environment.cpp
    ProcessOps.cpp
    StreamReader.cpp
    RingBuffer.cpp
)

add_executable(IACore_Test_Suite ${TEST_SOURCES})

# Enable exceptions for testing framework
target_compile_options(IACore_Test_Suite PRIVATE -fexceptions)
set_target_properties(IACore_Test_Suite PROPERTIES USE_EXCEPTIONS ON)

target_link_libraries(IACore_Test_Suite PRIVATE IACore)

# Copy necessary runtime assets
add_custom_command(TARGET IACore_Test_Suite POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:LongProcess>
    $<TARGET_FILE_DIR:IACore_Test_Suite>/LongProcess${CMAKE_EXECUTABLE_SUFFIX}
)