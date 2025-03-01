add_subdirectory(quill_wrapper_static)
add_subdirectory(foo_shared)

add_executable(quill_example_shared_lib_advanced example_shared_advanced.cpp)
set_common_compile_options(quill_example_shared_lib_advanced)
target_link_libraries(quill_example_shared_lib_advanced foo_shared)

# Optional
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(quill_example_shared_lib_advanced PRIVATE -fvisibility=hidden)
endif ()

# Required - Define QUILL_DLL_IMPORT when using the shared library on Windows
if (WIN32)
    # Post-build command to copy the DLL to the executable directory
    add_custom_command(TARGET quill_example_shared_lib_advanced POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:foo_shared>   # DLL location
            $<TARGET_FILE_DIR:quill_example_shared_lib_advanced>)  # Executable directory

    # The below will always copy
    #    # Custom target to copy all DLLs to the executable directory
    #    add_custom_target(copy_dll ALL
    #            COMMAND ${CMAKE_COMMAND} -E copy
    #            $<TARGET_FILE:foo_shared>          # DLL location
    #            $<TARGET_FILE_DIR:quill_example_shared_lib>  # Executable directory
    #            DEPENDS foo_shared # Ensure all shared libs are built before copying
    #    )
    #
    #    # Ensure that the custom target is executed after quill_example_shared_lib is built
    #    add_dependencies(quill_example_shared_lib copy_dll)

endif ()