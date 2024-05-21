set(LIB_NAME quill_wrapper_recommended)

add_library(${LIB_NAME} STATIC
        include/quill_wrapper/overwrite_macros.h
        include/quill_wrapper/quill_wrapper.h
        include/quill_wrapper/quill_wrapper.cpp)

# Add include directories for this library
target_include_directories(${LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Link quill dependency
target_link_libraries(${LIB_NAME} PUBLIC quill::quill)