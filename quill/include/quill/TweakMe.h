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
 * StaticLogRecordInfo constexpr instances created in compile time
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
