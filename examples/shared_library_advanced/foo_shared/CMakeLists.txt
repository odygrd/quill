set(LIB_NAME foo_shared)

add_library(${LIB_NAME} SHARED
        include/foo_shared/foo_shared.h
        include/foo_shared/foo_shared.cpp)

# Add include directories for this library
target_include_directories(${LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Required - Define QUILL_DLL_EXPORT when building the library on Windows
if (WIN32)
    target_compile_definitions(${LIB_NAME} PRIVATE FOO_DLL_EXPORT)
endif ()

# Link quill dependency
target_link_libraries(${LIB_NAME} PUBLIC quill_wrapper_static_foo)