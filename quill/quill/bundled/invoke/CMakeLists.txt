# library name
set(TARGET_NAME invoke)

# Add this as a library
add_library(${TARGET_NAME} INTERFACE)

# Add include directories for this library
target_include_directories(${TARGET_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

