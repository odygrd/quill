set(LIB_NAME quill_wrapper_static_foo)

add_library(${LIB_NAME} STATIC
        include/quill_wrapper_static/quill_wrapper_static.h
        include/quill_wrapper_static/quill_wrapper_static.cpp)

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

# Link quill dependency
target_link_libraries(${LIB_NAME} PUBLIC quill::quill)