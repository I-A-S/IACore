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