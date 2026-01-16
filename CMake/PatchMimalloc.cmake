# Get the file path from the command line argument
set(TARGET_FILE "${SOURCE_DIR}/src/prim/windows/prim.c")

# Read the file
file(READ "${TARGET_FILE}" FILE_CONTENT)

# Check if the patch is already applied
string(FIND "${FILE_CONTENT}" "struct _TEB* const teb" ALREADY_PATCHED)

if(ALREADY_PATCHED EQUAL -1)
    message(STATUS "Patching mimalloc source: ${TARGET_FILE}")
    
    # Perform the replacement
    string(REPLACE 
        "_TEB* const teb = NtCurrentTeb()" 
        "struct _TEB* const teb = NtCurrentTeb()" 
        FILE_CONTENT 
        "${FILE_CONTENT}"
    )

    # Write the file back only if changes were made
    file(WRITE "${TARGET_FILE}" "${FILE_CONTENT}")
else()
    message(STATUS "mimalloc source is already patched. Skipping.")
endif()

# Patch mimalloc complaing about false positive alignment issues in libc loader
file(READ "${SOURCE_DIR}/CMakeLists.txt" MI_CMAKE_CONTENT)
string(REPLACE 
    "set(mi_debug_default ON)" 
    "set(mi_debug_default OFF)" 
    MI_CMAKE_CONTENT 
    "${MI_CMAKE_CONTENT}"
)
file(WRITE "${SOURCE_DIR}/CMakeLists.txt" "${MI_CMAKE_CONTENT}")
message(STATUS "Patched mimalloc: Forced MI_DEBUG default to OFF to silence alignment warnings.")