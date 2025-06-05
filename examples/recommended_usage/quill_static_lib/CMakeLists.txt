set(LIB_NAME quill_static_lib_recommended_usage)

add_library(${LIB_NAME} STATIC
        overwrite_macros.h
        quill_static.h
        quill_static.cpp)

# Add include directories for this library
target_include_directories(${LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

set_common_compile_options(${LIB_NAME})

# Link quill dependency
target_link_libraries(${LIB_NAME} PUBLIC quill::quill)
