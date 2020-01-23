#pragma once

#include "fmt/format.h"
#include "quill/detail/LogLineInfo.h"
#include "quill/detail/PatternFormatter.h"
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
   * Uses the default pattern formatter
   */
  SinkBase() = default;

  /**
   * Constructor
   * Uses a custom formatter
   * @tparam TConstantString
   * @param format_pattern
   */
  template <typename TConstantString>
  explicit SinkBase(TConstantString format_pattern) : _formatter(format_pattern){};

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
   * @note: Accessor for backend processing
   * @return
   */
  detail::PatternFormatter& formatter() { return _formatter; }

  /**
   * Logs the formatted message to the sink
   * @note: Accessor for backend processing
   * @param formatted_line
   */
  virtual void log(fmt::memory_buffer const& formatted_line) = 0;

private:
  detail::PatternFormatter _formatter; /**< Owned formatter for this sink */
};
} // namespace quill::detail