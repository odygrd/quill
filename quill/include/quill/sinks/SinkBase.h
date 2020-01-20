#pragma once

#include "fmt/format.h"
#include "quill/detail/Formatter.h"
#include "quill/detail/LogLineInfo.h"
#include "quill/detail/message/MessageBase.h"

namespace quill::detail
{

/**
 * Base class for logging sinks
 */
class SinkBase
{
public:
  /**
   * Constructor
   */
  SinkBase() = default;

  /**
   * Deleted
   */
  SinkBase(SinkBase const&) = delete;
  SinkBase& operator=(SinkBase const&) = delete;

  /**
   * Destructor
   */
  virtual ~SinkBase() = default;

  /**
   * Returns the owned formatter by the sink
   * @return
   */
  detail::Formatter const& formatter() { return _formatter; }

  /**
   * Logs the formatted message to the sink
   * @param formatted_line
   */
  virtual void log(fmt::memory_buffer const& formatted_line) = 0;

private:
  detail::Formatter _formatter; /**< Owned formatter of for this sink */
};
} // namespace quill::detail