#pragma once

#include <memory>
#include <string>
#include <vector>

#include "quill/handlers/Handler.h"

namespace quill
{
namespace detail
{
/**
 * The logger object is broken down to LoggerDetails and Logger as we end up in circular include
 * references if we include both in LogRecord
 *
 * Logger includes LogRecord as it needs it to create it, and LogRecord needs to read the
 * LoggerDetails later during the backend thread processing, but we don't want to include Logger
 */
class LoggerDetails
{
public:
  /**
   * Constructor for a single handle
   * @param name
   */
  LoggerDetails(std::string name, Handler* handler) : _name(std::move(name))
  {
    _handlers.push_back(handler);
  }

  /**
   * Constructor for multiple handlers
   * @param name
   */
  LoggerDetails(std::string name, std::vector<Handler*> handlers)
    : _name(std::move(name)), _handlers(std::move(handlers))
  {
  }

  /**
   * Deleted
   */
  LoggerDetails(LoggerDetails const&) = delete;
  LoggerDetails& operator=(LoggerDetails const&) = delete;

  /**
   * Destructor
   */
  ~LoggerDetails() = default;

  /**
   * @return The name of the logger
   */
  [[nodiscard]] std::string const& name() const noexcept { return _name; }

  /**
   * @return a vector of all handlers of this logger, called by the backend worker thread
   */
  [[nodiscard]] std::vector<Handler*> const& handlers() const noexcept { return _handlers; }

private:
  std::string _name;
  std::vector<Handler*> _handlers;
};
} // namespace detail
} // namespace quill