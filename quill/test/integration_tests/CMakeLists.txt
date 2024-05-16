function(quill_add_test TEST_NAME SOURCES)
    set(HEADER_FILES
            ${PROJECT_SOURCE_DIR}/quill/test/bundled/doctest/doctest.h
            ${PROJECT_SOURCE_DIR}/quill/test/misc/TestUtilities.h
    )

    set(ADD_SOURCE_FILES
            ${PROJECT_SOURCE_DIR}/quill/test/misc/TestMain.cpp
            ${PROJECT_SOURCE_DIR}/quill/test/misc/TestUtilities.cpp
            ${PROJECT_SOURCE_DIR}/quill/test/misc/DocTestExtensions.cpp)

    list(APPEND SOURCES ${ADD_SOURCE_FILES})

    # Create a test executable
    add_executable(${TEST_NAME} "")

    # Add sources
    target_sources(${TEST_NAME} PRIVATE ${SOURCES} ${HEADER_FILES})

    # include dirs
    target_include_directories(${TEST_NAME}
            PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/quill/test/misc>
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/quill/test/bundled>
            $<INSTALL_INTERFACE:include>
            PRIVATE
            ${PROJECT_SOURCE_DIR}/quill/test)

    # Link dependencies
    target_link_libraries(${TEST_NAME} quill)

    # Do not decay cxx standard if not specified
    set_property(TARGET ${TEST_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

    # Set output test directory
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/test)

    # Add this target to the post build unit tests
    doctest_discover_tests(${TEST_NAME})
endfunction()

include(${PROJECT_SOURCE_DIR}/cmake/doctest.cmake)

quill_add_test(TEST_ArithmeticTypesLogging ArithmeticTypesLoggingTest.cpp)
quill_add_test(TEST_BackendExceptionNotifier BackendExceptionNotifierTest.cpp)
quill_add_test(TEST_BackendLongSleepAndNotify BackendLongSleepAndNotifyTest.cpp)
quill_add_test(TEST_BackendTransitBufferHardLimit BackendTransitBufferHardLimitTest.cpp)
quill_add_test(TEST_BacktraceDynamicLogLevel BacktraceDynamicLogLevelTest.cpp)
quill_add_test(TEST_BacktraceFlushOnError BacktraceFlushOnErrorTest.cpp)
quill_add_test(TEST_BacktraceManualFlush BacktraceManualFlushTest.cpp)
quill_add_test(TEST_BacktraceNoFlush BacktraceNoFlushTest.cpp)
quill_add_test(TEST_BacktraceTerminatedThreadFlush BacktraceTerminatedThreadFlushTest.cpp)
quill_add_test(TEST_BoundedBlockingQueue BoundedBlockingQueueTest.cpp)
quill_add_test(TEST_BoundedDroppingQueue BoundedDroppingQueueTest.cpp)
quill_add_test(TEST_ConsoleSinkStderrMultipleFormats ConsoleSinkStderrMultipleFormatsTest.cpp)
quill_add_test(TEST_ConsoleSinkStdoutMultipleFormats ConsoleSinkStdoutMultipleFormatsTest.cpp)
quill_add_test(TEST_LoggerAddRemoveGet LoggerAddRemoveGetTest.cpp)
quill_add_test(TEST_EnumLogging EnumLoggingTest.cpp)
quill_add_test(TEST_FlushWithoutAnyLog FlushWithoutAnyLog.cpp)
quill_add_test(TEST_JsonConsoleLoggingTest JsonConsoleLoggingTest.cpp)
quill_add_test(TEST_JsonFileLogging JsonFileLoggingTest.cpp)
quill_add_test(TEST_LogArgumentsEvaluation LogArgumentsEvaluationTest.cpp)
quill_add_test(TEST_MultiFrontendThreads MultiFrontendThreadsTest.cpp)
quill_add_test(TEST_MultipleSinksSameLogger MultipleSinksSameLoggerTest.cpp)
quill_add_test(TEST_RotatingSinkDailyRotation RotatingSinkDailyRotationTest.cpp)
quill_add_test(TEST_RotatingSinkKeepOldest RotatingSinkKeepOldestTest.cpp)
quill_add_test(TEST_RotatingSinkOverwriteOldest RotatingSinkOverwriteOldestTest.cpp)
quill_add_test(TEST_SingleFrontendThread SingleFrontendThreadTest.cpp)
quill_add_test(TEST_SinkFilter SinkFilterTest.cpp)
quill_add_test(TEST_StdArrayLogging StdArrayLoggingTest.cpp)
quill_add_test(TEST_SinkLogLevelFilter SinkLogLevelFilterTest.cpp)
quill_add_test(TEST_StringLargeLogging StringLargeLoggingTest.cpp)
quill_add_test(TEST_StringLoggingDynamicLogLevel StringLoggingDynamicLogLevelTest.cpp)
quill_add_test(TEST_StringLogging StringLoggingTest.cpp)
quill_add_test(TEST_StringRandomLargeLogging StringRandomLargeLoggingTest.cpp)
quill_add_test(TEST_StringRandomLogging StringRandomLoggingTest.cpp)
quill_add_test(TEST_StringRandomSmallLogging StringRandomSmallLoggingTest.cpp)
quill_add_test(TEST_TagsLogging TagsLoggingTest.cpp)
quill_add_test(TEST_UserClockSource UserClockSourceTest.cpp)

if (WIN32)
    quill_add_test(TEST_WideStringLogging WideStringLoggingTest.cpp)
endif ()
