# Define a variable with all example targets
set(EXAMPLE_TARGETS
        quill_docs_binary_file_writer
        quill_docs_example_backend_options
        quill_docs_example_backtrace_logging_1
        quill_docs_example_backtrace_logging_2
        quill_docs_example_basic
        quill_docs_example_console
        quill_docs_example_csv_writer
        quill_docs_example_custom_format
        quill_docs_example_file
        quill_docs_example_filter
        quill_docs_example_json_logging
        quill_docs_example_loggers_remove
        quill_docs_example_mdc
        quill_docs_example_metric_publishing
        quill_docs_example_multiple_sinks
        quill_docs_example_multiple_sinks_tags
        quill_docs_example_tags_with_custom_sink
        quill_docs_example_unbuffered_stream
        quill_docs_quick_start
        quill_docs_timestamp_types
)

# Add example executables
foreach (example_target ${EXAMPLE_TARGETS})
    # Determine the source file from the target name
    set(source_file "${example_target}.cpp")

    add_executable(${example_target} ${source_file})
    set_common_compile_options(${example_target})
    target_link_libraries(${example_target} quill)
endforeach ()

if (TARGET quill_module)
    add_executable(quill_docs_module quill_docs_module.cpp)
    set_common_compile_options(quill_docs_module)
    target_link_libraries(quill_docs_module PRIVATE quill::quill_module)
    set_target_properties(quill_docs_module PROPERTIES CXX_SCAN_FOR_MODULES ON)
    list(APPEND EXAMPLE_TARGETS quill_docs_module)

    if (QUILL_BUILD_TESTS)
        add_test(NAME quill_module_logging_test COMMAND quill_docs_module)
        set_tests_properties(quill_module_logging_test PROPERTIES
                PASS_REGULAR_EXPRESSION "Logging through the Quill module: 42")
    endif ()
endif ()

install(TARGETS ${EXAMPLE_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
