cmake_minimum_required(VERSION 3.8)
project(quill)

#-------------------------------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------------------------------

# Builds Quill without exceptions by adding the -fno-exceptions flag to the compiler.
option(QUILL_NO_EXCEPTIONS "Build without exceptions using -fno-exceptions flag" OFF)

# Disables features that rely on retrieving the thread name, which is not supported in older versions of Windows (e.g., Windows Server 2012/2016).
# Enabling this option ensures compatibility with older Windows versions.
option(QUILL_NO_THREAD_NAME_SUPPORT "Disable features not supported on Windows Server 2012/2016" OFF)

# Enables the use of _mm_prefetch, _mm_clflush, and _mm_clflushopt instructions to enhance cache coherence performance on x86 architectures.
# When enabled, Quill will utilize these instructions on the frontend's queue operations.
# Ensure to specify the target architecture with -march="..." when compiling to maximize compatibility and performance.
option(QUILL_X86ARCH "Enable optimizations for cache coherence on x86 architectures using specific CPU instructions" OFF)

# When enabled, removes the non-prefixed `LOG_*` macros, leaving only `QUILL_LOG_*` macros available.
# This is useful in scenarios where the original macro names conflict with those of an existing logging library.
option(QUILL_DISABLE_NON_PREFIXED_MACROS "Disable non-prefixed macros" OFF)

option(QUILL_BUILD_EXAMPLES "Build the examples" OFF)

option(QUILL_BUILD_TESTS "Build the tests (Requires https://github.com/google/googletest to be installed)" OFF)

option(QUILL_BUILD_BENCHMARKS "Build the benchmarks" OFF)

option(QUILL_SANITIZE_ADDRESS "Enable address sanitizer in tests" OFF)

option(QUILL_SANITIZE_THREAD "Enable thread sanitizer in tests (Using this option with any other compiler except clang may result to false positives)" OFF)

option(QUILL_CODE_COVERAGE "Enable code coverage" OFF)

option(QUILL_USE_VALGRIND "Use valgrind as the default memcheck tool in CTest (Requires Valgrind)" OFF)

option(QUILL_ENABLE_INSTALL "Enable CMake Install when Quill is not a master project" OFF)

option(QUILL_DOCS_GEN "Generate documentation" OFF)

#-------------------------------------------------------------------------------------------------------
# Use newer policies if possible, up to most recent tested version of CMake.
#-------------------------------------------------------------------------------------------------------
cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})

#-------------------------------------------------------------------------------------------------------
# Include common compiler options
#-------------------------------------------------------------------------------------------------------
include(${PROJECT_SOURCE_DIR}/cmake/SetCommonCompileOptions.cmake)

#-------------------------------------------------------------------------------------------------------
# Determine if quill is built as a subproject (using add_subdirectory) or if it is the master project.
#-------------------------------------------------------------------------------------------------------
set(QUILL_MASTER_PROJECT FALSE CACHE BOOL "Master Project" FORCE)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(QUILL_MASTER_PROJECT TRUE CACHE BOOL "Master Project" FORCE)
endif ()

#-------------------------------------------------------------------------------------------------------
# Custom cmake functions
#-------------------------------------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/quill/cmake")
include(Utility)

#-------------------------------------------------------------------------------------------------------
# Resolve version
#-------------------------------------------------------------------------------------------------------
quill_extract_version()

project(quill VERSION ${QUILL_VERSION} LANGUAGES CXX)

#-------------------------------------------------------------------------------------------------------
# Set default build to release
#-------------------------------------------------------------------------------------------------------
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif ()

#---------------------------------------------------------------------------------------
# Compiler config
#---------------------------------------------------------------------------------------
if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

#-------------------------------------------------------------------------------------------------------
# Required Packages
#-------------------------------------------------------------------------------------------------------
find_package(Threads REQUIRED)

if (QUILL_BUILD_TESTS)
    enable_testing()

    if (QUILL_USE_VALGRIND)
        # find valgrind
        find_program(MEMORYCHECK_COMMAND NAMES valgrind)
        if (NOT MEMORYCHECK_COMMAND)
            message(WARNING "Valgrind not found")
        endif ()

        # set valgrind params
        set(MEMORYCHECK_COMMAND_OPTIONS "--tool=memcheck --leak-check=full --leak-resolution=med --show-leak-kinds=all --track-origins=yes --vgdb=no --fair-sched=yes")

        # add memcheck test action to ctest
        include(CTest)
    endif ()
endif ()

#-------------------------------------------------------------------------------------------------------
# Log Info
#-------------------------------------------------------------------------------------------------------
if (QUILL_MASTER_PROJECT)
    option(QUILL_VERBOSE_MAKEFILE "Verbose make output" OFF)

    message(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})
    message(STATUS "QUILL_VERSION: ${QUILL_VERSION}")
endif ()

message(STATUS "QUILL_NO_EXCEPTIONS: " ${QUILL_NO_EXCEPTIONS})
message(STATUS "QUILL_NO_THREAD_NAME_SUPPORT: " ${QUILL_NO_THREAD_NAME_SUPPORT})
message(STATUS "QUILL_X86ARCH: " ${QUILL_X86ARCH})
message(STATUS "QUILL_DISABLE_NON_PREFIXED_MACROS: " ${QUILL_DISABLE_NON_PREFIXED_MACROS})

#---------------------------------------------------------------------------------------
# Verbose make file option
#---------------------------------------------------------------------------------------
if (QUILL_VERBOSE_MAKEFILE)
    set(CMAKE_VERBOSE_MAKEFILE TRUE CACHE BOOL "Verbose output" FORCE)
endif ()

# address sanitizer flags
if (QUILL_SANITIZE_ADDRESS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer -g")
endif ()

# thread sanitizer flags
if (QUILL_SANITIZE_THREAD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
endif ()

# Append extra options for coverage
if (QUILL_CODE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif ()

# Build Examples
if (QUILL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

if (QUILL_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif ()

add_subdirectory(quill)

if (QUILL_DOCS_GEN)
    # Add the cmake folder so the FindSphinx module is found
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
    add_subdirectory(docs)
endif ()