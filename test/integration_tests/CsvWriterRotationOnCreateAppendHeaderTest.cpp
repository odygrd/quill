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
TEST_CASE(
  "csv_writer_rotating_file_sink_rotation_on_creation_append_mode_does_not_duplicate_header")
{
  static constexpr char const* filename = "temperatures_rotation_on_creation_append.csv";
  static constexpr char const* filename_1 = "temperatures_rotation_on_creation_append.1.csv";

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::create_file(filename, "preexisting-data\n");

  Backend::start();

  {
    RotatingFileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('a');
    file_sink_config.set_filename_append_option(FilenameAppendOption::None);
    file_sink_config.set_rotation_max_file_size(1024 * 1024);
    file_sink_config.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Index);
    file_sink_config.set_rotation_on_creation(true);

    CsvWriter<DeviceCsvSchema, FrontendOptions> csv_writer{filename, file_sink_config};
    csv_writer.close();
  }

  Backend::stop();

  {
    std::vector<std::string> const file_contents = testing::file_contents(filename);
    REQUIRE_EQ(file_contents.size(), 1);
    REQUIRE_EQ(file_contents[0], "device_id,location,status,temperature_c");
  }

  {
    std::vector<std::string> const rotated_file_contents = testing::file_contents(filename_1);
    REQUIRE_EQ(rotated_file_contents.size(), 1);
    REQUIRE_EQ(rotated_file_contents[0], "preexisting-data");
  }

  testing::remove_file(filename);
  testing::remove_file(filename_1);
}
