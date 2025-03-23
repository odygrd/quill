#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/CsvWriter.h"
#include "quill/core/FrontendOptions.h"

using namespace quill;

struct OrderCsvSchema
{
  static constexpr char const* header = "order_id,symbol,quantity,price,side";
  static constexpr char const* format = "{},{},{},{:.2f},{}";
};

// Define custom Frontend Options
struct CustomFrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedBlocking;
  static constexpr size_t initial_queue_capacity = 131'072;
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
  static constexpr size_t unbounded_queue_max_capacity = 2ull * 1024u * 1024u * 1024u;
  static constexpr quill::HugePagesPolicy huge_pages_policy = quill::HugePagesPolicy::Never;
};

/***/
TEST_CASE("csv_writing_custom_frontend")
{
  static constexpr char const* filename = "orders_custom_frontend.csv";

  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  {
    quill::CsvWriter<OrderCsvSchema, CustomFrontendOptions> csv_writter{filename};
    csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
    csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
    csv_writter.append_row(14212123, "AAPL", 120, 210.42321, "BUY");
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
