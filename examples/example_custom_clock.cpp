#include "quill/Quill.h"
#include "quill/clock/TimestampClock.h"

/**
 * Custom clock logging example
 */

/**
 * Custom timestamp class
 */
class CustomTimestamp : public quill::TimestampClock
{
public:
  CustomTimestamp() = default;

  /**
   * Required by TimestampClock
   * @return current time now, in nanoseconds since epoch
   */
  uint64_t now() const override { return _ts.load(std::memory_order_relaxed); }

  /**
   * set custom timestamp
   * @param time_since_epoch
   */
  void set_timestamp(std::chrono::seconds time_since_epoch)
  {
    // always convert to nanos
    _ts.store(std::chrono::nanoseconds{time_since_epoch}.count(), std::memory_order_relaxed);
  }

private:
  /**
   * time since epoch - must always be in nanoseconds
   * This class needs to be thread-safe, unless only a single thread in the application calling LOG macros
   * **/
  std::atomic<uint64_t> _ts;
};

int main()
{
  // create a custom ts class
  CustomTimestamp custom_ts;

  // we will also change the log pattern in order to also see our custom timestamp's date
  // Get the stdout file handler
  std::shared_ptr<quill::Handler> file_handler = quill::stdout_handler();

  // Set a custom formatter for this handler
  file_handler->set_pattern(
    "%(ascii_time) [%(thread)] %(fileline:<28) %(level_name) %(logger_name:<16) - %(message)", // format
    "%Y-%m-%d %H:%M:%S.%Qms",  // timestamp format
    quill::Timezone::GmtTime); // timestamp's timezone

  // Config using the custom ts class and the stdout handler
  quill::Config cfg;
  cfg.default_handlers.emplace_back(file_handler);
  cfg.default_custom_timestamp_clock = std::addressof(custom_ts);
  quill::configure(cfg);

  // Start the logging backend thread
  quill::start();

  // get default logger
  quill::Logger* logger = quill::get_logger();

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  // Set our timestamp to Sunday 12 June 2022
  custom_ts.set_timestamp(std::chrono::seconds{1655007309});
  LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);

  // update our timestamp
  custom_ts.set_timestamp(std::chrono::seconds{1655039000});
  LOG_TRACE_L1(logger, "This is a log trace l1 {} example", "string");
  LOG_DEBUG(logger, "This is a log debug example {}", 4);

  // Any new logger will also use the custom timestamp class
  quill::Logger* another_logger = quill::create_logger("another_logger");
  LOG_INFO(another_logger, "This is a log info example {}", 5);
  LOG_WARNING(another_logger, "This is a log warning example {}", 6);

  // quill:flush() block the caller and flashes all pending logs up to CustomTimestamp::now()
  // Here we will increment our timestamp before calling flush() to ensure all previous messages are logged
  custom_ts.set_timestamp(std::chrono::seconds{1655039001});
  quill::flush();
}