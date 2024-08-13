# Get Quill version from include/quill/Version.h and store it as QUILL_VERSION
function(quill_extract_version)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/include/quill/Backend.h" file_contents)

    string(REGEX MATCH "constexpr uint32_t VersionMajor{([0-9]+)}" _ "${file_contents}")
    if (NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Failed to extract major version number from quill/Backend.h")
    endif ()
    set(version_major ${CMAKE_MATCH_1})

    string(REGEX MATCH "constexpr uint32_t VersionMinor{([0-9]+)}" _ "${file_contents}")
    if (NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Failed to extract minor version number from quill/Backend.h")
    endif ()
    set(version_minor ${CMAKE_MATCH_1})

    string(REGEX MATCH "constexpr uint32_t VersionPatch{([0-9]+)}" _ "${file_contents}")
    if (NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Failed to extract patch version number from quill/Backend.h")
    endif ()
    set(version_patch ${CMAKE_MATCH_1})

    set(QUILL_VERSION "${version_major}.${version_minor}.${version_patch}" PARENT_SCOPE)
endfunction()

# Define the function to set common compile options
function(set_common_compile_options target_name)
    cmake_parse_arguments(COMPILE_OPTIONS "" "VISIBILITY" "" ${ARGN})

    # Set default visibility to PRIVATE if not provided
    if (NOT DEFINED COMPILE_OPTIONS_VISIBILITY)
        set(COMPILE_OPTIONS_VISIBILITY PRIVATE)
    endif ()

    target_compile_options(${target_name} ${COMPILE_OPTIONS_VISIBILITY}
            $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
            -Wall -Wextra -pedantic -Werror>
            $<$<CXX_COMPILER_ID:MSVC>:/bigobj /WX /W4 /wd4324 /wd4996>)

    # Additional MSVC specific options
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        if (NOT QUILL_NO_EXCEPTIONS)
            target_compile_options(${target_name} ${COMPILE_OPTIONS_VISIBILITY} /EHsc)
        endif ()
    endif ()
endfunction()
