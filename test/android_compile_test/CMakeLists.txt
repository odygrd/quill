cmake_minimum_required(VERSION 3.12...4.0)
project(quill_android_compile_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." quill)

add_executable(quill_android_compile_test AndroidCompileTest.cpp)
target_link_libraries(quill_android_compile_test PRIVATE quill::quill log)
