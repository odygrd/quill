add_library(my_shared_library SHARED lib.cpp)
target_link_libraries(my_shared_library quill)

add_executable(test_so test_so.cpp)
target_link_libraries(test_so quill)
target_link_options(test_so PRIVATE -rdynamic)