add_subdirectory(recommended_usage)
add_subdirectory(shared_library)

# Opt-in: the prometheus-cpp example is only built when QUILL_BUILD_EXAMPLE_PROMETHEUS=ON.
# We do not call find_package(prometheus-cpp) unconditionally because a broken or partial
# install on CMAKE_PREFIX_PATH can raise FATAL_ERROR from its targets file and abort our
# configure step entirely.
set(QUILL_PROMETHEUS_AVAILABLE FALSE)
if (QUILL_BUILD_EXAMPLE_PROMETHEUS)
    find_package(prometheus-cpp REQUIRED CONFIG)
    if (TARGET prometheus-cpp::core AND TARGET prometheus-cpp::pull)
        set(QUILL_PROMETHEUS_AVAILABLE TRUE)
    endif ()
endif ()

# Define a variable with all example targets
set(EXAMPLE_TARGETS
        quill_example_backend_thread_notify
        quill_example_backend_tsc_clock
        quill_example_backtrace_logging
        quill_example_binary_protocol_logging
        quill_example_bounded_dropping_queue_frontend
        quill_example_console_logging
        quill_example_console_logging_macro_free
        quill_example_custom_console_colours
        quill_example_simple_setup
        quill_example_rotating_file_logging
        quill_example_rotating_json_file_logging
        quill_example_rotating_json_file_logging_custom_json
        quill_example_signal_handler
        quill_example_sink_formatter_override
        quill_example_stopwatch
        quill_example_logger_removal_with_file_event_notifier
        quill_example_custom_frontend_options
        quill_example_file_logging
        quill_example_filter_logging
        quill_example_mdc_logging
        quill_example_system_clock_logging
        quill_example_user_clock_source
        quill_example_user_defined_filter
        quill_example_user_defined_sink
        quill_example_tags_logging
        quill_example_json_console_logging
        quill_example_json_console_logging_custom_json
        quill_example_manual_backend_worker
        quill_example_csv_writing
        quill_example_json_file_logging
        quill_example_user_defined_types_logging_custom_codec
        quill_example_user_defined_types_logging_deferred_format
        quill_example_user_defined_types_logging_deferred_format_as
        quill_example_user_defined_types_logging_direct_format
        quill_example_user_defined_types_logging_helper_macros
)

if (QUILL_PROMETHEUS_AVAILABLE)
    list(APPEND EXAMPLE_TARGETS quill_example_metric_publishing_prometheus)
endif ()

# Add additional examples that are using exceptions in the example code
if (NOT QUILL_NO_EXCEPTIONS)
    add_subdirectory(sbe_binary_data)

    list(APPEND EXAMPLE_TARGETS quill_example_user_defined_types_multi_format)
endif ()

# Add example executables
foreach (example_target ${EXAMPLE_TARGETS})
    # Determine the source file from the target name
    string(REPLACE "quill_example_" "" source_name ${example_target})
    set(source_file "${source_name}.cpp")

    add_executable(${example_target} ${source_file})
    set_common_compile_options(${example_target})

    if (example_target STREQUAL "quill_example_metric_publishing_prometheus")
        target_link_libraries(${example_target} quill prometheus-cpp::core prometheus-cpp::pull)
    else ()
        target_link_libraries(${example_target} quill)
    endif ()
endforeach ()

if (TARGET quill_module)
    add_executable(quill_example_console_logging_module console_logging_module.cpp)
    set_common_compile_options(quill_example_console_logging_module)
    target_compile_features(quill_example_console_logging_module PRIVATE cxx_std_20)
    set_target_properties(quill_example_console_logging_module PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(quill_example_console_logging_module PRIVATE quill_module)

    list(APPEND EXAMPLE_TARGETS quill_example_console_logging_module)
endif ()

install(TARGETS ${EXAMPLE_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
