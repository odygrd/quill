/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

/**
 * Edit this file to optimize performance or customize compile-time enabled/disabled features.
 *
 * All the values below can be passed as compiler flags or uncommented here.
 *
 * HINT:
 * It is better to pass these flags using CMake or during compilation for flexibility.
 * Invoking different builds with different flags is preferable to editing this file directly.
 *
 * a) When invoking cmake: cmake . -DCMAKE_CXX_FLAGS="-DQUILL_ACTIVE_LOG_LEVEL=QUILL_LOG_LEVEL_INFO"
 */

/**
 * Completely compiles out log levels with zero cost.
 *
 * Macros like LOG_TRACE_L3(..), LOG_TRACE_L2(..) will expand to empty statements,
 * reducing branches in compiled code and the number of MacroMetadata constexpr instances.
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
 * Quill defers formatting to the backend thread. Objects are copied to a queue and formatted
 * by the backend thread using operator<<.
 *
 * When a user-defined type has mutable references, this can be problematic if the object
 * is modified by the caller thread during formatting on the backend logger thread.
 *
 * By default, Quill checks at compile time if the object is safe to copy. User-defined types
 * can provide a tag or an external template specialization:
 *
 * 1) Tagging on your class:
 *
 *   'using copy_loggable = std::true_type` -> Indicates the object is copyable.
 *   'using copy_loggable = std::false` or missing tag -> Indicates the object is NOT copyable.
 *
 * 2) Preferred way: provide a specialization to copy_loggable without modifying your class:
 *
 *   namespace quill {
 *     template <>
 *     struct copy_loggable<UserDefinedType> : std::true_type { };
 *   }
 *
 * If the user-defined type has shared mutable references as class members, do not provide this.
 * Formatting should be done explicitly on the caller thread side by the user.
 *
 * MODE_UNSAFE is not recommended as it requires careful handling when logging user-defined types.
 * When MODE_UNSAFE is enabled, Quill copies any copy-constructible user-defined types to the queue
 * without checks. Tagging or specialization is not required.
 */
// #define QUILL_MODE_UNSAFE

/**
 * When QUILL_DISABLE_NON_PREFIXED_MACROS is defined, it removes `LOG_*` macros,
 * and only `QUILL_LOG_*` macros are available.
 * This is useful if the original macro names collide with an existing logging library.
 */
// #define QUILL_DISABLE_NON_PREFIXED_MACROS

/**
 * When the application doesn't require multiple Logger objects and the root logger is enough,
 * define QUILL_ROOT_LOGGER_ONLY to log without passing any Logger* to the macro.
 * Example: LOG_INFO("Hello {}", "world");
 */
// #define QUILL_ROOT_LOGGER_ONLY

/**
 * When QUILL_USE_BOUNDED_QUEUE is defined, a bounded queue is used instead of the default unbounded queue.
 *
 * The default mode is unbounded queue, which is recommended. The overhead is low.
 * Use QUILL_USE_BOUNDED_QUEUE when all re-allocations should be avoided.
 * In QUILL_USE_BOUNDED_QUEUE mode, the number of dropped log messages is written to stderr.
 *
 * @note: In both modes (unbounded or bounded) the queue size is configurable via
 * `quill::config::set_initial_queue_capacity`.
 *
 * @note: You can avoid re-allocations when using the unbounded queue (default mode) by setting the
 * initial_queue_capacity to a higher value.
 * QUILL_USE_BOUNDED_QUEUE mode seems to be faster in `quill_hot_path_rdtsc_clock` benchmark by a few nanoseconds.
 *
 * For CMake:
 *   -DCMAKE_CXX_FLAGS:STRING="-DQUILL_USE_BOUNDED_QUEUE"
 *
 * @see: examples/example_bounded_queue_message_dropping.cpp
 */
// #define QUILL_USE_BOUNDED_QUEUE

/**
 * Similar to QUILL_USE_BOUNDED_QUEUE but the hot thread will now block instead of dropping messages.
 * @note: This will slow down you hot thread and it should only be used in special cases
 *
 * For CMake:
 *   -DCMAKE_CXX_FLAGS:STRING="-DQUILL_USE_BOUNDED_BLOCKING_QUEUE"
 *
 * @see: examples/example_bounded_queue_blocking.cpp
 */
// #define QUILL_USE_BOUNDED_BLOCKING_QUEUE

/**
 * The default unbounded queue stops re-allocating and blocks when it reaches the maximum capacity
 * (2GB by default). If you have multiple hot threads, this can lead to memory limits.
 *
 * When QUILL_USE_UNBOUNDED_NO_MAX_LIMIT_QUEUE is defined, additional 2GB queues are allocated,
 * and the hot thread will never block or drop messages.
 *
 * For CMake:
 *   -DCMAKE_CXX_FLAGS:STRING="-DQUILL_USE_UNBOUNDED_NO_MAX_LIMIT_QUEUE"
 */
// #define QUILL_USE_UNBOUNDED_NO_MAX_LIMIT_QUEUE

/**
 * Similar to QUILL_USE_UNBOUNDED_NO_MAX_LIMIT_QUEUE, but the hot thread will drop messages instead
 * of allocating additional 2GB queues when the maximum capacity of the unbounded queue is reached.
 *
 * For CMake:
 *   -DCMAKE_CXX_FLAGS:STRING="-DQUILL_USE_UNBOUNDED_DROPPING_QUEUE"
 */
// #define QUILL_USE_UNBOUNDED_DROPPING_QUEUE

/**
 * Similar to QUILL_USE_UNBOUNDED_NO_MAX_LIMIT_QUEUE, but the hot thread will block instead
 * of allocating additional 2GB queues when the maximum capacity of the unbounded queue is reached.
 * This is the default behavior.
 */
// #define QUILL_USE_UNBOUNDED_BLOCKING_QUEUE

/**
 * Applies to bounded/unbounded blocking queues. When the queue is full, the active thread
 * will sleep for a brief period and retry. The default value is 800 ns. Set to 0 to disable.
 *
 * For CMake:
 *   -DCMAKE_CXX_FLAGS:INT="-DBLOCKING_QUEUE_RETRY_INTERVAL_NS=1000"
 */
// #define QUILL_BLOCKING_QUEUE_RETRY_INTERVAL_NS 800

/**
 * Enables the use of _mm_prefetch, _mm_clflush, and _mm_clflushopt on the ring buffer to improve
 * performance on x86 architectures.
 *
 * When this option is enabled, ensure to pass the appropriate target architecture to the compiler
 * with -march="..."
 *
 * For CMake:
 *   -DCMAKE_CXX_FLAGS:STRING="-DQUILL_X86ARCH -march=native"
 * or
 *   target_compile_definitions(<target> PUBLIC -DQUILL_X86ARCH)
 */
// #define QUILL_X86ARCH

/**************************************************************************************************/
/* Anything after this point requires the whole library to be recompiled with the desired option. */
/**************************************************************************************************/

/**
 * Disables features not supported on Windows 2012/2016.
 * Also available as CMake option `-DQUILL_NO_THREAD_NAME_SUPPORT:BOOL=ON`
 */
// #define QUILL_NO_THREAD_NAME_SUPPORT

/**
 * Uses an installed version of the fmt library instead of Quill's bundled copy.
 * Quill will try to include <fmt/format.h>, so make sure to set -I directories
 * accordingly if not using CMake.
 *
 * Also available as CMake option `-DQUILL_FMT_EXTERNAL=ON`
 * When `-DQUILL_FMT_EXTERNAL=ON` is used, the below line does not need to be uncommented as CMake
 * will define it automatically.
 *
 * Quill will look for a CMake Target named `fmt`. If the target is not found, it will
 * use find_package(fmt REQUIRED), so make sure that the fmt library is installed on your system.
 */
// #define QUILL_FMT_EXTERNAL

/**
 * Disables all exceptions and replaces them with std::abort()
 * Also available as CMake option `-DQUILL_NO_EXCEPTIONS=ON`
 */
// #define QUILL_NO_EXCEPTIONS