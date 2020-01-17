# library name
set(TARGET_NAME quill)

# header files
set(HEADER_FILES
        include/quill/detail/BoundedSPSCQueue.h
        include/quill/detail/LoggerCollection.h
        include/quill/detail/LoggingWorker.h
        include/quill/detail/LogLineInfo.h
        include/quill/detail/LogManager.h
        include/quill/detail/Macros.h
        include/quill/detail/Message.h
        include/quill/detail/MessageHelpers.h
        include/quill/detail/ThreadContext.h
        include/quill/detail/ThreadContextCollection.h
        include/quill/detail/Utilities.h
        include/quill/Logger.h
        include/quill/LogLevel.h
        include/quill/LogMacros.h
        )

# source files
set(SOURCE_FILES
        src/detail/LoggerCollection.cpp
        src/detail/LoggingWorker.cpp
        src/detail/LogManager.cpp
        src/detail/ThreadContextCollection.cpp
        src/detail/Utilities.cpp
        src/LogLevel.cpp
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

# Link dependencies
# target_link_libraries(${TARGET_NAME} PUBLIC fmt)

# Do not decay cxx standard if not specified
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

# ---- Tests ---- #
if (ENABLE_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test)
    add_subdirectory(test)
endif ()

# ---- Benchmarks ---- #
if (ENABLE_BENCHMARKS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/benchmark)
    add_subdirectory(benchmark)
endif ()