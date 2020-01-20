# library name
set(TARGET_NAME fmt)

# header files
set(HEADER_FILES
        include/fmt/chrono.h
        include/fmt/color.h
        include/fmt/compile.h
        include/fmt/core.h
        include/fmt/format.h
        include/fmt/format-inl.h
        include/fmt/locale.h
        include/fmt/ostream.h
        include/fmt/posix.h
        include/fmt/printf.h
        include/fmt/ranges.h
        )

# source files
set(SOURCE_FILES
        src/format.cc
        src/posix.cc
        )

# Add this as a library
add_library(${TARGET_NAME} STATIC "")

# Add target sources
target_sources(${TARGET_NAME} PRIVATE ${SOURCE_FILES} ${HEADER_FILES})

# Add include directories for this library
target_include_directories(${TARGET_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Add compiler options for this library
target_compile_options(${TARGET_NAME} PRIVATE ${DEFAULT_COPTS})

# Do not decay cxx standard if not specified
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
