function(quill_add_test TEST_NAME SOURCES)
    set(HEADER_FILES
            ../bundled/doctest/doctest.h
            ../misc/TestUtilities.h
    )

    set(ADD_SOURCE_FILES
            ../misc/TestMain.cpp
            ../misc/TestUtilities.cpp
            ../misc/DocTestExtensions.cpp)

    list(APPEND SOURCES ${ADD_SOURCE_FILES})

    # Create a test executable
    add_executable(${TEST_NAME} "")

    set_common_compile_options(${TEST_NAME})

    # Add sources
    target_sources(${TEST_NAME} PRIVATE ${SOURCES} ${HEADER_FILES})

    # include dirs
    target_include_directories(${TEST_NAME}
            PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/test/misc>
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/test/bundled>
            $<INSTALL_INTERFACE:include>
            PRIVATE
            ..)

    # Link dependencies
    target_link_libraries(${TEST_NAME} quill)

    # Do not decay cxx standard if not specified
    set_property(TARGET ${TEST_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

    # Set output test directory
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/test)

    # Add this target to the post build unit tests
    doctest_discover_tests(${TEST_NAME})
endfunction()

include(Doctest)

quill_add_test(TEST_BoundedQueueTest BoundedQueueTest.cpp)
quill_add_test(TEST_DynamicFormatArgStore DynamicFormatArgStoreTest.cpp)
quill_add_test(TEST_FileEventNotifier FileEventNotifierTest.cpp)
quill_add_test(TEST_FileSink FileSinkTest.cpp)
quill_add_test(TEST_FileUtilities FileUtilitiesTest.cpp)
quill_add_test(TEST_InlinedVector InlinedVectorTest.cpp)
quill_add_test(TEST_LoggerManager LoggerManagerTest.cpp)
quill_add_test(TEST_Logger LoggerTest.cpp)
quill_add_test(TEST_LogLevel LogLevelTest.cpp)
quill_add_test(TEST_MacroMetadata MacroMetadataTest.cpp)
quill_add_test(TEST_MathUtils MathUtilsTest.cpp)
quill_add_test(TEST_PatternFormatter PatternFormatterTest.cpp)
quill_add_test(TEST_RotatingFileSink RotatingFileSinkTest.cpp)
quill_add_test(TEST_SinkManager SinkManagerTest.cpp)
quill_add_test(TEST_StringFromTime StringFromTimeTest.cpp)
quill_add_test(TEST_ThreadContextManager ThreadContextManagerTest.cpp)
quill_add_test(TEST_TimestampFormatter TimestampFormatterTest.cpp)
quill_add_test(TEST_TransitEventBuffer TransitEventBufferTest.cpp)
quill_add_test(TEST_UnboundedQueue.cpp UnboundedQueueTest.cpp)
quill_add_test(TEST_Utility UtilityTest.cpp)

if (NOT QUILL_SANITIZE_THREAD)
    # clang++12 thread sanitizer throws exception for unknown reason in the CI
    quill_add_test(TEST_UnboundedQueueLimit.cpp UnboundedQueueLimitTest.cpp)
endif ()

if (NOT QUILL_USE_VALGRIND)
    quill_add_test(TEST_RdtscClock RdtscClockTest.cpp)
endif ()
