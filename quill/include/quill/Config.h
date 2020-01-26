#pragma once

/**
 * Edit this file to squeeze more performance, or to customise supported
 * features
 *
 * All the values below can either be passed as compiler flags or uncommented here
 *
 * HINT:
 * It is better passing those flags using CMake or during compilation instead of uncommenting them
 * here as this gives more flexibility invoking different builds by passing different flags rather
 * than editing this file
 *
 * a) When invoking cmake : cmake . -DCMAKE_CXX_FLAGS="-DQUILL_ACTIVE_LOG_LEVEL=QUILL_LOG_LEVEL_INFO"
 */

// TODO: Add support for chrono::system_clock
/**
 * By default Quill will use the Time Stamp Counter (TSC) of the cpu to store the timestamps.
 *
 * The backend logging worker thread is constantly keeping track of the difference between the TSC
 * and the system clock resulting in accurate timestamps.
 *
 * TSC clock gives better latencies on the caller thread. However, the initialisation time
 * in the beginning is slower as we have to take samples to convert TSC to nanoseconds
 *
 * If you care about initialisation time, or your cpu does not support invariant TSC set this flag
 * to QUILL_CHRONO_SYSTEM_CLOCK to fallback to using std::chrono::system_clock. Warning: This
 * will increase caller thread latencies
 *
 * Options:
 *
 * QUILL_CHRONO_SYSTEM_CLOCK
 * QUILL_TSC_CLOCK
 */
// #define QUILL_CHRONO_SYSTEM_CLOCK QUILL_TSC_CLOCK

/**
 * Completely compiles out log level with zero cost.
 * Macros like LOG_TRACE_L3(..), LOG_TRACE_L2(..) will expand to empty statements
 * This helps reducing the number of branches in your compiled code
 *
 * The default value is QUILL_LOG_LEVEL_TRACE_L3
 *
 * Options:
 *
 * QUILL_LOG_LEVEL_TRACE_L3
 * QUILL_LOG_LEVEL_TRACE_L2
 * QUILL_LOG_LEVEL_TRACE_L1
 * QUILL_LOG_LEVEL_DEBUG
 * QUILL_LOG_LEVEL_INFO
 * QUILL_LOG_LEVEL_WARNING
 * QUILL_LOG_LEVEL_ERROR
 * QUILL_LOG_LEVEL_CRITICAL
 */
// #define QUILL_ACTIVE_LOG_LEVEL QUILL_LOG_LEVEL_TRACE_L3

/**
 * The backend logging worker thread will always "busy wait" spinning around the thread local
 * queues.
 *
 * The reason for this is to reduce latency on the caller thread as notifying the
 * backend thread even by a fast backed by atomics fast semaphone adds additional latency
 * to the caller thread.
 * The alternative to this is letting the backend thread busy wait and reduce the thread's
 * priority on the OS scheduler by a call to sleep.
 *
 * Each time the backend thread sees that there are no messages left in the queues it sleeps.
 * It is recommended to pin the backend logger thread to a shared or junk cpu core and use the
 * default sleep duration of 500ns.
 * However, if you really care about the backend logging worker thread speed you might want to pin
 * the logging worker thread to an exclusive core and change this value to 0.
 */
// #define QUILL_BACKEND_THREAD_SLEEP_DURATION_NS 500u
