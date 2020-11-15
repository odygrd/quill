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
 * This function will be called in case of a fatal signal
 * @param signal_number signal id
 */
void on_signal(int32_t signal_number);

/**
 * Setup a signal handler to handle fatal signals
 */
void init_signal_handler(std::initializer_list<int32_t> const& catchable_signals = {
                           SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV});
} // namespace detail
} // namespace quill