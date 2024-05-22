set(LIB_NAME qwrapper_compile_time_bench)

add_library(${LIB_NAME} STATIC
        include/qwrapper/qwrapper.h
        include/qwrapper/qwrapper.cpp)

# Add include directories for this library
target_include_directories(${LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Link quill dependency
target_link_libraries(${LIB_NAME} PUBLIC quill::quill)