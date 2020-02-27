cmake_minimum_required(VERSION 3.1.0)

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
set(QUILL_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(QUILL_MASTER_PROJECT ON)
endif ()

#-------------------------------------------------------------------------------------------------------
# Custom cmake functions
#-------------------------------------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
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

#---------------------------------------------------------------------------------------
# Always verbose make file
#---------------------------------------------------------------------------------------
if (QUILL_MASTER_PROJECT)
    set(CMAKE_VERBOSE_MAKEFILE TRUE CACHE BOOL "Verbose output" FORCE)
endif ()

#-------------------------------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------------------------------

option(QUILL_BUILD_SHARED "Build as shared library" OFF)

option(QUILL_BUILD_EXAMPLES "Build the examples" OFF)

option(QUILL_BUILD_TESTS "Build the tests (Requires https://github.com/google/googletest to be installed)" OFF)

option(QUILL_SANITIZE_ADDRESS "Enable address sanitizer in tests" OFF)

option(QUILL_SANITIZE_THREAD "Enable thread sanitizer in tests (Using this option with any other compiler except clang may result to false positives)" OFF)

option(QUILL_CODE_COVERAGE "Enable code coverage" OFF)

option(QUILL_USE_VALGRIND "Use valgrind as the default memcheck tool in CTest (Requires Valgrind)" OFF)

#-------------------------------------------------------------------------------------------------------
# Required Packages
#-------------------------------------------------------------------------------------------------------
find_package(Threads REQUIRED)

if (QUILL_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})

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
    message(STATUS "Build Type: " ${CMAKE_BUILD_TYPE})
    message(STATUS "Quill Version: ${QUILL_VERSION}")
endif ()

#-------------------------------------------------------------------------------------------------------
# Additional Compiler Options
#-------------------------------------------------------------------------------------------------------
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
endif ()

#-------------------------------------------------------------------------------------------------------
# Subdirectories
#-------------------------------------------------------------------------------------------------------
add_subdirectory(quill)

#-------------------------------------------------------------------------------------------------------
# Install
#-------------------------------------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(version_config ${PROJECT_BINARY_DIR}/quill-config-version.cmake)
set(project_config ${PROJECT_BINARY_DIR}/quill-config.cmake)
set(pkgconfig ${PROJECT_BINARY_DIR}/quill.pc)
set(targets_export_name quill-targets)

set(QUILL_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/quill CACHE STRING
        "Installation directory for cmake files, relative to ${CMAKE_INSTALL_PREFIX}.")

set(QUILL_LIB_DIR ${CMAKE_INSTALL_LIBDIR} CACHE STRING
        "Installation directory for libraries, relative to ${CMAKE_INSTALL_PREFIX}.")

set(QUILL_INC_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE STRING
        "Installation directory for include files, relative to ${CMAKE_INSTALL_PREFIX}.")

set(QUILL_PKGCONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/pkgconfig CACHE PATH
        "Installation directory for pkgconfig (.pc) files, relative to ${CMAKE_INSTALL_PREFIX}.")

# Generate the version, config and target files into the build directory.
# Generate version_config
write_basic_package_version_file(
        ${version_config}
        VERSION ${QUILL_VERSION}
        COMPATIBILITY AnyNewerVersion)

# FIX PKGCONFIG
## Generate pkgconfig
#configure_file(
#        "${PROJECT_SOURCE_DIR}/quill/cmake/quill.pc.in"
#        "${pkgconfig}"
#        @ONLY)

configure_package_config_file(
        ${PROJECT_SOURCE_DIR}/cmake/quill-config.cmake.in
        ${project_config}
        INSTALL_DESTINATION ${QUILL_CMAKE_DIR})

# Install version, config files
install(FILES ${project_config} ${version_config}
        DESTINATION ${QUILL_CMAKE_DIR})

# Install the library and headers.
install(TARGETS quill EXPORT ${targets_export_name}
        LIBRARY DESTINATION ${QUILL_LIB_DIR}
        ARCHIVE DESTINATION ${QUILL_LIB_DIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Export the library
install(EXPORT ${targets_export_name} DESTINATION ${QUILL_CMAKE_DIR}
        NAMESPACE quill::)

# Copy the headers
install(DIRECTORY quill/include/quill DESTINATION ${QUILL_INC_DIR})

# TODO: FIX
#install(FILES "${pkgconfig}" DESTINATION "${QUILL_PKGCONFIG_DIR}")
