cmake_minimum_required(VERSION 3.16)
project(my_project)

set(CMAKE_CXX_STANDARD 20)

add_executable(example_01 example_01.cpp)
target_link_libraries(example_01 quill)