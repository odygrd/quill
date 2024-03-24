# Get Quill version from include/quill/Version.h and store it as QUILL_VERSION
function(quill_extract_version)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/quill/include/quill/Backend.h" file_contents)

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