# The same harness, scenarios and payloads as the call site latency benchmarks of
# https://github.com/odygrd/logger_benchmarks, see hot_path_bench_config.h for the scenarios.
#
# Every benchmark is built once per payload and scenario, as
# BENCHMARK_quill_hot_path_<variant>_<payload>_<scenario>, and the disabled level benchmarks as
# BENCHMARK_quill_hot_path_<variant>_disabled and BENCHMARK_quill_hot_path_<variant>_disabled_argument.
# Each benchmark writes its log to <target>.log in the working directory.

set(QUILL_HOT_PATH_ALL_PAYLOADS int_int_double int_int_largestr vector_largestr)
set(QUILL_HOT_PATH_ALL_SCENARIOS burst single cold)

function(add_hot_path_latency_target TARGET_NAME SOURCE)
    add_executable(${TARGET_NAME} hot_path_bench_config.h hot_path_bench.h quill_hot_path_frontend_options.h ${SOURCE})
    set_common_compile_options(${TARGET_NAME})
    target_compile_definitions(${TARGET_NAME} PRIVATE ${ARGN} BENCH_LOG_FILENAME="${TARGET_NAME}.log")
    target_link_libraries(${TARGET_NAME} quill)
endfunction()

# add_hot_path_latency_benchmark(<variant> SOURCE <file>
#                                [PAYLOADS <payloads...>] [SCENARIOS <scenarios...>]
#                                [DEFINITIONS <definitions...>] [DISABLED])
function(add_hot_path_latency_benchmark VARIANT)
    cmake_parse_arguments(ARG "DISABLED" "SOURCE" "PAYLOADS;SCENARIOS;DEFINITIONS" ${ARGN})

    if (NOT ARG_PAYLOADS)
        set(ARG_PAYLOADS ${QUILL_HOT_PATH_ALL_PAYLOADS})
    endif ()
    if (NOT ARG_SCENARIOS)
        set(ARG_SCENARIOS ${QUILL_HOT_PATH_ALL_SCENARIOS})
    endif ()

    foreach (PAYLOAD ${ARG_PAYLOADS})
        string(TOUPPER "BENCH_${PAYLOAD}" PAYLOAD_DEFINITION)
        foreach (SCENARIO ${ARG_SCENARIOS})
            # burst is the default scenario
            set(SCENARIO_DEFINITION)
            if (NOT SCENARIO STREQUAL "burst")
                string(TOUPPER "BENCH_SCENARIO_${SCENARIO}" SCENARIO_DEFINITION)
            endif ()
            add_hot_path_latency_target(BENCHMARK_quill_hot_path_${VARIANT}_${PAYLOAD}_${SCENARIO} ${ARG_SOURCE}
                    ${PAYLOAD_DEFINITION} ${SCENARIO_DEFINITION} ${ARG_DEFINITIONS})
        endforeach ()
    endforeach ()

    if (ARG_DISABLED)
        add_hot_path_latency_target(BENCHMARK_quill_hot_path_${VARIANT}_disabled ${ARG_SOURCE}
                BENCH_INT_INT_DOUBLE BENCH_SCENARIO_DISABLED ${ARG_DEFINITIONS})

        # The same statement with an argument that is expensive to compute
        add_hot_path_latency_target(BENCHMARK_quill_hot_path_${VARIANT}_disabled_argument ${ARG_SOURCE}
                BENCH_INT_INT_DOUBLE BENCH_SCENARIO_DISABLED BENCH_DISABLED_EXPENSIVE_ARG ${ARG_DEFINITIONS})
    endif ()
endfunction()

# The LOG_ macros with the default unbounded queue and TSC timestamps
add_hot_path_latency_benchmark(rdtsc_clock DISABLED SOURCE quill_hot_path_latency.cpp)

# The macro-free functions with the default unbounded queue and TSC timestamps
add_hot_path_latency_benchmark(macro_free_rdtsc_clock DISABLED SOURCE quill_hot_path_latency_macro_free.cpp)

# The LOG_ macros with a bounded dropping queue of 512 KiB per thread
add_hot_path_latency_benchmark(bounded_rdtsc_clock SOURCE quill_hot_path_latency.cpp
        DEFINITIONS QUILL_USE_BOUNDED_DROPPING_QUEUE)

# The LOG_ macros with system clock timestamps instead of the TSC. The clock does not depend on
# the payload, so only int_int_double is built.
add_hot_path_latency_benchmark(system_clock SOURCE quill_hot_path_latency.cpp
        PAYLOADS int_int_double DEFINITIONS BENCH_QUILL_SYSTEM_CLOCK)

# The measurement floor: the harness with a call that logs nothing
add_hot_path_latency_benchmark(timer_only DISABLED SOURCE timer_only_hot_path_latency.cpp)
