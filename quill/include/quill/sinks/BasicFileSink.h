#include "quill/sinks/SinkBase.h"

namespace quill
{
/**
 * Simple stdout sink
 */
class BasicFileSink : public SinkBase
{
public:
  /**
   * Constructor with default formatter pattern
   * @param filename
   */
  explicit BasicFileSink(std::string const& filename) { _fopen(filename.data(), "w"); };

  /**
   * Constructor
   * Uses a custom formatter
   * @tparam TConstantString
   * @param format_pattern
   */
  template <typename TConstantString>
  BasicFileSink(std::string const& filename, TConstantString format_pattern)
    : SinkBase{format_pattern}
  {
    _fopen(filename.data(), "w");
  }

  [[nodiscard]] BasicFileSink* clone() const override;

  /**
   * Destructor
   * @param formatted_line
   */
  ~BasicFileSink() override { _fclose(); }

  /**
   * Log a formatted log record to the sink
   * @param formatted_log_record
   */
  void log(fmt::memory_buffer const& formatted_log_record) override;

  /**
   * Flushes to stdout
   */
  void flush() override;

private:
  /**
   * Open a file
   * @param filename
   * @param mode
   * @return A pointer to the opened file
   * @throws
   */
  void _fopen(char const* filename, char const* mode);

  /**
   * Close a file
   */
  void _fclose();

private:
  FILE* _fd{nullptr};
};

} // namespace quill