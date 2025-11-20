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
    check_cxx_atomics_available(HAVE_CXX_ATOMIC)
    if (HAVE_CXX_ATOMIC)
        target_link_libraries(${TEST_NAME} quill)
    else()
        target_link_libraries(${TEST_NAME} quill atomic)
    endif()

    if (QUILL_ENABLE_EXTENSIVE_TESTS)
        target_compile_definitions(${TEST_NAME} PRIVATE QUILL_ENABLE_EXTENSIVE_TESTS)
    endif ()

    # Do not decay cxx standard if not specified
    set_property(TARGET ${TEST_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

    # Set output test directory
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/test)

    # Add this target to the post build unit tests
    # Set sanitizer environment variables for test execution if sanitizers are enabled
    if (QUILL_SANITIZE_ADDRESS)
        doctest_discover_tests(${TEST_NAME} PROPERTIES
            ENVIRONMENT "ASAN_OPTIONS=detect_leaks=1:check_initialization_order=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
    else()
        doctest_discover_tests(${TEST_NAME})
    endif()
endfunction()

include(Doctest)

quill_add_test(TEST_ArithmeticTypesLogging ArithmeticTypesLoggingTest.cpp)
quill_add_test(TEST_BackendExceptionNotifier BackendExceptionNotifierTest.cpp)
quill_add_test(TEST_BackendLongSleepAndNotify BackendLongSleepAndNotifyTest.cpp)
quill_add_test(TEST_BackendTransitBufferHardLimit BackendTransitBufferHardLimitTest.cpp)
quill_add_test(TEST_BackendTransitBufferSoftLimit BackendTransitBufferSoftLimitTest.cpp)
quill_add_test(TEST_BackendTscClock BackendTscClockTest.cpp)
quill_add_test(TEST_BacktraceDynamicLogLevel BacktraceDynamicLogLevelTest.cpp)
quill_add_test(TEST_BacktraceFlushOnError BacktraceFlushOnErrorTest.cpp)
quill_add_test(TEST_BacktraceMultithreadedStressTest BacktraceMultithreadedStressTest.cpp)
quill_add_test(TEST_BacktraceManualFlush BacktraceManualFlushTest.cpp)
quill_add_test(TEST_BacktraceNoFlush BacktraceNoFlushTest.cpp)
quill_add_test(TEST_BacktraceTerminatedThreadFlush BacktraceTerminatedThreadFlushTest.cpp)
quill_add_test(TEST_BinaryDataLoggingTest BinaryDataLoggingTest.cpp)
quill_add_test(TEST_BinaryFileWriter BinaryFileWriterTest.cpp)
quill_add_test(TEST_BoundedBlockingQueue BoundedBlockingQueueTest.cpp)
quill_add_test(TEST_BoundedDroppingQueue BoundedDroppingQueueTest.cpp)
quill_add_test(TEST_CArrayTypesLogging CArrayTypesLoggingTest.cpp)
quill_add_test(TEST_BoundedDroppingQueueDropMessages BoundedDroppingQueueDropMessagesTest.cpp)
quill_add_test(TEST_ConsoleSinkStderrMultipleFormats ConsoleSinkStderrMultipleFormatsTest.cpp)
quill_add_test(TEST_ConsoleSinkStdoutMultipleFormats ConsoleSinkStdoutMultipleFormatsTest.cpp)
quill_add_test(TEST_CsvWriting CsvWritingTest.cpp)
quill_add_test(TEST_CsvWritingCustomFrontend CsvWritingCustomFrontendTest.cpp)
quill_add_test(TEST_EnumLogging EnumLoggingTest.cpp)
quill_add_test(TEST_FlushMultipleLoggers FlushMultipleLoggers.cpp)
quill_add_test(TEST_FlushWithoutAnyLog FlushWithoutAnyLog.cpp)
quill_add_test(TEST_JsonConsoleLogging JsonConsoleLoggingTest.cpp)
quill_add_test(TEST_JsonFileLogging JsonFileLoggingTest.cpp)
quill_add_test(TEST_JsonMultilineMetadata JsonMultilineMetadataTest.cpp)
quill_add_test(TEST_JsonVariedParamsLogging JsonVariedParamsLoggingTest.cpp)
quill_add_test(TEST_LogArgumentsEvaluation LogArgumentsEvaluationTest.cpp)
quill_add_test(TEST_LogFlushWithSoftLimit LogFlushWithSoftLimitTest.cpp)
quill_add_test(TEST_LoggerAddRemoveGet LoggerAddRemoveGetTest.cpp)
quill_add_test(TEST_MacroFreeBacktraceFlushTest MacroFreeBacktraceFlushTest.cpp)
quill_add_test(TEST_MacroFreeJsonLoggingTest MacroFreeJsonLoggingTest.cpp)
quill_add_test(TEST_MacroFreeMultiFrontendThreadsTest MacroFreeMultiFrontendThreadsTest.cpp)
quill_add_test(TEST_MacrosAndFunctionsLoggingTest MacrosAndFunctionsLoggingTest.cpp)
quill_add_test(TEST_MacrosDisableFileFunctionTest MacrosDisableFileFunctionTest.cpp)
quill_add_test(TEST_Macros MacrosTest.cpp)
quill_add_test(TEST_MultiFrontendThreads MultiFrontendThreadsTest.cpp)
quill_add_test(TEST_MultiFrontendThreadsTimestampOrderTest MultiFrontendThreadsTimestampOrderTest.cpp)
quill_add_test(TEST_MultilineMetadata MultilineMetadataTest.cpp)
quill_add_test(TEST_MultilineMetadataOverrideFormatTest MultilineMetadataOverrideFormatTest.cpp)
quill_add_test(TEST_MultipleSinksSameLogger MultipleSinksSameLoggerTest.cpp)
quill_add_test(TEST_OverrideSinkFormatter OverrideSinkFormatterTest.cpp)
quill_add_test(TEST_RemoveLoggerBlocking RemoveLoggerBlockingTest.cpp)
quill_add_test(TEST_RotatingSinkKeepOldest RotatingSinkKeepOldestTest.cpp)
quill_add_test(TEST_RotatingSinkOverwriteOldest RotatingSinkOverwriteOldestTest.cpp)
quill_add_test(TEST_RotatingSinkSizeRotation RotatingSinkSizeRotationTest.cpp)
quill_add_test(TEST_RuntimeMetadata RuntimeMetadataTest.cpp)
quill_add_test(TEST_ShrinkThreadLocalQueueTest ShrinkThreadLocalQueueTest.cpp)
quill_add_test(TEST_SignalHandler SignalHandlerTest.cpp)
quill_add_test(TEST_SignalHandlerLogger SignalHandlerLoggerTest.cpp)
quill_add_test(TEST_SingleFrontendThreadMultipleLoggers SingleFrontendThreadMultipleLoggersTest.cpp)
quill_add_test(TEST_SingleFrontendThread SingleFrontendThreadTest.cpp)
quill_add_test(TEST_SinkFilter SinkFilterTest.cpp)
quill_add_test(TEST_SinkFilterOverrideFormat SinkFilterOverrideFormatTest.cpp)
quill_add_test(TEST_StdArrayLogging StdArrayLoggingTest.cpp)
quill_add_test(TEST_StdChronoLogging StdChronoLoggingTest.cpp)
quill_add_test(TEST_StdDequeLogging StdDequeLoggingTest.cpp)
quill_add_test(TEST_StdFilesystemPathLogging StdFilesystemPathLoggingTest.cpp)
quill_add_test(TEST_StdForwardListLogging StdForwardListLoggingTest.cpp)
quill_add_test(TEST_StdListLogging StdListLoggingTest.cpp)
quill_add_test(TEST_StdMapLogging StdMapLoggingTest.cpp)
quill_add_test(TEST_StdMultiMapLogging StdMultiMapLoggingTest.cpp)
quill_add_test(TEST_StdMultiSetLogging StdMultiSetLoggingTest.cpp)
quill_add_test(TEST_StdOptionalLogging StdOptionalLoggingTest.cpp)
quill_add_test(TEST_StdPairLogging StdPairLoggingTest.cpp)
quill_add_test(TEST_StdSetLogging StdSetLoggingTest.cpp)
quill_add_test(TEST_StdTupleLogging StdTupleLoggingTest.cpp)
quill_add_test(TEST_StdUnorderedMapLogging StdUnorderedMapLoggingTest.cpp)
quill_add_test(TEST_StdUnorderedMultiMapLogging StdUnorderedMultiMapLoggingTest.cpp)
quill_add_test(TEST_StdUnorderedMultiSetLogging StdUnorderedMultiSetLoggingTest.cpp)
quill_add_test(TEST_StdUnorderedSetLogging StdUnorderedSetLoggingTest.cpp)
quill_add_test(TEST_StdVectorLogging StdVectorLoggingTest.cpp)
quill_add_test(TEST_SinkLogLevelFilter SinkLogLevelFilterTest.cpp)
quill_add_test(TEST_StartStopBackendWorker StartStopBackendWorkerTest.cpp)
quill_add_test(TEST_StringLargeLogging StringLargeLoggingTest.cpp)
quill_add_test(TEST_StringLoggingDynamicLogLevel StringLoggingDynamicLogLevelTest.cpp)
quill_add_test(TEST_StringLogging StringLoggingTest.cpp)
quill_add_test(TEST_StringRandomLargeLogging StringRandomLargeLoggingTest.cpp)
quill_add_test(TEST_StringRandomLogging StringRandomLoggingTest.cpp)
quill_add_test(TEST_StringRandomSmallLogging StringRandomSmallLoggingTest.cpp)
quill_add_test(TEST_StringRef StringRefTest.cpp)
quill_add_test(TEST_TagsLogging TagsLoggingTest.cpp)
quill_add_test(TEST_UnboundedUnlimitedQueue UnboundedUnlimitedQueueTest.cpp)
quill_add_test(TEST_UserClockSource UserClockSourceTest.cpp)
quill_add_test(TEST_UserDefinedTypeLoggingDeferredFormatTest UserDefinedTypeLoggingDeferredFormatTest.cpp)
quill_add_test(TEST_UserDefinedTypeDeferredFormatCopyMove UserDefinedTypeDeferredFormatCopyMoveTest.cpp)
quill_add_test(TEST_UserDefinedTypeLoggingDirectFormatTest UserDefinedTypeLoggingDirectFormatTest.cpp)
quill_add_test(TEST_UserDefinedTypeLogging UserDefinedTypeLoggingTest.cpp)
quill_add_test(TEST_UserSink UserSinkTest.cpp)
quill_add_test(TEST_VariableLogging VariableLoggingTest.cpp)

quill_add_test(TEST_CompileActiveLogLevel CompileActiveLogLevelTest.cpp)
target_compile_definitions(TEST_CompileActiveLogLevel PRIVATE -DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)

if (WIN32)
    quill_add_test(TEST_WideStdTypesLogging WideStdTypesLoggingTest.cpp)
    quill_add_test(TEST_WideStringLogging WideStringLoggingTest.cpp)
endif ()
