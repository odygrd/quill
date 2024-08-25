add_subdirectory(recommended_usage)
add_subdirectory(shared_library)
add_subdirectory(advanced)
add_subdirectory(single_logger_multiple_sink_formats)

# Define a variable with all example targets
set(EXAMPLE_TARGETS
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
        quill_example_json_console_logging
        quill_example_csv_writing
        quill_example_json_file_logging
        quill_example_user_defined_types_logging
        quill_example_user_defined_types_multi_format
)

# Add example executables
foreach (example_target ${EXAMPLE_TARGETS})
    # Determine the source file from the target name
    string(REPLACE "quill_example_" "" source_name ${example_target})
    set(source_file "${source_name}.cpp")

    add_executable(${example_target} ${source_file})
    set_common_compile_options(${example_target})
    target_link_libraries(${example_target} quill)
endforeach ()

install(TARGETS ${EXAMPLE_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})