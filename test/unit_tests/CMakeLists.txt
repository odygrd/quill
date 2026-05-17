function(quill_add_test TEST_NAME SOURCES)
    list(APPEND SOURCES ${ARGN})

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

    if (NOT MSVC)
        # disable false warning on armel/armhf
        set_source_files_properties(PatternFormatterTest.cpp PROPERTIES
                COMPILE_OPTIONS "-Wno-unused-value")
    endif ()

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

    if (QUILL_ENABLE_EXTENSIVE_TESTS)
        target_compile_definitions(${TEST_NAME} PRIVATE QUILL_ENABLE_EXTENSIVE_TESTS)
    endif ()

    if (QUILL_NO_EXCEPTIONS)
        target_compile_definitions(${TEST_NAME} PRIVATE DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS)
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
    else ()
        doctest_discover_tests(${TEST_NAME})
    endif ()
endfunction()

include(Doctest)

quill_add_test(TEST_BoundedQueueTest BoundedQueueTest.cpp)
quill_add_test(TEST_BacktraceStorage BacktraceStorageTest.cpp)
quill_add_test(TEST_BackendMdcState BackendMdcStateTest.cpp)
quill_add_test(TEST_BackendUtilities BackendUtilitiesTest.cpp)
quill_add_test(TEST_ChronoCodec ChronoCodecTest.cpp)
quill_add_test(TEST_ChronoTimeUtils ChronoTimeUtilsTest.cpp)
quill_add_test(TEST_ConsoleSink ConsoleSinkTest.cpp)
quill_add_test(TEST_DynamicFormatArgStore DynamicFormatArgStoreTest.cpp)
quill_add_test(TEST_FileEventNotifier FileEventNotifierTest.cpp)
quill_add_test(TEST_FileSink FileSinkTest.cpp)
quill_add_test(TEST_FileUtilities FileUtilitiesTest.cpp)
quill_add_test(TEST_InlinedVector InlinedVectorTest.cpp)
quill_add_test(TEST_LoggerManager LoggerManagerTest.cpp)
quill_add_test(TEST_Logger LoggerTest.cpp)
quill_add_test(TEST_LogLevel LogLevelTest.cpp)
quill_add_test(TEST_MacroMetadata MacroMetadataTest.cpp)
quill_add_test(TEST_MathUtilities MathUtilitiesTest.cpp)
quill_add_test(TEST_MetricManager MetricManagerTest.cpp)
quill_add_test(TEST_PatternFormatter PatternFormatterTest.cpp)
quill_add_test(TEST_RotatingFileSink RotatingFileSinkTest.cpp)
quill_add_test(TEST_SinkAddFilter SinkAddFilterTest.cpp)
quill_add_test(TEST_SinkManager SinkManagerTest.cpp)
quill_add_test(TEST_StringFromTime StringFromTimeTest.cpp)
quill_add_test(TEST_SequentialThreadId SequentialThreadIdTest.cpp)
quill_add_test(TEST_ThreadContextManager ThreadContextManagerTest.cpp)
quill_add_test(TEST_TimestampFormatter TimestampFormatterTest.cpp)
quill_add_test(TEST_TransitEventBuffer TransitEventBufferTest.cpp)
quill_add_test(TEST_UnboundedQueue UnboundedQueueTest.cpp)
quill_add_test(TEST_Utility UtilityTest.cpp)

if (NOT QUILL_USE_VALGRIND)
    quill_add_test(TEST_RdtscClock RdtscClockTest.cpp)
    quill_add_test(TEST_StopWatch StopWatchTest.cpp)
endif ()
