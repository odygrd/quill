#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/CsvWriter.h"
#include "quill/core/FrontendOptions.h"

#include <string>
#include <vector>

using namespace quill;

struct FailedRotationCsvSchema
{
  static constexpr char const* header = "device_id,location,status,temperature_c";
  static constexpr char const* format = "{},{},{},{}";
};

/***/
TEST_CASE("csv_writer_failed_rotation_reopen_does_not_write_header")
{
#if !defined(_WIN32)
  return;
#else
  static constexpr char const* filename = "csv_writer_failed_rotation_header.csv";
  static constexpr char const* filename_1 = "csv_writer_failed_rotation_header.1.csv";

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::create_file(filename, std::string{"existing,row\n"} + std::string(512, 'x') + "\n");

  Backend::start();

  {
    RotatingFileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('a');
    file_sink_config.set_filename_append_option(FilenameAppendOption::None);
    file_sink_config.set_rotation_max_file_size(512);
    file_sink_config.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Index);

    CsvWriter<FailedRotationCsvSchema, FrontendOptions> csv_writer{filename, file_sink_config};

    fs::path const file_path{filename};
    HANDLE const rotation_blocker =
      ::CreateFileW(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE_NE(rotation_blocker, INVALID_HANDLE_VALUE);

    csv_writer.append_row("sensor-001", "ward-a", "stable", 21.5);
    csv_writer.flush();

    REQUIRE(::CloseHandle(rotation_blocker));
    csv_writer.close();
  }

  Backend::stop();

  // The rotation fails while the other handle is open, and the file is opened again to append to
  // it. It has content, so it keeps it and gets no header in the middle of it
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 3);
  REQUIRE_EQ(file_contents[0], "existing,row");
  REQUIRE_EQ(file_contents[1], std::string(512, 'x'));
  REQUIRE_EQ(file_contents[2], "sensor-001,ward-a,stable,21.5");

  testing::remove_file(filename);
  testing::remove_file(filename_1);
#endif
}
