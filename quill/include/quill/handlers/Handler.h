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
   * Constructor
   * Uses a custom formatter
   * @tparam TConstantString
   * @param format_pattern
   */
  template <typename TConstantString>
  explicit Handler(TConstantString format_pattern) : _formatter(format_pattern){};

  /**
   * Copy Constructor
   */
  Handler(Handler const& other) = default;

  /**
   * Move Constructor
   */
  Handler(Handler&& other) noexcept = default;

  /**
   * Copy Assignment
   * @return
   */
  Handler& operator=(Handler const& other) = default;

  /**
   * Move Assignment
   * @return
   */
  Handler& operator=(Handler&& other) noexcept = default;

  /**
   * Destructor
   */
  virtual ~Handler() = default;

  /**
   * Set a custom formatter for this handler
   * @param formatter
   */
  void set_formatter(PatternFormatter formatter) { _formatter = std::move(formatter); }

  /**
   * Returns the owned formatter by the handler
   * @note: Accessor for backend processing
   * @return
   */
  PatternFormatter const& formatter() { return _formatter; }

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
  PatternFormatter _formatter; /**< Owned formatter for this handler */
};

} // namespace quill