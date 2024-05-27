set(LIB_NAME quill_shared_test)

add_library(${LIB_NAME} SHARED
        include/quill_wrapper/QuillWrapper.h
        include/quill_wrapper/QuillWrapper.cpp)

# Add include directories for this library
target_include_directories(${LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Link quill dependency
target_link_libraries(${LIB_NAME} PUBLIC quill::quill)