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

#define THREAD_LIST_COUNT                                                                          \
  std::vector<uint16_t> { 1, 4 }

#define MESSAGES_PER_ITERATION                                                                     \
  std::size_t { 20 }

#define ITERATIONS                                                                                 \
  std::size_t { 10000 }

/**
 * Min-Max wait duration between each iteration - This lets the backend thread catch up
 * a little bit with the caller thread, because the caller thread is so much faster.
 * When the backend thread can't catch up it will cause the caller thread on the hot path
 * to reallocate more space in the queue slowing it down.
 * This benchmark is measuring latency not high throughput
 * **/
#define MIN_WAIT_DURATION                                                                          \
  std::chrono::microseconds { 2000 }

#define MAX_WAIT_DURATION                                                                          \
  std::chrono::microseconds { 2200 }
