#pragma once

#include "fmt/format.h"
#include "quill/PatternFormatter.h"

namespace quill
{

/**
 * Base class for handlers
 */
class Handler
{
public:
  /**
   * Constructor
   * Uses the default pattern formatter
   */
  Handler() = default;

  /**
   * Destructor
   */
  virtual ~Handler() = default;

  Handler(Handler const&) = delete;
  Handler& operator=(Handler const&) = delete;

  /**
   * Set a custom formatter for this handler
   * @param formatter
   */
  template <typename TConstantString>
  QUILL_ATTRIBUTE_COLD void set_pattern(TConstantString format_pattern,
                                        std::string date_format = std::string{"%H:%M:%S"},
                                        PatternFormatter::TimestampPrecision timestamp_precision =
                                          PatternFormatter::TimestampPrecision::NanoSeconds)
  {
    _formatter = std::make_unique<PatternFormatter>(format_pattern, date_format, timestamp_precision);
  }

  /**
   * Returns the owned formatter by the handler
   * @note: Accessor for backend processing
   * @return
   */
  PatternFormatter const& formatter() { return *(_formatter.get()); }

  /**
   * Logs a formatted log record to the handler
   * @note: Accessor for backend processing
   * @param formatted_log_record
   */
  virtual void emit(fmt::memory_buffer const& formatted_log_record) = 0;

  /**
   * Flush the handler synchronising the associated handler with its controlled output sequence.
   */
  virtual void flush() = 0;

private:
  /**< Owned formatter for this handler, we have to use a pointer here since the PatterFormatter
   * must not be moved or copied. We create the default pattern formatter always on init */
  std::unique_ptr<PatternFormatter> _formatter{std::make_unique<PatternFormatter>()};
};

} // namespace quill