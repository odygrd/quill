set(TEST_NAME "TEST_quill")

set(HEADER_FILES
        misc/TestMain.cpp
        misc/TestUtilities.cpp
        misc/TestUtilities.h
        )

set(SOURCE_FILES
        BoundedSPSCQueueTest.cpp
        ConfigTest.cpp
        FileUtilitiesTest.cpp
        HandlerCollectionTest.cpp
        LoggerCollectionTest.cpp
        LoggerTest.cpp
        LogLevelTest.cpp
        LogTest.cpp
        PatternFormatterTest.cpp
        QuillLogTest.cpp
        RecordTest.cpp
        StaticLogRecordInfoTest.cpp
        ThreadContextCollectionTest.cpp
        TimestampFormatterTest.cpp
        UtilityTest.cpp
        )

# Create a test executable
add_executable(${TEST_NAME} "")

# Add sources
target_sources(${TEST_NAME} PRIVATE ${SOURCE_FILES} ${HEADER_FILES})

# include dirs
target_include_directories(${TARGET_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/misc>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Link dependencies
target_link_libraries(${TEST_NAME} quill ${GTEST_BOTH_LIBRARIES})

# Do not decay cxx standard if not specified
set_property(TARGET ${TEST_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

# Set output test directory
set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/test)

# Add this target to the post build unit tests
add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} --gtest_output=xml:${TEST_NAME}_report.xml)