#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace quill;

struct CapturedBinaryRecord
{
  uint64_t timestamp{0};
  std::string thread_id;
  std::string process_id;
  std::string logger_name;
  MacroMetadata::Event event{MacroMetadata::Event::None};
  std::string file_name;
  std::string line;
};

/** Keeps the log and binary records in the order the backend writes them */
struct BinaryCapturingSink final : public Sink
{
  void write_log(LogRecord const& record) override
  {
    events.push_back("log:" + std::string{record.log_message});
  }

  void write_binary(BinaryRecord const& record) override
  {
    events.push_back("binary:" + std::string{reinterpret_cast<char const*>(record.data), record.size});
    binary_records.push_back(CapturedBinaryRecord{
      record.timestamp, std::string{record.thread_id}, std::string{record.process_id},
      std::string{record.logger_name}, record.macro_metadata->event(),
      std::string{record.macro_metadata->file_name()}, std::string{record.macro_metadata->line()}});
  }

  void flush_sink(SinkFlushReason) override {}

  std::vector<std::string> events;
  std::vector<CapturedBinaryRecord> binary_records;
};

/** A sink that writes text, as it does not implement write_binary() */
struct TextOnlySink final : public Sink
{
  void write_log(LogRecord const& /* record */) override { ++log_writes; }

  void flush_sink(SinkFlushReason) override {}

  size_t log_writes{0};
};

struct ThrowingBinarySink final : public Sink
{
  void write_log(LogRecord const& /* record */) override {}

  void write_binary(BinaryRecord const& /* record */) override
  {
#if !defined(QUILL_NO_EXCEPTIONS)
    throw std::runtime_error{"simulated binary sink failure"};
#endif
  }

  void flush_sink(SinkFlushReason) override {}
};

/***/
TEST_CASE("log_binary_sink")
{
  static std::string const logger_name = "log_binary_sink_logger";

  std::atomic<size_t> error_notifications{0};
  BackendOptions backend_options;
  backend_options.error_notifier = [&error_notifications](std::string const& message)
  {
    if (message.find("Allocated a new SPSC queue") == std::string::npos)
    {
      error_notifications.fetch_add(1, std::memory_order_relaxed);
    }
  };

  Backend::start(backend_options);

  auto throwing_sink = Frontend::create_or_get_sink<ThrowingBinarySink>("log_binary_sink_throwing");
  auto text_only_sink = Frontend::create_or_get_sink<TextOnlySink>("log_binary_sink_text_only");
  auto capturing_sink =
    Frontend::create_or_get_sink<BinaryCapturingSink>("log_binary_sink_capturing");

  // The log level filter of a sink applies to its log statements, not to its binary records
  capturing_sink->set_log_level_filter(LogLevel::Warning);

  Logger* logger = Frontend::create_or_get_logger(
    logger_name, std::initializer_list<std::shared_ptr<Sink>>{throwing_sink, text_only_sink, capturing_sink});

  // The bytes are copied, so they can change after the call
  std::string bytes{"\x01\n\x02", 3};

  LOG_WARNING(logger, "first");
  int const first_binary_line = __LINE__ + 1;
  LOG_BINARY(logger, bytes.data(), bytes.size());
  bytes.assign("changed");
  LOG_INFO(logger, "below the level of the capturing sink");
  LOG_BINARY(logger, bytes.data(), bytes.size());

  // Nor does the log level of the logger apply to them
  logger->set_log_level(LogLevel::Critical);
  LOG_WARNING(logger, "below the level of the logger");

  // Logger::log_binary() takes the metadata that the macro defines for its call site
  static constexpr MacroMetadata macro_metadata{
    "feed_handler.cpp:42", "on_packet", "", nullptr, LogLevel::None, MacroMetadata::Event::Binary};

  REQUIRE(logger->log_binary(&macro_metadata, "last", 4));

  // A header and data are one record: the bytes of the header followed by the data
  std::string const header{"\x00\x10", 2};
  REQUIRE(logger->log_binary(&macro_metadata, header.data(), header.size(), "payload", 7));
  REQUIRE(logger->log_binary(&macro_metadata, nullptr, 0, "data only", 9));
  REQUIRE(logger->log_binary(&macro_metadata, header.data(), header.size(), nullptr, 0));

  // The limit leaves room for what is queued with the bytes, also where size_t has 32 bits: a
  // record of 4 GiB less two bytes would wrap the size of its reservation around to 34 bytes
  static_assert(detail::max_binary_record_size(UINT64_MAX, 32) == (UINT32_MAX - 1u),
                "On a 64-bit platform the limit is the length that 32 bits can encode");
  static_assert(detail::max_binary_record_size(UINT32_MAX, 32) == (UINT32_MAX - 36u),
                "On a 32-bit platform the limit leaves room for the queue header and the length");
  static_assert((Logger::max_binary_record_size + 36u) > Logger::max_binary_record_size,
                "The queue space of the largest record does not overflow");

#if !defined(QUILL_NO_EXCEPTIONS)
  // The length of a record is encoded in 32 bits. A larger one is rejected, not cut short
  size_t const too_large = Logger::max_binary_record_size + 1u;
  REQUIRE_THROWS_AS(logger->log_binary(&macro_metadata, "x", too_large), QuillError);
  REQUIRE_THROWS_AS(logger->log_binary(&macro_metadata, header.data(), header.size(), "x",
                                       Logger::max_binary_record_size - 1u),
                    QuillError);
#endif

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  auto const* capturing = static_cast<BinaryCapturingSink const*>(capturing_sink.get());
  auto const* text_only = static_cast<TextOnlySink const*>(text_only_sink.get());

  // Each sink gets every binary record, in timestamp order with its log statements, also after
  // another sink has thrown
  REQUIRE_EQ(capturing->events.size(), 7);
  REQUIRE_EQ(capturing->events[0], "log:first");
  REQUIRE_EQ(capturing->events[1], std::string{"binary:\x01\n\x02", 10});
  REQUIRE_EQ(capturing->events[2], "binary:changed");
  REQUIRE_EQ(capturing->events[3], "binary:last");
  REQUIRE_EQ(capturing->events[4], std::string{"binary:\x00\x10payload", 16});
  REQUIRE_EQ(capturing->events[5], "binary:data only");
  REQUIRE_EQ(capturing->events[6], std::string{"binary:\x00\x10", 9});

  REQUIRE_EQ(text_only->log_writes, 2);

#if !defined(QUILL_NO_EXCEPTIONS)
  // The error of the throwing sink is the same for each record, so it is reported once
  REQUIRE_EQ(error_notifications.load(std::memory_order_relaxed), 1);
#else
  REQUIRE_EQ(error_notifications.load(std::memory_order_relaxed), 0);
#endif

  REQUIRE_EQ(capturing->binary_records.size(), 6);

  uint64_t previous_timestamp{0};

  for (CapturedBinaryRecord const& binary_record : capturing->binary_records)
  {
    REQUIRE_GT(binary_record.timestamp, 0);
    REQUIRE_GE(binary_record.timestamp, previous_timestamp);
    REQUIRE_FALSE(binary_record.thread_id.empty());
    REQUIRE_FALSE(binary_record.process_id.empty());
    REQUIRE_EQ(binary_record.logger_name, logger_name);
    REQUIRE(binary_record.event == MacroMetadata::Event::Binary);

    previous_timestamp = binary_record.timestamp;
  }

  // The macro passes the metadata of its call site, as the macros of the log statements do
#if defined(QUILL_DISABLE_FILE_INFO)
  // The source location is compiled out
  (void)first_binary_line;
  REQUIRE(capturing->binary_records[0].file_name.empty());
  REQUIRE(capturing->binary_records[1].file_name.empty());
#else
  REQUIRE_EQ(capturing->binary_records[0].file_name, "LogBinarySinkTest.cpp");
  REQUIRE_EQ(capturing->binary_records[0].line, std::to_string(first_binary_line));
  REQUIRE_EQ(capturing->binary_records[1].file_name, "LogBinarySinkTest.cpp");
  REQUIRE_NE(capturing->binary_records[1].line, capturing->binary_records[0].line);
#endif

  REQUIRE_EQ(capturing->binary_records[2].file_name, "feed_handler.cpp");
  REQUIRE_EQ(capturing->binary_records[2].line, "42");
}
