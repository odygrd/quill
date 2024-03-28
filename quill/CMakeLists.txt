if (NOT QUILL_MASTER_PROJECT)
    find_package(Threads REQUIRED)
endif ()

# library name
set(TARGET_NAME quill)

# header files
set(HEADER_FILES
        # quill
        include/quill/core/filters/FilterBase.h

        include/quill/core/handlers/ConsoleHandler.h
        include/quill/core/handlers/FileHandler.h
        include/quill/core/handlers/Handler.h
        include/quill/core/handlers/JsonFileHandler.h
        include/quill/core/handlers/NullHandler.h
        include/quill/core/handlers/RotatingFileHandler.h
        include/quill/core/handlers/StreamHandler.h

        include/quill/backend/BackendWorker.h
        include/quill/backend/BacktraceStorage.h
        include/quill/backend/Clock.h
        include/quill/core/FileUtilities.h
        include/quill/core/SinkManager.h
        include/quill/core/LoggerManager.h
        include/quill/backend/BackendManager.h
        include/quill/core/PatternFormatter.h
        include/quill/backend/RdtscClock.h
        include/quill/backend/StringFromTime.h
        include/quill/core/TimestampFormatter.h
        include/quill/core/Utilities.h



        include/quill/detail/SignalHandler.h


        include/quill/core/Attributes.h
        include/quill/core/BoundedSPSCQueue.h
        include/quill/core/Config.h

        include/quill/core/LoggerDetails.h
        include/quill/core/Common.h
        include/quill/core/LogLevel.h

        include/quill/core/MacroMetadata.h
        include/quill/core/QuillError.h
        include/quill/core/Rdtsc.h

        include/quill/core/EncodeDecode.h
        include/quill/core/ThreadContextManager.h
        include/quill/core/UnboundedSPSCQueue.h
        include/quill/core/TransitEvent.h
        include/quill/core/TransitEventBuffer.h

        include/quill/core/Os.h
        include/quill/core/UserClock.h


        include/quill/frontend/Logger.h
        include/quill/frontend/LogMacros.h
        include/quill/Quill.h
        include/quill/TweakMe.h
        include/quill/Utility.h
        )

# source files
set(SOURCE_FILES
        src/detail/SignalHandler.cpp
        )

    list(APPEND HEADER_FILES
            include/quill/bundled/fmt/args.h
            include/quill/bundled/fmt/chrono.h
            include/quill/bundled/fmt/color.h
            include/quill/bundled/fmt/compile.h
            include/quill/bundled/fmt/core.h
            include/quill/bundled/fmt/format.h
            include/quill/bundled/fmt/format-inl.h
            include/quill/bundled/fmt/os.h
            include/quill/bundled/fmt/ostream.h
            include/quill/bundled/fmt/printf.h
            include/quill/bundled/fmt/ranges.h
            include/quill/bundled/fmt/std.h
            include/quill/bundled/fmt/xchar.h)

# Add as a library
add_library(${TARGET_NAME} "")
add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

if (BUILD_SHARED_LIBS)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    target_compile_definitions(${TARGET_NAME} PRIVATE QUILL_DLL_EXPORT INTERFACE QUILL_BUILD_SHARED)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_VISIBILITY_PRESET default
            VISIBILITY_INLINES_HIDDEN OFF
            POSITION_INDEPENDENT_CODE TRUE)
else ()
    # static library
    if (QUILL_DISABLE_POSITION_INDEPENDENT_CODE)
        set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE FALSE)
    else ()
        set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
    endif ()
endif ()

if (QUILL_NO_EXCEPTIONS)
    target_compile_definitions(${TARGET_NAME} PUBLIC -DQUILL_NO_EXCEPTIONS)

    if (NOT MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE -fno-exceptions)
    endif ()
endif ()

if (QUILL_NO_THREAD_NAME_SUPPORT)
    target_compile_definitions(${TARGET_NAME} PUBLIC -DQUILL_NO_THREAD_NAME_SUPPORT)
endif ()

# Add target sources
target_sources(${TARGET_NAME} PRIVATE ${SOURCE_FILES} ${HEADER_FILES})

    # Link dependencies
    target_link_libraries(${TARGET_NAME} PUBLIC Threads::Threads)

if (MINGW)
    # strftime requires this when using MinGw to correctly format the time ..
    target_link_libraries(${TARGET_NAME} PUBLIC ucrtbase)
endif ()

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(${TARGET_NAME} PUBLIC stdc++fs)
endif ()

# Add include directories for this library
target_include_directories(${TARGET_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Compiler options
target_compile_options(${TARGET_NAME} PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Wconversion -pedantic -Wfatal-errors -Wno-unused-private-field -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-parameter>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /wd4324 /wd4189 /wd4996 /wd4100 /wd4127 /wd4702>)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if (NOT QUILL_NO_EXCEPTIONS)
        target_compile_options(${TARGET_NAME} PUBLIC /EHsc)
    endif ()

    target_compile_options(${TARGET_NAME} PRIVATE /WX)
    target_compile_definitions(${TARGET_NAME} PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
endif ()

# Properties
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${QUILL_VERSION} SOVERSION ${QUILL_VERSION})

# ---- Tests ---- #
if (QUILL_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test)
    add_subdirectory(test)
endif ()

if (QUILL_MASTER_PROJECT OR QUILL_ENABLE_INSTALL)
    # ---- Install ---- #
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

    set(QUILL_PKGCONFIG_DIR ${CMAKE_INSTALL_PREFIX}/pkgconfig CACHE PATH
            "Installation directory for pkgconfig (.pc) files, relative to ${CMAKE_INSTALL_PREFIX}.")

    # Generate the version, config and target files into the build directory.
    # Generate version_config
    write_basic_package_version_file(
            ${version_config}
            VERSION ${QUILL_VERSION}
            COMPATIBILITY AnyNewerVersion)

    # Generate pkgconfig
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/cmake/quill.pc.in"
            "${pkgconfig}"
            @ONLY)

    configure_package_config_file(
            ${CMAKE_CURRENT_LIST_DIR}/cmake/quill-config.cmake.in
            ${project_config}
            INSTALL_DESTINATION ${QUILL_CMAKE_DIR})

    # Install version, config files
    install(FILES ${project_config} ${version_config}
            DESTINATION ${QUILL_CMAKE_DIR})

    # Install the library and headers.
    install(TARGETS ${TARGET_NAME} EXPORT ${targets_export_name}
            LIBRARY DESTINATION ${QUILL_LIB_DIR}
            ARCHIVE DESTINATION ${QUILL_LIB_DIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Export the library
    install(EXPORT ${targets_export_name} DESTINATION ${QUILL_CMAKE_DIR}
            NAMESPACE quill::)

    # Install the examples
    if (QUILL_BUILD_EXAMPLES)
        install(TARGETS quill_example_backtrace quill_example_configure_backend quill_example_console_colours
                quill_example_console_colours_with_custom_formatter quill_example_custom_clock quill_example_custom_clock_advanced
                quill_example_custom_console_colours quill_example_trivial quill_example_logging_to_file quill_example_multiple_handlers
                quill_example_quick_start quill_example_signal_handler quill_example_stdout_multiple_formatters quill_example_custom_formatter
                quill_example_custom_formatter_thread_name quill_example_custom_handler quill_example_daily_file_rotation
                quill_example_file_rotation quill_example_filters quill_example_handler_log_levels quill_example_json_structured_log
                quill_example_trivial_system_clock quill_example_trivial_file_logging quill_example_user_defined_types
                quill_example_bounded_queue_message_dropping quill_example_bounded_queue_blocking quill_example_file_callbacks
                quill_example_wake_up_logging_thread
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif ()

    install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/quill DESTINATION ${QUILL_INC_DIR})

    # Copy pkgconfig
    install(FILES "${pkgconfig}" DESTINATION "${QUILL_PKGCONFIG_DIR}")

    # ---- Packaging ---- #
    set(CPACK_GENERATOR
            ZIP
            )

    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
    set(CPACK_INSTALL_CMAKE_PROJECTS
            "${CMAKE_BINARY_DIR}"
            "${PROJECT_NAME}"
            ALL
            .
            )

    set(CPACK_PROJECT_URL "https://github.com/odygrd/quill")
    set(CPACK_PACKAGE_VENDOR "Odysseas Georgoudis")
    set(CPACK_PACKAGE_CONTACT "Odysseas Odysseas <odygrd@hotmail.com>")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Asynchronous Low Latency C++ Logging Library")
    set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
    set(CPACK_PACKAGE_RELOCATABLE ON)

    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "System Environment/Libraries")
    set(CPACK_RPM_PACKAGE_URL ${CPACK_PROJECT_URL})
    set(CPACK_RPM_PACKAGE_DESCRIPTION "Asynchronous Low Latency C++ Logging Library")
    include(CPack)
endif ()
