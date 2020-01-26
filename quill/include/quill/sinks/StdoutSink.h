#include "quill/sinks/SinkBase.h"

namespace quill
{
/**
 * Simple stdout sink
 */
class StdoutSink : public SinkBase
{
public:
  /**
   * Using SinkBase constructors
   */
  using SinkBase::SinkBase;

  [[nodiscard]] StdoutSink* clone() const override;

  /**
   * Destructor
   * @param formatted_line
   */
  ~StdoutSink() override = default;

  /**
   * Log a formatted log record to the sink
   * @param formatted_log_record
   */
  void log(fmt::memory_buffer const& formatted_log_record) override;

  /**
   * Flushes to stdout
   */
  void flush() override;
};

} // namespace quill