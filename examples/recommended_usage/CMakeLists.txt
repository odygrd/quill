add_subdirectory(quill_static_lib)

set(EXAMPLE_TARGETS
        quill_example_recommended_usage
        quill_example_std_types_logging
        quill_example_use_overwrite_macros
)

# Add example executables
foreach (example_target ${EXAMPLE_TARGETS})
    # Determine the source file from the target name
    string(REPLACE "quill_example_" "" source_name ${example_target})
    set(source_file "${source_name}.cpp")

    add_executable(${example_target} ${source_file})
    set_common_compile_options(${example_target})
    target_link_libraries(${example_target} quill_static_lib_recommended_usage)
endforeach ()

install(TARGETS ${EXAMPLE_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
