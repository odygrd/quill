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

    set_common_compile_options(${TEST_NAME})

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

quill_add_test(TEST_BoundedQueueTest BoundedQueueTest.cpp)
quill_add_test(TEST_DynamicFormatArgStore DynamicFormatArgStoreTest.cpp)
quill_add_test(TEST_FileUtilities FileUtilitiesTest.cpp)
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

if (NOT QUILL_USE_VALGRIND)
    quill_add_test(TEST_RdtscClock RdtscClockTest.cpp)
endif ()
