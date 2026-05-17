#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/CsvWriter.h"
#include "quill/core/FrontendOptions.h"

using namespace quill;

struct DeviceCsvSchema
{
  static constexpr char const* header = "device_id,location,status,temperature_c";
  static constexpr char const* format = "{},{},{},{}";
};

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
