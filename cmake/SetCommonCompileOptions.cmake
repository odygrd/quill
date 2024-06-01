# Define the function to set common compile options
function(set_common_compile_options target_name)
    cmake_parse_arguments(COMPILE_OPTIONS "" "VISIBILITY" "" ${ARGN})

    # Set default visibility to PRIVATE if not provided
    if (NOT DEFINED COMPILE_OPTIONS_VISIBILITY)
        set(COMPILE_OPTIONS_VISIBILITY PRIVATE)
    endif ()

    target_compile_options(${target_name} ${COMPILE_OPTIONS_VISIBILITY}
            $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
            -Wall -Wextra -Wconversion -pedantic -Wfatal-errors -Wno-unused-private-field -Wno-unused-parameter>
            $<$<CXX_COMPILER_ID:MSVC>:/bigobj /WX /W4 /wd4324 /wd4189 /wd4996 /wd4100 /wd4127 /wd4702>)

    # Additional MSVC specific options
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        if (NOT QUILL_NO_EXCEPTIONS)
            target_compile_options(${target_name} ${COMPILE_OPTIONS_VISIBILITY} /EHsc)
        endif ()
    endif ()
endfunction()