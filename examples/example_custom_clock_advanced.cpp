#include "quill/Quill.h"
#include "quill/clock/TimestampClock.h"

/**
 * Advanced custom clock logging example
 * In this example each Logger is using it's own clock.
 * Note that since some messages will have the current timestamp and some in the past it is possible
 * to have messages in stdout that are not ordered.
 * However, it is possible to configure a different file handler to a different file per Logger to avoid that.
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
  // we will also change the log pattern in order to also see our custom timestamp's date
  // Get the stdout file handler
  std::shared_ptr<quill::Handler> stdout_handler = quill::stdout_handler();

  // Set a custom formatter for this handler
  stdout_handler->set_pattern(
    "%(ascii_time) [%(thread)] %(fileline:<28) %(level_name) %(logger_name:<16) - %(message)", // format
    "%Y-%m-%d %H:%M:%S.%Qms",  // timestamp format
    quill::Timezone::GmtTime); // timestamp's timezone

  // Register the handler as default
  quill::Config cfg;
  cfg.default_handlers.emplace_back(stdout_handler);

  // Apply the configuration
  quill::configure(cfg);

  // Start the logging backend thread
  quill::start();

  // get default logger - using rdtsc clock
  quill::Logger* logger = quill::get_logger();
  logger->set_log_level(quill::LogLevel::TraceL3);

  // create a logger that is using a custom clock
  std::shared_ptr<quill::Handler> stdout_handler1 = stdout_handler;
  CustomTimestamp ts1;
  quill::Logger* logger_ts1 = quill::create_logger(
    "logger_ts1", std::move(stdout_handler1), quill::TimestampClockType::Custom, std::addressof(ts1));
  logger_ts1->set_log_level(quill::LogLevel::TraceL3);
  ts1.set_timestamp(std::chrono::seconds{1655007309});

  CustomTimestamp ts2;
  quill::Logger* logger_ts2 = quill::create_logger(
    "logger_ts2", std::move(stdout_handler), quill::TimestampClockType::Custom, std::addressof(ts2));
  logger_ts2->set_log_level(quill::LogLevel::TraceL3);
  ts2.set_timestamp(std::chrono::seconds{1685007309});

  LOG_TRACE_L3(logger, "This message is using rdtsc clock {}", 1);
  LOG_TRACE_L2(logger, "This message is using rdtsc clock {} {}", 2, 2.3);

  LOG_TRACE_L3(logger_ts1, "This message is using ts1 clock {}", 1);
  LOG_TRACE_L2(logger_ts1, "This message is using ts1 clock {} {}", 2, 2.3);

  LOG_TRACE_L3(logger_ts2, "This message is using ts2 clock {}", 1);
  LOG_TRACE_L2(logger_ts2, "This message is using ts2 clock {} {}", 2, 2.3);

  // quill:flush() blocks the caller and flashes all pending logs using the default loggers rdtsc clock
  quill::flush();
}