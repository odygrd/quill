#pragma once

/**
 * This file is optional and demonstrates how users can replace the library macros with their own.
 *
 * It can be useful when your wrapper library exposes one default logger and you want shorter
 * call sites without passing that logger explicitly every time.
 *
 * This is only one optional pattern. Multiple logger objects are often a better fit for
 * larger applications because they let different components use different names, sinks,
 * and runtime log levels.
 */

/**
 * By defining QUILL_DISABLE_NON_PREFIXED_MACROS before including LogMacros, we disable the
 * default 'LOG_' and then create our own macros using the global logger.
 */
#define QUILL_DISABLE_NON_PREFIXED_MACROS

#include "quill/LogMacros.h"
#include "quill/Logger.h"

// The logger exposed by the wrapper library example.
extern quill::Logger* global_logger_a;

// Define custom log macros using the wrapper's default logger. Two examples are provided here
// for demonstration.
#define LOG_INFO(fmt, ...) QUILL_LOG_INFO(global_logger_a, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) QUILL_LOG_WARNING(global_logger_a, fmt, ##__VA_ARGS__)
// etc ..
