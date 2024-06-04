set(LIB_NAME quill_wrapper_shared)

add_library(${LIB_NAME} SHARED
        include/quill_wrapper_shared/quill_wrapper_shared.h
        include/quill_wrapper_shared/quill_wrapper_shared.cpp)

# Add include directories for this library
target_include_directories(${LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Optional - Set the compiler flags for GCC/Clang
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${LIB_NAME} PRIVATE -fvisibility=hidden)
endif ()

# Required - Define QUILL_DLL_EXPORT when building the library on Windows
if (WIN32)
    target_compile_definitions(${LIB_NAME} PRIVATE QUILL_DLL_EXPORT)
endif ()

# Link quill dependency
target_link_libraries(${LIB_NAME} PUBLIC quill::quill)