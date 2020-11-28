cmake_minimum_required(VERSION 3.1.0)
project(my_project)

#-------------------------------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------------------------------

option(QUILL_FMT_EXTERNAL "Use external fmt library instead of bundled" OFF)

option(QUILL_NO_EXCEPTIONS "Build without exceptions with -fno-exceptions flag" OFF)

option(QUILL_USE_BOUNDED_QUEUE "Build with bounded queue instead of unbounded" OFF)

option(QUILL_BUILD_EXAMPLES "Build the examples" OFF)

option(QUILL_BUILD_TESTS "Build the tests (Requires https://github.com/google/googletest to be installed)" OFF)

option(QUILL_BUILD_BENCHMARKS "Build the benchmarks" OFF)

option(QUILL_SANITIZE_ADDRESS "Enable address sanitizer in tests" OFF)

option(QUILL_SANITIZE_THREAD "Enable thread sanitizer in tests (Using this option with any other compiler except clang may result to false positives)" OFF)

option(QUILL_CODE_COVERAGE "Enable code coverage" OFF)

option(QUILL_USE_VALGRIND "Use valgrind as the default memcheck tool in CTest (Requires Valgrind)" OFF)

option(QUILL_ENABLE_INSTALL "Enable CMake Install when Quill is not a master project" OFF)

#-------------------------------------------------------------------------------------------------------
# Use newer policies if possible, up to most recent tested version of CMake.
#-------------------------------------------------------------------------------------------------------
if (${CMAKE_VERSION} VERSION_LESS 3.16)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else ()
    cmake_policy(VERSION 3.16)
endif ()

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
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/quill/cmake" CACHE STRING "Modules for CMake" FORCE)
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
    set(CMAKE_CXX_STANDARD 14)
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

    message(STATUS "Build Type: " ${CMAKE_BUILD_TYPE})
    message(STATUS "Quill Version: ${QUILL_VERSION}")
endif ()

#---------------------------------------------------------------------------------------
# Verbose make file option
#---------------------------------------------------------------------------------------
if (QUILL_VERBOSE_MAKEFILE)
    set(CMAKE_VERBOSE_MAKEFILE TRUE CACHE BOOL "Verbose output" FORCE)
endif ()

#-------------------------------------------------------------------------------------------------------
# Additional Compiler Options
#-------------------------------------------------------------------------------------------------------
if (QUILL_NO_EXCEPTIONS)
    add_definitions(-DQUILL_NO_EXCEPTIONS)
endif ()

if (QUILL_USE_BOUNDED_QUEUE)
    add_definitions(-DQUILL_USE_BOUNDED_QUEUE)
endif ()

if (QUILL_FMT_EXTERNAL)
    add_definitions(-DQUILL_FMT_EXTERNAL)
endif ()

message(STATUS "QUILL_NO_EXCEPTIONS: " ${QUILL_NO_EXCEPTIONS})
message(STATUS "QUILL_USE_BOUNDED_QUEUE: " ${QUILL_USE_BOUNDED_QUEUE})
message(STATUS "QUILL_FMT_EXTERNAL: " ${QUILL_FMT_EXTERNAL})

# address sanitizer flags
if (QUILL_SANITIZE_ADDRESS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
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
