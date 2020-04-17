/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/LogManager.h"

namespace quill
{
namespace detail
{
/**
 * A wrapper class around LogManager to make LogManager act as a singleton.
 * In fact LogManager is always a singleton as every access is provided via this class but this
 * gives us the possibility have multiple unit tests for LogManager as it would be harder to test
 * a singleton class
 */
class LogManagerSingleton
{
public:
  /**
   * Access to singleton instance
   * @return a reference to the singleton
   */
  static LogManagerSingleton& instance() noexcept;

  /**
   * Access to LogManager
   * @return a reference to the log manager
   */
  detail::LogManager& log_manager() noexcept;

private:
  LogManagerSingleton() = default;
  ~LogManagerSingleton();

private:
  detail::LogManager _log_manager;
};
} // namespace detail
} // namespace quill