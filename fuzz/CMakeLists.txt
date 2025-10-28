function(quill_add_fuzzer FUZZER_NAME SOURCES)
    # Create fuzzer executable
    add_executable(${FUZZER_NAME} "")
    set_common_compile_options(${FUZZER_NAME})
    target_sources(${FUZZER_NAME} PRIVATE ${SOURCES})
    target_link_libraries(${FUZZER_NAME} quill)
    target_compile_options(${FUZZER_NAME} PRIVATE
            -fsanitize=fuzzer,address,undefined
            -fno-omit-frame-pointer
            -g
    )
    target_link_options(${FUZZER_NAME} PRIVATE
            -fsanitize=fuzzer,address,undefined
    )
    set_property(TARGET ${FUZZER_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
    set_target_properties(${FUZZER_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/test)
endfunction()

quill_add_fuzzer(FUZZ_BasicTypes BasicTypesFuzzer.cpp)
quill_add_fuzzer(FUZZ_StlContainers StlContainersFuzzer.cpp)
quill_add_fuzzer(FUZZ_UserDefinedDeferredFormat UserDefinedDeferredFormatFuzzer.cpp)
quill_add_fuzzer(FUZZ_UserDefinedDirectFormat UserDefinedDirectFormatFuzzer.cpp)
quill_add_fuzzer(FUZZ_QueueStress QueueStressFuzzer.cpp)
quill_add_fuzzer(FUZZ_BinaryData BinaryDataFuzzer.cpp)
