#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/BinaryWriter.h"
#include "quill/CsvWriter.h"
#include "quill/Frontend.h"
#include "quill/backend/SignalHandler.h"
#include "quill/core/FrontendOptions.h"

#include <chrono>
#include <csignal>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

struct SignalHandlerWriterLoggersCsvSchema
{
  static constexpr char const* header = "order_id,symbol";
  static constexpr char const* format = "{},{}";
};

/***/
TEST_CASE("signal_handler_writer_loggers")
{
  static constexpr char const* csv_filename = "signal_handler_writer_loggers.csv";
  static constexpr char const* binary_filename = "signal_handler_writer_loggers.bin";

  testing::remove_file(csv_filename);
  testing::remove_file(binary_filename);

  // The backend flushes the sinks once and then not again for an hour, so the records that are
  // in the files after the signal were flushed by its handler
  BackendOptions backend_options;
  backend_options.sink_min_flush_interval = std::chrono::hours{1};

  SignalHandlerOptions signal_handler_options{};
  signal_handler_options.catchable_signals = std::vector<int>{SIGABRT};
  signal_handler_options.timeout_seconds = 40;
  Backend::start<FrontendOptions>(backend_options, signal_handler_options);

  // For testing purposes we want to keep the application running, we do not reraise the signal
  detail::SignalHandlerContext::instance().should_reraise_signal.store(false);

#if defined(_WIN32)
  // NOTE: On windows the signal handler must be installed on each new thread
  quill::init_signal_handler<quill::FrontendOptions>();
#endif

  Frontend::preallocate();

  // The application has the loggers of writers only. The signal handler does not log its
  // messages through them, which would end up in their files
  CsvWriter<SignalHandlerWriterLoggersCsvSchema, FrontendOptions> csv_writer{csv_filename};
  BinaryWriter binary_writer{binary_filename};

  // Give the backend the time for its one flush, before there are records
  std::this_thread::sleep_for(std::chrono::milliseconds{300});

  std::string const record{"\x01\x00\n\xff", 4};
  csv_writer.append_row(13212123, "AAPL");
  REQUIRE(binary_writer.write(record.data(), record.size()));

  // Now raise a signal. Its handler still flushes the files of the writers
  std::raise(SIGABRT);

  {
    std::vector<std::string> const csv_file_contents = testing::file_contents(csv_filename);
    REQUIRE_EQ(csv_file_contents.size(), 2);
    REQUIRE_EQ(csv_file_contents[0], "order_id,symbol");
    REQUIRE_EQ(csv_file_contents[1], "13212123,AAPL");

    std::ifstream binary_file{binary_filename, std::ios::binary};
    std::string const binary_file_contents{std::istreambuf_iterator<char>{binary_file},
                                           std::istreambuf_iterator<char>{}};
    REQUIRE(binary_file_contents == record);
  }

  csv_writer.close();
  binary_writer.close();

  Backend::stop();
  REQUIRE_FALSE(Backend::is_running());

  testing::remove_file(csv_filename);
  testing::remove_file(binary_filename);
}
