#pragma once

#include <memory>
#include <string>
#include <vector>

#include "quill/sinks/SinkBase.h"

namespace quill::detail
{
/**
 * The logger object is broken down to LoggerDetails and Logger as we end up in circular include
 * references if we include both in LogMessage
 */
class LoggerDetails
{
public:
  /**
   * Constructor
   * @param name
   */
  LoggerDetails(std::string name, std::unique_ptr<SinkBase> sink) : _name(std::move(name))
  {
    _sinks.push_back(std::move(sink));
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
   * @return a vector of all sinks of this logger, called by the logging thread
   */
  [[nodiscard]] std::vector<std::unique_ptr<SinkBase>> const& sinks() const noexcept
  {
    return _sinks;
  }

private:
  std::string _name;
  std::vector<std::unique_ptr<SinkBase>> _sinks;
};
} // namespace quill::detail