set(LIB_NAME quill_shared_lib)

add_library(${LIB_NAME} SHARED
        quill_shared.h
        quill_shared.cpp)

# Add include directories for this library
target_include_directories(${LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

set_common_compile_options(${LIB_NAME})

# Optional - Set the compiler flags for GCC/Clang
if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER MATCHES "clang-cl"))
    target_compile_options(${LIB_NAME} PRIVATE -fvisibility=hidden)
endif ()

# Required - Define QUILL_DLL_EXPORT when building the library on Windows
if (WIN32)
    target_compile_definitions(${LIB_NAME} PRIVATE QUILL_DLL_EXPORT)
endif ()

# Link quill dependency
target_link_libraries(${LIB_NAME} PUBLIC quill::quill)
