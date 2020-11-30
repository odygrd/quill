/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <csignal>
#include <cstdint>
#include <initializer_list>

namespace quill
{
namespace detail
{
/**
 * Setup a signal handler to handle fatal signals
 */
#if defined(_WIN32)
void init_exception_handler();
#endif

/**
 * Linux/Windows.
 * On windows it has to be called on each thread
 * @param catchable_signals the signals we are catching
 */
void init_signal_handler(std::initializer_list<int32_t> const& catchable_signals = {
                           SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV});

} // namespace detail
} // namespace quill