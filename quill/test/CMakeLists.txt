function(quill_add_test TEST_NAME SOURCES)
    set(HEADER_FILES
            bundled/doctest/doctest.h
            misc/TestUtilities.h
            )

    set(ADD_SOURCE_FILES
            misc/TestMain.cpp
            misc/TestUtilities.cpp
            misc/DocTestExtensions.cpp)

    list(APPEND SOURCES ${ADD_SOURCE_FILES})

    # Create a test executable
    add_executable(${TEST_NAME} "")

    # Add sources
    target_sources(${TEST_NAME} PRIVATE ${SOURCES} ${HEADER_FILES})

    # include dirs
    target_include_directories(${TEST_NAME}
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/misc>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bundled>
            $<INSTALL_INTERFACE:include>
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR})

    # Link dependencies
    target_link_libraries(${TEST_NAME} quill)

    # Do not decay cxx standard if not specified
    set_property(TARGET ${TEST_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

    # Set output test directory
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/test)

    # Add this target to the post build unit tests
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} --duration=true)
endfunction()

quill_add_test(TEST_BoundedQueueTest.cpp BoundedQueueTest.cpp)
quill_add_test(TEST_FileUtilities FileUtilitiesTest.cpp)
quill_add_test(TEST_FreeListAllocator FreeListAllocatorTest.cpp)
quill_add_test(TEST_HandlerCollection HandlerCollectionTest.cpp)
quill_add_test(TEST_LoggerCollection LoggerCollectionTest.cpp)
quill_add_test(TEST_Logger LoggerTest.cpp)
quill_add_test(TEST_LogLevel LogLevelTest.cpp)
quill_add_test(TEST_MacroMetadata MacroMetadataTest.cpp)
quill_add_test(TEST_Log LogTest.cpp)
quill_add_test(TEST_PatternFormatter PatternFormatterTest.cpp)
quill_add_test(TEST_QuillStructuredLog QuillStructuredLogTest.cpp)
quill_add_test(TEST_QuillLog QuillLogTest.cpp)
quill_add_test(TEST_StringFromTime StringFromTimeTest.cpp)
quill_add_test(TEST_ThreadContextCollection ThreadContextCollectionTest.cpp)
quill_add_test(TEST_TimeRotatingFileHandler TimeRotatingFileHandlerTest.cpp)
quill_add_test(TEST_TimestampFormatter TimestampFormatterTest.cpp)
quill_add_test(TEST_TypeTraitsCopyable TypeTraitsCopyableTest.cpp)
quill_add_test(TEST_UnboundedQueueTest.cpp UnboundedQueueTest.cpp)
quill_add_test(TEST_Utilities UtilitiesTest.cpp)
quill_add_test(TEST_Utility UtilityTest.cpp)

if (NOT QUILL_USE_VALGRIND)
    quill_add_test(TEST_RdtscClock RdtscClockTest.cpp)
endif ()

include(../cmake/CodeCoverage.cmake)