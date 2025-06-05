add_subdirectory(quill_shared_lib)

add_executable(quill_example_shared_lib example_shared.cpp)
set_common_compile_options(quill_example_shared_lib)
target_link_libraries(quill_example_shared_lib quill_shared_lib)

# Optional - apply visibility flag except for clang-cl
if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER MATCHES "clang-cl"))
    target_compile_options(quill_example_shared_lib PRIVATE -fvisibility=hidden)
endif ()

# Required - Define QUILL_DLL_IMPORT when using the shared library on Windows
if (WIN32)
    target_compile_definitions(quill_example_shared_lib PRIVATE QUILL_DLL_IMPORT)

    # Post-build command to copy the DLL to the executable directory
    add_custom_command(TARGET quill_example_shared_lib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:quill_shared_lib>   # DLL location
            $<TARGET_FILE_DIR:quill_example_shared_lib>)  # Executable directory

    # The below will always copy
    #    # Custom target to copy all DLLs to the executable directory
    #    add_custom_target(copy_dll ALL
    #            COMMAND ${CMAKE_COMMAND} -E copy
    #            $<TARGET_FILE:quill_shared_lib>          # DLL location
    #            $<TARGET_FILE_DIR:quill_example_shared_lib>  # Executable directory
    #            DEPENDS quill_shared_lib # Ensure all shared libs are built before copying
    #    )
    #
    #    # Ensure that the custom target is executed after quill_example_shared_lib is built
    #    add_dependencies(quill_example_shared_lib copy_dll)

endif ()