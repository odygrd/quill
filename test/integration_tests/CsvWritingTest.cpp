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

/***/
TEST_CASE("csv_writing")
{
  static constexpr char const* filename = "orders.csv";
  static constexpr char const* filename_1 = "orders_1.csv";
  static constexpr char const* filename_2 = "orders_rotating.csv";
  static constexpr char const* filename_2_1 = "orders_rotating.1.csv";
  static constexpr char const* filename_2_2 = "orders_rotating.2.csv";

  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  {
    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{filename};
    csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
    csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
    csv_writter.append_row(14212123, "AAPL", 120, 210.42321, "BUY");
  }

  {
    quill::FileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('w');
    file_sink_config.set_filename_append_option(FilenameAppendOption::None);

    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{filename_1, file_sink_config};
    csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
    csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
    csv_writter.append_row(14212123, "AAPL", 120, 210.42321, "BUY");
  }

  {
    quill::RotatingFileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('w');
    file_sink_config.set_filename_append_option(FilenameAppendOption::None);
    file_sink_config.set_rotation_max_file_size(512);
    file_sink_config.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Index);

    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{filename_2, file_sink_config};
    for (size_t i = 0; i < 40; ++i)
    {
      csv_writter.append_row(132121122 + i, "AAPL", i, 100.1, "BUY");
    }
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  {
    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
    REQUIRE_EQ(file_contents.size(), 4);

    REQUIRE(quill::testing::file_contains(file_contents, "order_id,symbol,quantity,price,side"));
    REQUIRE(quill::testing::file_contains(file_contents, "13212123,AAPL,100,210.32,BUY"));
    REQUIRE(quill::testing::file_contains(file_contents, "132121123,META,300,478.32,SELL"));
    REQUIRE(quill::testing::file_contains(file_contents, "14212123,AAPL,120,210.42,BUY"));
  }

  {
    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_1);
    REQUIRE_EQ(file_contents.size(), 4);

    REQUIRE(quill::testing::file_contains(file_contents, "order_id,symbol,quantity,price,side"));
    REQUIRE(quill::testing::file_contains(file_contents, "13212123,AAPL,100,210.32,BUY"));
    REQUIRE(quill::testing::file_contains(file_contents, "132121123,META,300,478.32,SELL"));
    REQUIRE(quill::testing::file_contains(file_contents, "14212123,AAPL,120,210.42,BUY"));
  }

  {
    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_2);
    REQUIRE_EQ(file_contents.size(), 8);
    REQUIRE_EQ(file_contents[0], "order_id,symbol,quantity,price,side");

    std::vector<std::string> const file_contents_1 = quill::testing::file_contents(filename_2_1);
    REQUIRE_EQ(file_contents_1.size(), 18);
    REQUIRE_EQ(file_contents_1[0], "order_id,symbol,quantity,price,side");

    std::vector<std::string> const file_contents_2 = quill::testing::file_contents(filename_2_2);
    REQUIRE_EQ(file_contents_2.size(), 17);
    REQUIRE_EQ(file_contents_2[0], "order_id,symbol,quantity,price,side");
  }

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_2_1);
  testing::remove_file(filename_2_2);
}
