cmake_minimum_required(VERSION 3.28)
project(quill_module_shared_library_test LANGUAGES CXX)

# The Linux executable and its startup-linked shared library both link the static module with hidden visibility.
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(QUILL_BUILD_MODULE ON)
set(QUILL_USE_SEQUENTIAL_THREAD_ID ON)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." quill)

add_library(quill_module_logger_owner SHARED LoggerOwner.cpp)
target_link_libraries(quill_module_logger_owner PRIVATE quill::quill_module)
set_common_compile_options(quill_module_logger_owner)

add_executable(quill_module_shared_thread_id_test SequentialThreadIdTest.cpp)
target_link_libraries(quill_module_shared_thread_id_test PRIVATE quill_module_logger_owner quill::quill_module)
set_common_compile_options(quill_module_shared_thread_id_test)

enable_testing()
add_test(NAME quill_module_shared_thread_id_test COMMAND quill_module_shared_thread_id_test)
set_tests_properties(quill_module_shared_thread_id_test PROPERTIES
        PASS_REGULAR_EXPRESSION "caller 2, worker 1\nBackend stopped")
