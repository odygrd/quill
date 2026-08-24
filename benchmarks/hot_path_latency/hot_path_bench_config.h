/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <chrono>

/**
 * When running the benchmark using e.g. perf, enable this definition to remove extra noise
 * from calculating and printing the results.
 *
 * To see shared cached lines :
 * perf c2c record -g --call-graph dwarf,8192  ./benchmark_quill_call_site_latency
 * perf c2c report -NN -g --call-graph -c pid,iaddr --stdio
 * perf c2c report -NN -g --call-graph -d lcl --stdio
 */
// #define PERF_ENABLED

#define THREAD_LIST_COUNT std::vector<uint16_t>{1, 4}

// Total messages emitted per iteration across all logging threads. Keeping this
// aggregate fixed makes the 1-thread and 4-thread latency results comparable.
#define MESSAGES_PER_ITERATION std::size_t{20}

#define ITERATIONS std::size_t{10'000}

// Give the backend time to catch up between batches. This benchmark measures
// hot-path latency under controlled load; sustained throughput belongs in a
// dedicated throughput benchmark.
#define MIN_WAIT_DURATION std::chrono::microseconds{2000}
#define MAX_WAIT_DURATION std::chrono::microseconds{2200}
