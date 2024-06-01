if (NOT QUILL_MASTER_PROJECT)
    find_package(Threads REQUIRED)
endif ()

# library name
set(TARGET_NAME quill)

# header files
set(HEADER_FILES
        include/quill/backend/BackendManager.h
        include/quill/backend/BackendOptions.h
        include/quill/backend/BackendWorker.h
        include/quill/backend/BacktraceStorage.h
        include/quill/backend/PatternFormatter.h
        include/quill/backend/RdtscClock.h
        include/quill/backend/SignalHandler.h
        include/quill/backend/StringFromTime.h
        include/quill/backend/TimestampFormatter.h
        include/quill/backend/TransitEvent.h
        include/quill/backend/TransitEventBuffer.h
        include/quill/backend/BackendUtilities.h

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
        include/quill/bundled/fmt/xchar.h

        include/quill/core/Attributes.h
        include/quill/core/BoundedSPSCQueue.h
        include/quill/core/Common.h
        include/quill/core/DynamicFormatArgStore.h
        include/quill/core/Codec.h
        include/quill/core/Filesystem.h
        include/quill/core/FormatBuffer.h
        include/quill/core/FrontendOptions.h
        include/quill/core/LoggerBase.h
        include/quill/core/LoggerManager.h
        include/quill/core/LogLevel.h
        include/quill/core/MacroMetadata.h
        include/quill/core/MathUtils.h
        include/quill/core/QuillError.h
        include/quill/core/Rdtsc.h
        include/quill/core/SinkManager.h
        include/quill/core/ThreadContextManager.h
        include/quill/core/ThreadUtilities.h
        include/quill/core/TimeUtilities.h
        include/quill/core/UnboundedSPSCQueue.h

        include/quill/filters/Filter.h

        include/quill/sinks/ConsoleSink.h
        include/quill/sinks/FileSink.h
        include/quill/sinks/JsonConsoleSink.h
        include/quill/sinks/JsonFileSink.h
        include/quill/sinks/NullSink.h
        include/quill/sinks/RotatingFileSink.h
        include/quill/sinks/Sink.h
        include/quill/sinks/StreamSink.h

        include/quill/std/Array.h
        include/quill/std/Deque.h
        include/quill/std/FilesystemPath.h
        include/quill/std/ForwardList.h
        include/quill/std/List.h
        include/quill/std/Map.h
        include/quill/std/Optional.h
        include/quill/std/Pair.h
        include/quill/std/Set.h
        include/quill/std/Tuple.h
        include/quill/std/UnorderedMap.h
        include/quill/std/UnorderedSet.h
        include/quill/std/Vector.h

        include/quill/Backend.h
        include/quill/BackendTscClock.h
        include/quill/Frontend.h
        include/quill/Logger.h
        include/quill/LogMacros.h
        include/quill/UserClockSource.h
        include/quill/Utility.h
)

# Add as a library
add_library(${TARGET_NAME} INTERFACE)
add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

if (QUILL_NO_EXCEPTIONS)
    target_compile_definitions(${TARGET_NAME} PUBLIC INTERFACE -DQUILL_NO_EXCEPTIONS)

    if (NOT MSVC)
        target_compile_options(${TARGET_NAME} PUBLIC INTERFACE -fno-exceptions)
    endif ()
endif ()

if (QUILL_NO_THREAD_NAME_SUPPORT)
    target_compile_definitions(${TARGET_NAME} PUBLIC INTERFACE -DQUILL_NO_THREAD_NAME_SUPPORT)
endif ()

if (QUILL_X86ARCH)
    target_compile_definitions(${TARGET_NAME} PUBLIC INTERFACE -DQUILL_X86ARCH)
endif ()

if (QUILL_DISABLE_NON_PREFIXED_MACROS)
    target_compile_definitions(${TARGET_NAME} PUBLIC INTERFACE -DQUILL_DISABLE_NON_PREFIXED_MACROS)
endif ()

# Add target sources
target_sources(${TARGET_NAME} PRIVATE ${HEADER_FILES})

# Link dependencies
target_link_libraries(${TARGET_NAME} PUBLIC INTERFACE Threads::Threads)

if (MINGW)
    # strftime requires this when using MinGw to correctly format the time ..
    target_link_libraries(${TARGET_NAME} PUBLIC INTERFACE ucrtbase)
endif ()

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(${TARGET_NAME} PUBLIC INTERFACE stdc++fs)
endif ()

# Add include directories for this library
target_include_directories(${TARGET_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

# Compiler options
target_compile_options(${TARGET_NAME} INTERFACE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-Wno-gnu-zero-variadic-macro-arguments>)

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

    # Generate pkgconfig
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/cmake/quill.pc.in"
            "${pkgconfig}"
            @ONLY)

    # Copy pkgconfig
    install(FILES "${pkgconfig}" DESTINATION "${QUILL_PKGCONFIG_DIR}")

    # Generate the version, config and target files into the build directory.
    write_basic_package_version_file(
            ${version_config}
            VERSION ${QUILL_VERSION}
            COMPATIBILITY AnyNewerVersion)

    configure_package_config_file(
            ${CMAKE_CURRENT_LIST_DIR}/cmake/quill-config.cmake.in
            ${project_config}
            INSTALL_DESTINATION ${QUILL_CMAKE_DIR})

    # Install version, config files
    install(FILES ${project_config} ${version_config}
            DESTINATION ${QUILL_CMAKE_DIR})

    # Install the headers
    install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/quill DESTINATION ${QUILL_INC_DIR})

    # Install the library
    install(TARGETS ${TARGET_NAME} EXPORT ${targets_export_name}
            LIBRARY DESTINATION ${QUILL_LIB_DIR}
            ARCHIVE DESTINATION ${QUILL_LIB_DIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Export the library
    install(EXPORT ${targets_export_name} DESTINATION ${QUILL_CMAKE_DIR}
            NAMESPACE quill::)

    # Install the examples
    if (QUILL_BUILD_EXAMPLES)
        install(TARGETS
                quill_example_use_overwrite_macros
                quill_example_recommended_usage
                quill_example_backend_thread_notify
                quill_example_backtrace_logging
                quill_example_bounded_dropping_queue_frontend
                quill_example_console_logging
                quill_example_custom_console_colours
                quill_example_rotating_file_logging
                quill_example_signal_handler
                quill_example_logger_removal_with_file_event_notifier
                quill_example_custom_frontend_options
                quill_example_file_logging
                quill_example_filter_logging
                quill_example_system_clock_logging
                quill_example_user_clock_source
                quill_example_user_defined_filter
                quill_example_user_defined_sink
                quill_example_tags_logging
                quill_example_json_sink_logging
                quill_example_user_defined_types_logging
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif ()

    # ---- Packaging ---- #
    set(CPACK_GENERATOR ZIP)
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
    set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR}" "${PROJECT_NAME}" ALL .)
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
