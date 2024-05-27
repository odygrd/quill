set(LIB_NAME foo_shared_test)

add_library(${LIB_NAME} SHARED
        include/foo/Foo.h
        include/foo/Foo.cpp)

# Add include directories for this library
target_include_directories(${LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Link quill dependency
target_link_libraries(${LIB_NAME} PUBLIC quill_shared_test)