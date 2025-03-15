set(LIB_NAME quill_static_lib_recommended_usage)

add_library(${LIB_NAME} STATIC
        include/quill_static_lib/overwrite_macros.h
        include/quill_static_lib/quill_static.h
        include/quill_static_lib/quill_static.cpp)

# Add include directories for this library
target_include_directories(${LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Link quill dependency
target_link_libraries(${LIB_NAME} PUBLIC quill::quill)