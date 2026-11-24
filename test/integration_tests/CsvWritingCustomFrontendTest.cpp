#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/CsvWriter.h"
#include "quill/core/FrontendOptions.h"
#include "quill/sinks/NullSink.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

using namespace quill;

struct OrderCsvSchema
{
  static constexpr char const* header = "order_id,symbol,quantity,price,side";
  static constexpr char const* format = "{},{},{},{:.2f},{}";
};

#define QUILL_TEST_REPEAT_16(text)                                                                 \
  text text text text text text text text text text text text text text text text

struct WideCsvSchema
{
  // 256 columns, which is more bytes than the queue of CustomFrontendOptions holds
  static constexpr char const* header =
    QUILL_TEST_REPEAT_16(QUILL_TEST_REPEAT_16("a_long_column_name,"));
  static constexpr char const* format = "{}";
};

// Define custom Frontend Options
struct CustomFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedBlocking;
  static constexpr size_t initial_queue_capacity = 4096;
};

// A writer at namespace scope is constructed before the static members of the CsvWriter template
quill::CsvWriter<OrderCsvSchema, CustomFrontendOptions> namespace_scope_csv_writer{
  "csv_writing_custom_frontend_namespace_scope", std::make_shared<quill::NullSink>(), false};

/***/
TEST_CASE("csv_writing_custom_frontend")
{
  using CsvFrontend = FrontendImpl<CustomFrontendOptions>;
  static constexpr char const* filename = "orders_custom_frontend.csv";

  // The writer appends by default, so the file of a run that did not finish is removed first
  testing::remove_file(filename);

  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // The logger of every writer has the prefix that the signal handler excludes by default
  REQUIRE_EQ(CsvFrontend::get_all_loggers().size(), 1);
  REQUIRE_EQ(CsvFrontend::get_all_loggers().front()->get_logger_name().rfind("__csv__", 0), 0u);
  namespace_scope_csv_writer.close();
  REQUIRE(CsvFrontend::get_all_loggers().empty());

#if !defined(QUILL_NO_EXCEPTIONS)
  {
    // The header does not fit in the queue. The writer is not constructed, and leaves no logger
    using WideCsvWriter = quill::CsvWriter<WideCsvSchema, CustomFrontendOptions>;
    REQUIRE_THROWS_AS(
      WideCsvWriter("csv_writing_custom_frontend_wide", std::make_shared<quill::NullSink>()), QuillError);
    REQUIRE(CsvFrontend::get_all_loggers().empty());
  }
#endif

  {
    quill::CsvWriter<OrderCsvSchema, CustomFrontendOptions> csv_writter{filename};

    // CSV rows honor the same per-logger flush policy as regular log messages.
    auto const loggers = CsvFrontend::get_all_loggers();
    REQUIRE_EQ(loggers.size(), 1);
    loggers.front()->set_immediate_flush(1);

    csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
    csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
    csv_writter.append_row(14212123, "AAPL", 120, 210.42321, "BUY");
    csv_writter.close();
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 4);

  REQUIRE(quill::testing::file_contains(file_contents, "order_id,symbol,quantity,price,side"));
  REQUIRE(quill::testing::file_contains(file_contents, "13212123,AAPL,100,210.32,BUY"));
  REQUIRE(quill::testing::file_contains(file_contents, "132121123,META,300,478.32,SELL"));
  REQUIRE(quill::testing::file_contains(file_contents, "14212123,AAPL,120,210.42,BUY"));

  testing::remove_file(filename);
}
