add_subdirectory(quill_wrapper_shared)

add_executable(quill_example_shared_lib example_shared.cpp)
set_common_compile_options(quill_example_shared_lib)
target_link_libraries(quill_example_shared_lib quill_wrapper_shared)

# Optional
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(quill_example_shared_lib PRIVATE -fvisibility=hidden)
endif ()

# Required - Define QUILL_DLL_IMPORT when using the shared library on Windows
if (WIN32)
    target_compile_definitions(quill_example_shared_lib PRIVATE QUILL_DLL_IMPORT)
endif ()