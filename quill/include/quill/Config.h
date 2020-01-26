#pragma once

/**
 * Edit this file to squeeze more performance, or to customise supported
 * features
 *
 * It is easy to pass those flags using CMake e.g.
 * a) When invoking cmake : cmake . -DCMAKE_CXX_FLAGS="-DQUILL_ACTIVE_LOG_LEVEL=QUILL_LOG_LEVEL_INFO"
 * b) Hardcoded : target_compile_definitions(-DQUILL_ACTIVE_LOG_LEVEL=QUILL_LOG_LEVEL_INFO) or just compile_definitions for project wide
 */

// TODO: Add support for chrono::system_clock
/**
 * By default Quill will use the Time Stamp Counter (TSC) of the cpu to store the timestamps.
 *
 * The backend logging worker thread is constantly keeping track of the difference between the TSC
 * and the system clock resulting in accurate timestamps.
 *
 * This results in much better latencies on each caller thread. However, the initialisation time
 * in the beginning is slower as we have to take samples to convert TSC to nanoseconds
 *
 * If you care about initialisation time, or your cpu does not support invariant TSC uncomment it
 * to fallback to using std::chrono::system_clock
 *
 * @warning This will increase the caller thread latency
 */
// #define QUILL_CHRONO_SYSTEM_CLOCK

/**
 * Uncomment and set to completely compile out log level with zero cost.
 * Macros like LOG_TRACE_L3(..), LOG_TRACE_L2(..) will expand to empty statements
 * This helps reducing the number of the branches in your compiled code
 * By default the active log level is QUILL_LOG_LEVEL_TRACE_L3
 */
// #define QUILL_ACTIVE_LOG_LEVEL QUILL_LOG_LEVEL_INFO