#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "misc/TestUtilities.h"

#include "quill/core/DynamicFormatArgStore.h"
#include "quill/sinks/FileSink.h"

TEST_SUITE_BEGIN("FileSink");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("append_date_to_file")
{
  uint64_t const timestamp_20230612 = 1686528000000000000;
  fs::path const filename = "append_date_to_file.log";
  fs::path const expected_filename = "append_date_to_file_20230612.log";

  FileSinkConfig fsc;
  fsc.set_filename_append_option(FilenameAppendOption::StartDate);
  fsc.set_timezone(Timezone::GmtTime);

  {
    FileSink file_sink{filename, fsc, FileEventNotifier{}, true,
                       std::chrono::time_point<std::chrono::system_clock>(
                         std::chrono::seconds(timestamp_20230612 / 1000000000))};

    REQUIRE_EQ(file_sink.get_filename().filename(), expected_filename);
    REQUIRE(fs::exists(expected_filename));
  }

  testing::remove_file(expected_filename);
}

/***/
TEST_CASE("append_date_and_time_to_file")
{
  uint64_t const timestamp_20230612 = 1686528321331324000;
  fs::path const filename = "append_date_and_time_to_file.log";
  fs::path const expected_filename = "append_date_and_time_to_file_20230612_000521.log";

  FileSinkConfig fsc;
  fsc.set_filename_append_option(FilenameAppendOption::StartDateTime);
  fsc.set_timezone(Timezone::GmtTime);

  {
    FileSink file_sink{filename, fsc, FileEventNotifier{}, true,
                       std::chrono::time_point<std::chrono::system_clock>(
                         std::chrono::seconds(timestamp_20230612 / 1000000000))};

    REQUIRE_EQ(file_sink.get_filename().filename(), expected_filename);
    REQUIRE(fs::exists(expected_filename));
  }

  testing::remove_file(expected_filename);
}

/***/
TEST_CASE("create_directory")
{
  uint64_t const timestamp_20230612 = 1686528321331324000;
  fs::path const filename = fs::path{"test_create_directory"} / fs::path{"create_directory.log"};
  fs::path const expected_filename = "create_directory.log";
  fs::path const expected_dir = "test_create_directory";

  FileSinkConfig fsc;
  fsc.set_filename_append_option(FilenameAppendOption::None);

  {
    FileSink file_sink{filename, fsc, FileEventNotifier{}, true,
                       std::chrono::time_point<std::chrono::system_clock>(
                         std::chrono::seconds(timestamp_20230612 / 1000000000))};

    REQUIRE_EQ(file_sink.get_filename().filename(), expected_filename);
    REQUIRE_EQ(file_sink.get_filename().parent_path().filename(), expected_dir);
    REQUIRE(fs::exists(filename));
  }

  testing::remove_file(filename);
}

#if !defined(_WIN32)
// Symlink test, currently for Linux only but can probably also expanded to windows later

/***/
TEST_CASE("use_symlink_directory")
{
  fs::path const target_folder = "./use_symlink_directory_actual";
  fs::path const symlink_folder = "./use_symlink_directory_symlink_log";

  std::error_code ec;

  // create target folder
  fs::create_directories(target_folder, ec);
  REQUIRE_FALSE(ec);

  // Create the symlink
  fs::create_symlink(target_folder, symlink_folder, ec);
  REQUIRE_FALSE(ec);

  REQUIRE(fs::exists(target_folder));
  REQUIRE(fs::exists(symlink_folder));

  fs::path const filename = symlink_folder / fs::path{"use_symlink_directory.log"};

  FileSinkConfig fsc;
  fsc.set_filename_append_option(FilenameAppendOption::None);

  {
    FileSink file_sink{filename, fsc, FileEventNotifier{}, true};
    REQUIRE(fs::exists(filename));
  }

  // Cleanup the symlink after tests
  testing::remove_file(filename);
  fs::remove(target_folder, ec);
  fs::remove(symlink_folder, ec);
}
#endif

TEST_SUITE_END();