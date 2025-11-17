# Define a variable with all example targets
set(EXAMPLE_TARGETS
        quill_docs_binary_file_writer
        quill_docs_example_backend_options
        quill_docs_example_backtrace_logging_1
        quill_docs_example_backtrace_logging_2
        quill_docs_example_basic
        quill_docs_example_console
        quill_docs_example_csv_writer_1
        quill_docs_example_custom_format
        quill_docs_example_file
        quill_docs_example_filter_1
        quill_docs_example_json_logging
        quill_docs_example_loggers_remove
        quill_docs_example_multiple_sinks
        quill_docs_example_multiple_sinks_tags
        quill_docs_example_tags_with_custom_sink
        quill_docs_quick_start
        quill_docs_timestamp_types_1
)

# Add example executables
foreach (example_target ${EXAMPLE_TARGETS})
    # Determine the source file from the target name
    set(source_file "${example_target}.cpp")

    add_executable(${example_target} ${source_file})
    set_common_compile_options(${example_target})
    target_link_libraries(${example_target} quill)
endforeach ()

install(TARGETS ${EXAMPLE_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})