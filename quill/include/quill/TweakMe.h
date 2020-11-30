/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

/**
 * Edit this file to squeeze more performance, or to customise supported
 * features that are enabled/disabled in compile time
 *
 * All the values below can either be passed as compiler flags or get uncommented here
 *
 * HINT:
 * It is better passing those flags using CMake or during compilation instead of uncommenting them
 * here as this gives more flexibility invoking different builds by passing different flags rather
 * than editing this file
 *
 * a) When invoking cmake : cmake . -DCMAKE_CXX_FLAGS="-DQUILL_ACTIVE_LOG_LEVEL=QUILL_LOG_LEVEL_INFO"
 */

/**
 * If QUILL_CHRONO_CLOCK value is defined, Quill will use chrono system_clock for timestamps.
 *
 * QUILL RDTSC CLOCK mode :
 *
 * TSC clock gives better performance on the caller thread. However, the initialisation time of the application is higher as
 * we have to take multiple samples in the beginning to convert TSC to nanoseconds
 *
 * Consider reading https://stackoverflow.com/questions/42189976/calculate-system-time-using-rdtsc
 *
 * The backend thread is constantly keeping track of the difference between TSC and the system wall clock resulting
 * in accurate timestamps.
 *
 * When using the TSC counter the backend thread will also periodically call chrono::system_clock:now() and will
 * resync the TSC based on the system clock.
 *
 * @note: This should be switchable even after quill is already installed as a static or shared library.
 *
 * Usage:
 * Run cmake as e.g: cmake . -DCMAKE_CXX_FLAGS="-DQUILL_CHRONO_CLOCK=1"
 * or
 * In the root CMake file use: `add_definitions(-DQUILL_CHRONO_CLOCK=1)`
 *
 * By default RDTSC clock is enabled
 */
// #define QUILL_CHRONO_CLOCK

/**
 * This option is only applicable if the RDTSC clock is enabled.
 * When QUILL_CHRONO_CLOCK is defined this option can be ignored
 *
 * This value controls how frequently the backend thread will re-calculate and sync the TSC by
 * getting the system time from the system wall clock.
 * The TSC clock drifts slightly over time and is also not synchronised with the NTP server updates
 * Therefore the smaller this value is the more accurate the log timestamps will be.
 *
 * It is not recommended to change the default value unless there is a real reason.
 * The value is in milliseconds and the default value is 700.
 *
 * @note: This should be switchable even after quill is already installed as a static or shared library.
 */
// #define QUILL_RDTSC_RESYNC_INTERVAL 700

/**
 * Completely compiles out log level with zero cost.
 *
 * Macros like LOG_TRACE_L3(..), LOG_TRACE_L2(..) will expand to empty statements
 * This helps reducing the number of branches in your compiled code and the number of
 * LogMacroMetadata constexpr instances created in compile time
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
 * Quill refers all formatting to the backend thread. That means that the objects get copied to
 * a queue and later on processed on the backend thread which calls operator<< to format them.
 *
 * In some cases when a user defined type has mutable references in the object this can be
 * problematic as they can be modified by the caller thread while they are trying to get
 * formatted on the backend logger thread.
 *
 * By default Quill checks in compile time if the object is safe to get copied. User defined
 * types can also provide a tag :
 *
 * 'using copy_loggable = std::true_type` -> This will indicate that the object is copyable.
 * 'using copy_loggable = std::false` or missing tag -> This will indicate that the object is NOT copyable.
 *
 * There is a convenience macro also `QUILL_COPY_LOGGABLE`
 *
 * If the user defined type has shared mutable references to other objects as class members
 * this tag should not be provided, instead the formatting should be done explicitly on the caller
 * thread side by the user.
 *
 * QUILL_MODE_UNSAFE is not recommended as the user should be never careful when logging user
 * defined types.
 * When QUILL_MODE_UNSAFE is enabled quill will copy any user defined types that are copy constructible
 * to the queue.
 * Tag `copy_loggable` is no longer applicable in QUILL_MODE_UNSAFE
 *
 * QUILL_MODE_UNSAFE needs to be defined before including Quill.h
 *
 * Usage:
 * Run cmake as e.g: cmake . -DCMAKE_CXX_FLAGS="-DQUILL_MODE_UNSAFE"
 * or
 * In the root CMake file use: `add_definitions(-DQUILL_MODE_UNSAFE)`
 */
// #define QUILL_MODE_UNSAFE

/**
 * This option is useful only for GCC versions < 9.0.
 * Quill requires __FUNCTION__ to be usable as a constant expression and in some cases in gcc prior
 * to 9.0 __FUNCTION__ is not usable as a constant expression (e.g. in nested lambdas)
 * In those cases the regular LOG_* macros won't compile.
 * see https://github.com/odygrd/quill/issues/28
 *     https://stackoverflow.com/questions/55850013/pretty-function-in-constant-expression
 *
 * When this option is enabled then some extra log macros get enabled that can be used in places
 * where __FUNCTION__ is not constexpr
 * All log macros will also be available in the form of LOG_LEVEL_NOFN(...)
 */
// #define QUILL_NOFN_MACROS

/**
 * When QUILL_DISABLE_NON_PREFIXED_MACROS is removes LOG_INFO macros and only QUILL_LOG_INFO is available.
 * This is only required in case the original macro names collide with another third party library
 */
// #define QUILL_DISABLE_NON_PREFIXED_MACROS

/**************************************************************************************************/
/* Anything after this point requires the whole library to be recompiled with the desired option. */
/**************************************************************************************************/

/**
 * Uses an installed version of the fmt library instead of quill's bundled copy.
 * In this case quill will try to include <fmt/format.h> so make sure to set -I directories
 * accordingly if not using CMake.
 *
 * This is also available as CMake option -DQUILL_FMT_EXTERNAL=ON.
 * When -DQUILL_FMT_EXTERNAL=ON is used the below line does not need to be uncommented as CMake will
 * define it automatically.
 * Quill will look for a CMake Target with name `fmt`. If the target is not found it will
 * use find_package(fmt REQUIRED) so make sure that fmt library is installed in your system
 */
// #define QUILL_FMT_EXTERNAL

/**
 * Disables all exceptions and replaces them with std::abort()
 */
// #define QUILL_NO_EXCEPTIONS

/**
 * When QUILL_USE_BOUNDED_QUEUE is defined a bounded queue for passing the log messages
 * to the backend thread, instead of the default unbounded queue is used.
 *
 * Bounded Queue : When full the log messages will get dropped.
 * Unbounded Queue : When full, a new queue is allocated and no log messages are lost.
 *
 * The default mode is unbounded queue as the recommended option. The overhead is low.
 * Using QUILL_USE_BOUNDED_QUEUE option is in the case when all re-allocations should be avoided.
 * In QUILL_USE_BOUNDED_QUEUE mode the number of dropped log messages is written to stderr.
 *
 * @note: In both modes (unbounded or bounded) the queue size is configurable via `quill::config::set_initial_queue_capacity`.
 *
 * @note: You can avoid re-allocations when using the unbounded queue (default mode) by setting the initial_queue_capacity to a higher value.
 * QUILL_USE_BOUNDED_QUEUE mode seems to be faster in `quill_hot_path_rdtsc_clock` benchmark by a few nanoseconds.
 */
// #define QUILL_USE_BOUNDED_QUEUE

/**
 * When this is enabled Quill will use 2 SPSC queues for each thread
 * 1) The first queue is the main queue called event queue. In this queue there are events pushed
 * from the caller threads to the backend worker thread. Example the flush event, log events etc..
 * The log events are pushed after all log statement arguments are converted to a tuple inside an
 * event. A log event can contain arguments of any type e.g. vectors, maps or user defined types.
 * The events of this queue are read by the backend worker thread via the help of virtual methods.
 * This queue is required for quill to worke and it is NOT possible to disable it.
 *
 * 2) The second queue is a fast raw spsc queue where only fundamental types are pused. In this queue
 * the arguments are inserted using memcpy() and it is faster than the first queue by a few nanoseconds
 * on the caller threads - on my system this ranges from around 2 to 10 nanoseconds on the caller thread.
 *
 * This queue also avoids the overhead of `new` when short string optimization is not enabled or
 * not supported. For example if we wanted to copy a std::string or a long char const* to the first
 * queue (event queue) it would be copied as std::tuple<std::string> which could result in
 * calling std::string's constructor for and operator `new` large strings.
 * Inserting the same string to this fast raw spsc queue we just do a memcpy of the string contents
 * to the queue resulting to no extra allocations.
 *
 * How the fast queue works :
 * When we push to the fast queue we store only the argument value and NOT their types.
 * In order to retrieve the types for deserialization we construct a SerializationMetadata string
 * DURING program initialization time. The SerializationMetadata have static lifetime.
 * To the fast queue we only push a pointer to this pre-constructed SerializationMetadata
 *
 * As a result using DUAL_QUEUE will result in doing some small initial calculations every time
 * the program starts.
 *
 * Each log statement is pushed either to the first OR to the second queue.
 *
 * When QUILL_DUAL_QUEUE_MODE is disabled everything is pushed into the first queue.
 *
 * When QUILL_DUAL_QUEUE_MODE is enabled any log statements that have all their arguments satisfying
 * the below criteria will get pushed to the second queue resulting in better performance and less allocations
 * a) fundamental types
 * b) enums
 * c) std::string and char arrays
 *
 * E.g.
 * LOG_INFO(logger, "{} {} {}", 1, 2.5, "test"); --> This is pushed to the second (raw) queue because ALL arguments satisfy the above criteria
 * LOG_INFO(logger, "{} {} {}", 1, "test", std::array<int,3>{1,2,3}); -> This is pushed to the first queue (EVENT) as it contains a complex type
 *
 * NOTE:
 * 1) Using QUILL_DUAL_QUEUE_MODE with unbounded queue is the recommended option.
 *  The difference of bounded and unbounded queues is very small in latency and the unbounded queue is
 *  safer as no log messages will get dropped.
 *
 * 2) Using QUILL_DUAL_QUEUE_MODE + QUILL_USE_BOUNDED_QUEUE will give the fastest performance possible.
 * Logging only fundamental types or strings (like a printf only API) in the hot path is also recommended if you care about every nanosecond of latency.
 *
 * 3) Disable QUILL_DUAL_QUEUE_MODE if
 * a) Don't care about around 2-10 extra nanoseconds of latency when logging just fundamental types
 * b) Mostly logging complex types, user defined types etc
 * c) A few extra ms during program initialization time are important
 */
#define QUILL_DUAL_QUEUE_MODE

/**
 * Quill uses a unbounded SPSC queue per spawned thread to forward the LogRecords to the backend thread.
 *
 * During very high logging activity the backend thread won't be able to consume fast enough
 * and the queue will become full. In this scenario the caller thread will not block but instead
 * it will allocate a new queue of the same capacity.
 *
 * If the backend thread is falling behind also consider reducing the sleep duration of the backend
 * thread first or pinning it to a dedicated core. This will keep the queue more empty.
 *
 * The queue size can be increased or decreased based on the user needs. This queue will be shared
 * between two threads and it should not exceed the size of LLC cache.
 *
 * When QUILL_DUAL_QUEUE_MODE is used this affects the size of both queues.
 *
 * @warning The configured queue size needs to be in bytes, it MUST be a power of two and a multiple
 * of the page size (4096).
 * Look for an online Mebibyte to Byte converted to easily find a correct value.
 */
// #define QUILL_QUEUE_CAPACITY 262'144