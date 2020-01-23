set(TEST_NAME "TEST_quill")

set(SOURCE_FILES
        #        BoundedSPSCQueueTest.cpp
        #        LoggerCollectionTest.cpp
        #        LoggerTest.cpp
        #        LogLevelTest.cpp
        #        LogLineInfoTest.cpp
        LogTest.cpp
        #        MessageTest.cpp
        #PatternFormatterTest.cpp
        #        TestMain.cpp
        #        ThreadContextCollectionTest.cpp
        )

# Create a test executable
add_executable(${TEST_NAME} "")

# Add sources
target_sources(${TEST_NAME} PRIVATE ${SOURCE_FILES})

# Add compiler options for this library
target_compile_options(${TEST_NAME} PRIVATE ${TEST_COPTS} ${EXCEPTIONS_FLAG})

# Link dependencies
target_link_libraries(${TEST_NAME} ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} quill)

# Do not decay cxx standard if not specified
set_property(TARGET ${TEST_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

# Set output test directory
set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/test)

# Add this target to the post build unit tests
add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} --gtest_output=xml:${TEST_OUTPUT_DIR}/${TEST_NAME}_report.xml)