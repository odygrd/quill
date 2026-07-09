set(QUILL_TEST_COMMON_SOURCES
        TestMain.cpp
        TestUtilities.cpp
        DocTestExtensions.cpp)

add_library(quill_test_common STATIC ${QUILL_TEST_COMMON_SOURCES})
set_common_compile_options(quill_test_common)
target_link_libraries(quill_test_common PUBLIC quill)

target_include_directories(quill_test_common
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/test/misc>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/test/bundled>
        $<INSTALL_INTERFACE:include>)

if (QUILL_ENABLE_EXTENSIVE_TESTS)
    target_compile_definitions(quill_test_common PRIVATE QUILL_ENABLE_EXTENSIVE_TESTS)
endif ()

if (QUILL_NO_EXCEPTIONS)
    target_compile_definitions(quill_test_common PRIVATE DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS)
endif ()

set_property(TARGET quill_test_common PROPERTY CXX_STANDARD_REQUIRED ON)
