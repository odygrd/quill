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

struct DeviceCsvSchema
{
  static constexpr char const* header = "device_id,location,status,temperature_c";
  static constexpr char const* format = "{},{},{},{}";
};

struct FileSinkPathHelper : quill::FileSink
{
  using FileSink::FileSink;

  static fs::path appended_filename(fs::path const& filename, std::string const& pattern,
                                    Timezone timezone, std::chrono::system_clock::time_point timestamp)
  {
    return append_datetime_to_filename(filename, pattern, timezone, timestamp);
  }
};

/***/
TEST_CASE("csv_writing")
{
  static constexpr char const* filename = "orders.csv";
  static constexpr char const* filename_1 = "orders_1.csv";
  static constexpr char const* filename_2 = "orders_rotating.csv";
  static constexpr char const* filename_2_1 = "orders_rotating.1.csv";
  static constexpr char const* filename_2_2 = "orders_rotating.2.csv";
  static constexpr char const* filename_3 = "orders_3.csv";
  static constexpr char const* filename_4 = "orders_4.csv";
  static constexpr char const* filename_5 = "orders_5.csv";
  static constexpr char const* filename_6 = "orders_6.csv";
  static constexpr char const* filename_7 = "orders_shared_writer.csv";
  static constexpr char const* filename_8 = "orders_appended_date.csv";

  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  {
    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{filename};
    csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
    csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
    csv_writter.append_row(14212123, "AAPL", 120, 210.42321, "BUY");
    csv_writter.close();
  }

  {
    quill::FileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('w');
    file_sink_config.set_filename_append_option(FilenameAppendOption::None);

    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{filename_1, file_sink_config};
    csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
    csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
    csv_writter.append_row(14212123, "AAPL", 120, 210.42321, "BUY");
    csv_writter.close();
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
    csv_writter.close();
  }

  {
    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
      filename_3,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        cfg.set_filename_append_option(quill::FilenameAppendOption::None);
        return cfg;
      }());

    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{filename_3, std::move(file_sink)};
    csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
    csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
    csv_writter.append_row(14212123, "AAPL", 120, 210.42321, "BUY");
    csv_writter.close();
  }

  {
    // append mode
    {
      quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{filename_4, 'w'};
      csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
      csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
      csv_writter.append_row(14212123, "AAPL", 120, 210.42321, "BUY");
      csv_writter.close();
    }

    {
      quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{filename_4, 'a'};
      csv_writter.append_row(13212123, "AAPL", 200, 210.32321, "BUY");
      csv_writter.append_row(132121123, "META", 400, 478.32321, "SELL");
      csv_writter.append_row(14212123, "AAPL", 220, 210.42321, "BUY");
      csv_writter.close();
    }
  }

  {
    auto file_sink_5 = quill::Frontend::create_or_get_sink<quill::FileSink>(
      filename_5,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        cfg.set_filename_append_option(quill::FilenameAppendOption::None);
        return cfg;
      }());

    auto file_sink_6 = quill::Frontend::create_or_get_sink<quill::FileSink>(
      filename_6,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        cfg.set_filename_append_option(quill::FilenameAppendOption::None);
        return cfg;
      }());

    std::vector<std::shared_ptr<quill::Sink>> sinks;
    sinks.push_back(std::move(file_sink_5));
    sinks.push_back(std::move(file_sink_6));

    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{filename_5, std::move(sinks)};
    csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
    csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
    csv_writter.append_row(14212123, "AAPL", 120, 210.42321, "BUY");
    csv_writter.close();
  }

  {
    quill::FileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('w');
    file_sink_config.set_filename_append_option(FilenameAppendOption::None);

    auto csv_writer_a = std::make_unique<quill::CsvWriter<DeviceCsvSchema, quill::FrontendOptions>>(
      filename_7, file_sink_config, true);
    auto csv_writer_b = std::make_unique<quill::CsvWriter<DeviceCsvSchema, quill::FrontendOptions>>(
      filename_7, file_sink_config, false);

    csv_writer_a->append_row("dev-001", "ward-a", "online", 21.5);
    csv_writer_a->close();
    csv_writer_a.reset();

    csv_writer_b->append_row("dev-002", "ward-b", "offline", 19.0);
    csv_writer_b->close();
    csv_writer_b.reset();
  }

  {
    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{
      filename_8, 'w', FilenameAppendOption::StartDate};
    csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
    csv_writter.close();
  }

  {
    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter{
      filename_8, 'a', FilenameAppendOption::StartDate};
    csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
    csv_writter.close();
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

  {
    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_3);
    REQUIRE_EQ(file_contents.size(), 4);

    REQUIRE(quill::testing::file_contains(file_contents, "order_id,symbol,quantity,price,side"));
    REQUIRE(quill::testing::file_contains(file_contents, "13212123,AAPL,100,210.32,BUY"));
    REQUIRE(quill::testing::file_contains(file_contents, "132121123,META,300,478.32,SELL"));
    REQUIRE(quill::testing::file_contains(file_contents, "14212123,AAPL,120,210.42,BUY"));
  }

  {
    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_4);
    REQUIRE_EQ(file_contents.size(), 7);

    REQUIRE(quill::testing::file_contains(file_contents, "order_id,symbol,quantity,price,side"));
    REQUIRE(quill::testing::file_contains(file_contents, "13212123,AAPL,100,210.32,BUY"));
    REQUIRE(quill::testing::file_contains(file_contents, "132121123,META,300,478.32,SELL"));
    REQUIRE(quill::testing::file_contains(file_contents, "14212123,AAPL,120,210.42,BUY"));
    REQUIRE(quill::testing::file_contains(file_contents, "13212123,AAPL,200,210.32,BUY"));
    REQUIRE(quill::testing::file_contains(file_contents, "132121123,META,400,478.32,SELL"));
    REQUIRE(quill::testing::file_contains(file_contents, "14212123,AAPL,220,210.42,BUY"));
  }

  {
    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_5);
    REQUIRE_EQ(file_contents.size(), 4);

    REQUIRE(quill::testing::file_contains(file_contents, "order_id,symbol,quantity,price,side"));
    REQUIRE(quill::testing::file_contains(file_contents, "13212123,AAPL,100,210.32,BUY"));
    REQUIRE(quill::testing::file_contains(file_contents, "132121123,META,300,478.32,SELL"));
    REQUIRE(quill::testing::file_contains(file_contents, "14212123,AAPL,120,210.42,BUY"));
  }

  {
    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_6);
    REQUIRE_EQ(file_contents.size(), 4);

    REQUIRE(quill::testing::file_contains(file_contents, "order_id,symbol,quantity,price,side"));
    REQUIRE(quill::testing::file_contains(file_contents, "13212123,AAPL,100,210.32,BUY"));
    REQUIRE(quill::testing::file_contains(file_contents, "132121123,META,300,478.32,SELL"));
    REQUIRE(quill::testing::file_contains(file_contents, "14212123,AAPL,120,210.42,BUY"));
  }

  {
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_7);
    REQUIRE_EQ(file_contents.size(), 3);

    REQUIRE(quill::testing::file_contains(file_contents, "device_id,location,status,temperature_c"));
    REQUIRE(quill::testing::file_contains(file_contents, "dev-001,ward-a,online,21.5"));
    REQUIRE(quill::testing::file_contains(file_contents, "dev-002,ward-b,offline,19"));
  }

  {
    FileSinkConfig cfg;
    cfg.set_filename_append_option(FilenameAppendOption::StartDate);

    auto const appended_filename = FileSinkPathHelper::appended_filename(
      filename_8, cfg.append_filename_format_pattern(), cfg.timezone(), std::chrono::system_clock::now());

    std::vector<std::string> const file_contents = quill::testing::file_contents(appended_filename);
    REQUIRE_EQ(file_contents.size(), 3);

    REQUIRE(quill::testing::file_contains(file_contents, "order_id,symbol,quantity,price,side"));
    REQUIRE(quill::testing::file_contains(file_contents, "13212123,AAPL,100,210.32,BUY"));
    REQUIRE(quill::testing::file_contains(file_contents, "132121123,META,300,478.32,SELL"));

    testing::remove_file(appended_filename);
  }

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_2_1);
  testing::remove_file(filename_2_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_4);
  testing::remove_file(filename_5);
  testing::remove_file(filename_6);
  testing::remove_file(filename_7);
}

/***/
TEST_CASE("csv_writer_file_sink_config_append_mode_does_not_duplicate_header")
{
  static constexpr char const* filename = "temperatures_append_config.csv";

  testing::remove_file(filename);

  Backend::start();

  {
    FileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('w');
    file_sink_config.set_filename_append_option(FilenameAppendOption::None);

    CsvWriter<DeviceCsvSchema, FrontendOptions> csv_writer{filename, file_sink_config};
    csv_writer.append_row("sensor-001", "ward-a", "stable", 21.5);
    csv_writer.close();
  }

  Backend::stop();

  {
    std::vector<std::string> const file_contents = testing::file_contents(filename);
    REQUIRE_EQ(file_contents.size(), 2);
    REQUIRE_EQ(file_contents[0], "device_id,location,status,temperature_c");
    REQUIRE_EQ(file_contents[1], "sensor-001,ward-a,stable,21.5");
  }

  Backend::start();

  {
    FileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('a');
    file_sink_config.set_filename_append_option(FilenameAppendOption::None);

    CsvWriter<DeviceCsvSchema, FrontendOptions> csv_writer{filename, file_sink_config};
    csv_writer.append_row("sensor-002", "ward-b", "alert", 24.0);
    csv_writer.close();
  }

  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 3);
  REQUIRE_EQ(file_contents[0], "device_id,location,status,temperature_c");
  REQUIRE_EQ(file_contents[1], "sensor-001,ward-a,stable,21.5");
  REQUIRE_EQ(file_contents[2], "sensor-002,ward-b,alert,24");

  testing::remove_file(filename);
}

/***/
TEST_CASE("csv_writer_rotating_file_sink_config_append_mode_does_not_duplicate_header")
{
  static constexpr char const* filename = "temperatures_append_rotating_config.csv";
  static constexpr char const* filename_1 = "temperatures_append_rotating_config.1.csv";

  testing::remove_file(filename);
  testing::remove_file(filename_1);

  Backend::start();

  {
    RotatingFileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('w');
    file_sink_config.set_filename_append_option(FilenameAppendOption::None);
    file_sink_config.set_rotation_max_file_size(1024 * 1024);
    file_sink_config.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Index);

    CsvWriter<DeviceCsvSchema, FrontendOptions> csv_writer{filename, file_sink_config};
    csv_writer.append_row("sensor-001", "ward-a", "stable", 21.5);
    csv_writer.close();
  }

  Backend::stop();

  {
    std::vector<std::string> const file_contents = testing::file_contents(filename);
    REQUIRE_EQ(file_contents.size(), 2);
    REQUIRE_EQ(file_contents[0], "device_id,location,status,temperature_c");
    REQUIRE_EQ(file_contents[1], "sensor-001,ward-a,stable,21.5");
  }

  Backend::start();

  {
    RotatingFileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('a');
    file_sink_config.set_filename_append_option(FilenameAppendOption::None);
    file_sink_config.set_rotation_max_file_size(1024 * 1024);
    file_sink_config.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Index);

    CsvWriter<DeviceCsvSchema, FrontendOptions> csv_writer{filename, file_sink_config};
    csv_writer.append_row("sensor-002", "ward-b", "alert", 24.0);
    csv_writer.close();
  }

  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 3);
  REQUIRE_EQ(file_contents[0], "device_id,location,status,temperature_c");
  REQUIRE_EQ(file_contents[1], "sensor-001,ward-a,stable,21.5");
  REQUIRE_EQ(file_contents[2], "sensor-002,ward-b,alert,24");

  testing::remove_file(filename);
  testing::remove_file(filename_1);
}

/***/
TEST_CASE("csv_writer_operations_throw_after_close")
{
  static constexpr char const* filename = "csv_writer_operations_throw_after_close.csv";

  Backend::start();

  quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writer{filename};
  csv_writer.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
  csv_writer.close();

  REQUIRE_THROWS_AS(csv_writer.append_row(13212124, "META", 200, 310.32321, "SELL"), QuillError);
  REQUIRE_THROWS_AS(csv_writer.write_header(), QuillError);
  REQUIRE_THROWS_AS(csv_writer.flush(), QuillError);

  Backend::stop();

  testing::remove_file(filename);
}
