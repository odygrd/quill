#pragma once

/**
 * This file is optional and demonstrates how users can replace the library macros with their own.
 *
 * It becomes useful when opting for a single Logger object over several logger objects.
 *
 * However, using multiple logger objects provides greater flexibility in applications with multiple
 * components. With multiple loggers, it's easier to control the log level of each component
 * individually, whereas using a single logger, as shown in this example, may present limitations.
 */

/**
 * By defining QUILL_DISABLE_NON_PREFIXED_MACROS before including LogMacros, we disable the
 * default 'LOG_' and then create our own macros using the global logger.
 */
#define QUILL_DISABLE_NON_PREFIXED_MACROS

#include "quill/LogMacros.h"
#include "quill/Logger.h"

// The logger we defined in quill_wrapper.cpp
extern quill::Logger* global_logger_a;

// Define custom log macros using global_logger_a. Two examples are provided here for demonstration.
#define LOG_INFO(fmt, ...) QUILL_LOG_INFO(global_logger_a, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) QUILL_LOG_WARNING(global_logger_a, fmt, ##__VA_ARGS__)
// etc ..
