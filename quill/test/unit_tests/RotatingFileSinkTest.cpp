#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/core/Common.h"
#include "quill/sinks/RotatingFileSink.h"

TEST_SUITE_BEGIN("RotatingFileSink");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("rotating_file_sink_index_no_backup_limit")
{
  fs::path const filename = "rotating_file_sink_index_no_backup_limit.log";
  fs::path const filename_1 = "rotating_file_sink_index_no_backup_limit.1.log";
  fs::path const filename_2 = "rotating_file_sink_index_no_backup_limit.2.log";
  fs::path const filename_3 = "rotating_file_sink_index_no_backup_limit.3.log";

  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [3]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [2]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [0]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
}

/***/
TEST_CASE("rotating_file_sink_index_with_backup_limit_overwrite_rolled_files")
{
  // we expect to have only 2 backup files and to overwrite the rolled files
  fs::path const filename = "rotating_file_sink_index_with_backup_limit_overwrite_rolled_files.log";
  fs::path const filename_1 =
    "rotating_file_sink_index_with_backup_limit_overwrite_rolled_files.1.log";
  fs::path const filename_2 =
    "rotating_file_sink_index_with_backup_limit_overwrite_rolled_files.2.log";

  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_max_backup_files(2);
                                  cfg.set_overwrite_rolled_files(true);
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    // write some records to the file
    for (size_t i = 0; i < 12; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [11]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [10]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [9]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
}

/***/
TEST_CASE("rotating_file_sink_index_with_backup_limit_dont_overwrite_rolled_files")
{
  // we expect to have only 2 backup files and then the rotation will stop and all the
  // remaining messages will be written to the last file
  fs::path const filename =
    "rotating_file_sink_index_with_backup_limit_dont_overwrite_rolled_files.log";
  fs::path const filename_1 =
    "rotating_file_sink_index_with_backup_limit_dont_overwrite_rolled_files.1.log";
  fs::path const filename_2 =
    "rotating_file_sink_index_with_backup_limit_dont_overwrite_rolled_files.2.log";

  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_max_backup_files(2);
                                  cfg.set_overwrite_rolled_files(false);
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    // write some records to the file
    for (size_t i = 0; i < 12; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [2]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [3]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [4]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [5]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [6]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [7]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [8]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [9]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [10]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [11]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [1]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [0]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
}

/***/
TEST_CASE("rotating_file_sink_index_open_mode_write_clean_up_old_files")
{
  // On write mode, with RotateFileNamingScheme::Index, the log file names can collide
  // we need to clean up the old files
  fs::path const filename = "rotating_file_sink_index_open_mode_write_clean_up_old_files.log";
  fs::path const filename_1 = "rotating_file_sink_index_open_mode_write_clean_up_old_files.1.log";
  fs::path const filename_2 = "rotating_file_sink_index_open_mode_write_clean_up_old_files.2.log";
  fs::path const filename_3 = "rotating_file_sink_index_open_mode_write_clean_up_old_files.3.log";
  fs::path const filename_4 =
    "rotating_file_sink_index_open_mode_write_clean_up_old_files_another.2.log";

  // create all files simulating the previous run
  testing::create_file(filename);
  testing::create_file(filename_1);
  testing::create_file(filename_2);
  testing::create_file(filename_4);

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_4));

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Index);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{}};

    REQUIRE_EQ(static_cast<size_t>(fs::file_size(filename)), 0);
    REQUIRE(!fs::exists(filename_1));
    REQUIRE(!fs::exists(filename_2));
    REQUIRE(fs::exists(filename_4));

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [3]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [2]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [0]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_4);
}

/***/
TEST_CASE("rotating_file_sink_index_open_mode_write_dont_clean_up_old_files")
{
  // On write mode, with RotateFileNamingScheme::Index, the log file names can collide with old
  // files, but we do not want to clean them up
  // Eventually the files later will get overwritten by the new files

  fs::path const filename = "rotating_file_sink_index_open_mode_write_dont_clean_up_old_files.log";
  fs::path const filename_1 =
    "rotating_file_sink_index_open_mode_write_dont_clean_up_old_files.1.log";
  fs::path const filename_2 =
    "rotating_file_sink_index_open_mode_write_dont_clean_up_old_files.2.log";
  fs::path const filename_3 =
    "rotating_file_sink_index_open_mode_write_dont_clean_up_old_files.3.log";
  fs::path const filename_4 =
    "rotating_file_sink_index_open_mode_write_dont_clean_up_old_files_another.2.log";

  // create all files simulating the previous run
  testing::create_file(filename);
  testing::create_file(filename_1);
  testing::create_file(filename_2);
  testing::create_file(filename_4);

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_4));

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_remove_old_files(false);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Index);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{}};

    REQUIRE_EQ(static_cast<size_t>(fs::file_size(filename)), 0);
    REQUIRE(fs::exists(filename_1));
    REQUIRE(fs::exists(filename_2));
    REQUIRE(fs::exists(filename_4));

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [3]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [2]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [0]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
}

/***/
TEST_CASE("rotating_file_sink_index_open_mode_append")
{
  // In append mode, with RotateFileNamingScheme::Index, the log file names can collide
  // we want to start appending to the last log file and then also rotate it with the correct index
  // continuing the count from the last run
  // On write mode, with RotateFileNamingScheme::Index, the log file names can collide
  // we need to clean up the old files
  fs::path const filename = "rotating_file_sink_index_open_mode_append.log";
  fs::path const filename_1 = "rotating_file_sink_index_open_mode_append.1.log";
  fs::path const filename_2 = "rotating_file_sink_index_open_mode_append.2.log";
  fs::path const filename_3 = "rotating_file_sink_index_open_mode_append.3.log";
  fs::path const filename_4 = "rotating_file_sink_index_open_mode_append.4.log";
  fs::path const filename_5 = "rotating_file_sink_index_open_mode_append.5.log";
  fs::path const filename_6 = "rotating_file_sink_index_open_mode_append.6.log";

  // create all files simulating the previous run
  testing::create_file(filename, "Existing [2]");
  testing::create_file(filename_1, "Existing [1]");
  testing::create_file(filename_2, "Existing [0]");

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Index);
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{}};

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename_4));
  REQUIRE(fs::exists(filename_5));
  REQUIRE(fs::exists(filename_6));

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [3]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [2]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [0]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_4);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Existing [2]"), true);

  std::vector<std::string> const file_contents_5 = testing::file_contents(filename_5);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Existing [1]"), true);

  std::vector<std::string> const file_contents_6 = testing::file_contents(filename_6);
  REQUIRE_EQ(testing::file_contains(file_contents_6, "Existing [0]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_4);
  testing::remove_file(filename_5);
  testing::remove_file(filename_6);
}

/** Tests for scheme date **/

/***/
TEST_CASE("rotating_file_sink_date_no_backup_limit")
{
  // e.g .2023-06-12. is the date the rotation occurred
  uint64_t const timestamp_20230612 = 1686528000000000000;
  uint64_t const timestamp_20230613 = 1686614400000000000;
  uint64_t const timestamp_20230614 = 1686700800000000000;

  fs::path const filename_1 = "rotating_file_sink_date_no_backup_limit.20230612.3.log";
  fs::path const filename_2 = "rotating_file_sink_date_no_backup_limit.20230612.2.log";
  fs::path const filename_3 = "rotating_file_sink_date_no_backup_limit.20230612.1.log";
  fs::path const filename_4 = "rotating_file_sink_date_no_backup_limit.20230612.log";
  fs::path const filename_5 = "rotating_file_sink_date_no_backup_limit.20230613.log";
  fs::path const filename_6 = "rotating_file_sink_date_no_backup_limit.20230614.1.log";
  fs::path const filename_7 = "rotating_file_sink_date_no_backup_limit.20230614.log";
  fs::path const filename = "rotating_file_sink_date_no_backup_limit.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230612, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    {
      std::string s{"Record [" + std::to_string(4) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230613, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    // change the date and rotate again
    for (size_t i = 5; i < 8; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230614, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename_4));
  REQUIRE(fs::exists(filename_5));
  REQUIRE(fs::exists(filename_6));
  REQUIRE(fs::exists(filename_7));

  // Read file and check
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [0]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [2]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_4);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Record [3]"), true);

  std::vector<std::string> const file_contents_5 = testing::file_contents(filename_5);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Record [4]"), true);

  std::vector<std::string> const file_contents_6 = testing::file_contents(filename_6);
  REQUIRE_EQ(testing::file_contains(file_contents_6, "Record [5]"), true);

  std::vector<std::string> const file_contents_7 = testing::file_contents(filename_7);
  REQUIRE_EQ(testing::file_contains(file_contents_7, "Record [6]"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [7]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_4);
  testing::remove_file(filename_5);
  testing::remove_file(filename_6);
  testing::remove_file(filename_7);
}

/***/
TEST_CASE("rotating_file_sink_date_with_backup_limit_overwrite_rolled_files")
{
  // e.g .2023-06-12. is the date the rotation occurred
  uint64_t const timestamp_20230612 = 1686528000000000000;
  uint64_t const timestamp_20230613 = 1686614400000000000;
  uint64_t const timestamp_20230614 = 1686700800000000000;

  fs::path const filename_2 =
    "rotating_file_sink_date_with_backup_limit_overwrite_rolled_files.20230614.1.log";
  fs::path const filename_1 =
    "rotating_file_sink_date_with_backup_limit_overwrite_rolled_files.20230614.log";
  fs::path const filename = "rotating_file_sink_date_with_backup_limit_overwrite_rolled_files.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_max_backup_files(2);
        cfg.set_overwrite_rolled_files(true);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230612, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    {
      // change the date and rotate again
      std::string s{"Record [" + std::to_string(4) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230613, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    // change the date and rotate again
    for (size_t i = 5; i < 8; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230614, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));

  // Read file and check
  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [5]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [6]"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [7]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
}

/***/
TEST_CASE("rotating_file_sink_date_with_backup_limit_dont_overwrite_rolled_files")
{
  // e.g .2023-06-12. is the date the rotation occurred
  uint64_t const timestamp_20230612 = 1686528000000000000;
  uint64_t const timestamp_20230613 = 1686614400000000000;
  uint64_t const timestamp_20230614 = 1686700800000000000;

  fs::path const filename_1 =
    "rotating_file_sink_date_with_backup_limit_dont_overwrite_rolled_files.20230612.3.log";
  fs::path const filename_2 =
    "rotating_file_sink_date_with_backup_limit_dont_overwrite_rolled_files.20230612.2.log";
  fs::path const filename_3 =
    "rotating_file_sink_date_with_backup_limit_dont_overwrite_rolled_files.20230612.1.log";
  fs::path const filename_4 =
    "rotating_file_sink_date_with_backup_limit_dont_overwrite_rolled_files.20230612.log";
  fs::path const filename =
    "rotating_file_sink_date_with_backup_limit_dont_overwrite_rolled_files.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_max_backup_files(4);
        cfg.set_overwrite_rolled_files(false);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230612, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    {
      // change the date and rotate again
      std::string s{"Record [" + std::to_string(4) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230613, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    // change the date and rotate again
    for (size_t i = 5; i < 8; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230614, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename_4));

  // Read file and check
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [0]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [2]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_4);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Record [3]"), true);

  std::vector<std::string> const file_contents_5 = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Record [4]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Record [5]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Record [6]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Record [7]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_4);
}

/***/
TEST_CASE("rotating_file_sink_date_open_mode_write_clean_up_old_files")
{
  // On write mode, with RotateFileNamingScheme::Index, the log file names can collide

  // e.g .2023-06-12. is the date the rotation occurred
  uint64_t const timestamp_20230612 = 1686528000000000000;

  fs::path const filename_1 =
    "rotating_file_sink_date_open_mode_write_clean_up_old_files.20230612.2.log";
  fs::path const filename_2 =
    "rotating_file_sink_date_open_mode_write_clean_up_old_files.20230612.1.log";
  fs::path const filename_3 =
    "rotating_file_sink_date_open_mode_write_clean_up_old_files.20230612.log";
  fs::path const filename = "rotating_file_sink_date_open_mode_write_clean_up_old_files.log";

  //  create files simulating the previous run
  testing::create_file(filename_1, "Existing [2]");
  testing::create_file(filename_2, "Existing [1]");
  testing::create_file(filename_3, "Existing [0]");
  testing::create_file(filename, "Existing [0]");

  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename));

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_remove_old_files(true);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    REQUIRE_EQ(static_cast<size_t>(fs::file_size(filename)), 0);
    REQUIRE(!fs::exists(filename_1));
    REQUIRE(!fs::exists(filename_2));
    REQUIRE(!fs::exists(filename_3));

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230612, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));

  // Read file and check
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [3]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [2]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [1]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Record [0]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
}

/***/
TEST_CASE("rotating_file_sink_date_open_mode_write_dont_clean_up_old_files")
{
  // On write mode, the log file names can collide with old
  // files, but we do not want to clean them up
  // Eventually the files later will get overwritten by the new files

  // e.g .2023-06-12. is the date the rotation occurred
  uint64_t const timestamp_20230612 = 1686528000000000000;

  fs::path const filename_1 =
    "rotating_file_sink_date_open_mode_write_dont_clean_up_old_files.20230612.2.log";
  fs::path const filename_2 =
    "rotating_file_sink_date_open_mode_write_dont_clean_up_old_files.20230612.1.log";
  fs::path const filename_3 =
    "rotating_file_sink_date_open_mode_write_dont_clean_up_old_files.20230612.log";
  fs::path const filename = "rotating_file_sink_date_open_mode_write_dont_clean_up_old_files.log";

  //  create files simulating the previous run
  testing::create_file(filename_1, "Existing [2]");
  testing::create_file(filename_2, "Existing [1]");
  testing::create_file(filename_3, "Existing [0]");
  testing::create_file(filename, "Existing [0]");

  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename));

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_remove_old_files(false);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    REQUIRE_EQ(static_cast<size_t>(fs::file_size(filename)), 0);
    REQUIRE(fs::exists(filename_1));
    REQUIRE(fs::exists(filename_2));
    REQUIRE(fs::exists(filename_3));

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230612, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));

  // Read file and check
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [3]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [2]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [1]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Record [0]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
}

/***/
TEST_CASE("rotating_file_sink_data_open_mode_append")
{
  // In append mode, with RotateFileNamingScheme::Index, the log file names can collide
  // we want to start appending to the last log file and then also rotate it with the correct index
  // continuing the count from the last run
  // e.g .2023-06-12. is the date the rotation occurred
  uint64_t const timestamp_20230612 = 1686528000000000000;

  fs::path const filename_7 = "rotating_file_sink_data_open_mode_append.20230612.6.log";
  fs::path const filename_6 = "rotating_file_sink_data_open_mode_append.20230612.5.log";
  fs::path const filename_5 = "rotating_file_sink_data_open_mode_append.20230612.4.log";
  fs::path const filename_4 = "rotating_file_sink_data_open_mode_append.20230612.3.log";
  fs::path const filename_3 = "rotating_file_sink_data_open_mode_append.20230612.2.log"; // 0
  fs::path const filename_2 = "rotating_file_sink_data_open_mode_append.20230612.1.log"; // 1
  fs::path const filename_1 = "rotating_file_sink_data_open_mode_append.20230612.log";   // 2
  fs::path const filename = "rotating_file_sink_data_open_mode_append.log";              // 3

  //  create files simulating the previous run
  testing::create_file(filename_1, "Existing [2]");
  testing::create_file(filename_2, "Existing [1]");
  testing::create_file(filename_3, "Existing [0]");
  testing::create_file(filename, "Existing [3]");

  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename));

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    REQUIRE(fs::exists(filename));
    REQUIRE(fs::exists(filename_1));
    REQUIRE(fs::exists(filename_2));
    REQUIRE(fs::exists(filename_3));

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230612, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename_4));
  REQUIRE(fs::exists(filename_5));
  REQUIRE(fs::exists(filename_6));
  REQUIRE(fs::exists(filename_7));

  // Read file and check
  std::vector<std::string> const file_contents_7 = testing::file_contents(filename_7);
  REQUIRE_EQ(testing::file_contains(file_contents_7, "Existing [0]"), true);

  std::vector<std::string> const file_contents_6 = testing::file_contents(filename_6);
  REQUIRE_EQ(testing::file_contains(file_contents_6, "Existing [1]"), true);

  std::vector<std::string> const file_contents_5 = testing::file_contents(filename_5);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Existing [2]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_4);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Existing [3]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [0]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [2]"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [3]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_4);
  testing::remove_file(filename_5);
  testing::remove_file(filename_6);
  testing::remove_file(filename_7);
}

/** Tests for scheme DateAndTime **/

/***/
TEST_CASE("rotating_file_sink_dateandtime_no_backup_limit")
{
  // e.g .2023-06-12. is the date the rotation occurred
  uint64_t const timestamp_20230612 = 1686538000000000000;
  uint64_t const timestamp_20230613 = 1686634400000000000;
  uint64_t const timestamp_20230613_2 = 1686639400000000000;
  uint64_t const timestamp_20230614 = 1686700800000000000;

  fs::path const filename_1 =
    "rotating_file_sink_dateandtime_no_backup_limit.20230612_024640.3.log";
  fs::path const filename_2 =
    "rotating_file_sink_dateandtime_no_backup_limit.20230612_024640.2.log";
  fs::path const filename_3 =
    "rotating_file_sink_dateandtime_no_backup_limit.20230612_024640.1.log";
  fs::path const filename_4 = "rotating_file_sink_dateandtime_no_backup_limit.20230612_024640.log";
  fs::path const filename_5 = "rotating_file_sink_dateandtime_no_backup_limit.20230613_053320.log";
  fs::path const filename_6 = "rotating_file_sink_dateandtime_no_backup_limit.20230613_065640.log";
  fs::path const filename_7 =
    "rotating_file_sink_dateandtime_no_backup_limit.20230614_000000.1.log";
  fs::path const filename_8 = "rotating_file_sink_dateandtime_no_backup_limit.20230614_000000.log";
  fs::path const filename = "rotating_file_sink_dateandtime_no_backup_limit.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::DateAndTime);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230612, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    {
      // change the date and rotate again

      std::string s{"Record [" + std::to_string(4) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230613, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    {
      // change the date and rotate again
      std::string s{"Record [" + std::to_string(5) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230613_2, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    // change the date and rotate again
    for (size_t i = 6; i < 9; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230614, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename_4));
  REQUIRE(fs::exists(filename_5));
  REQUIRE(fs::exists(filename_6));
  REQUIRE(fs::exists(filename_7));
  REQUIRE(fs::exists(filename_8));

  // Read file and check
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [0]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [2]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_4);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Record [3]"), true);

  std::vector<std::string> const file_contents_5 = testing::file_contents(filename_5);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Record [4]"), true);

  std::vector<std::string> const file_contents_6 = testing::file_contents(filename_6);
  REQUIRE_EQ(testing::file_contains(file_contents_6, "Record [5]"), true);

  std::vector<std::string> const file_contents_7 = testing::file_contents(filename_7);
  REQUIRE_EQ(testing::file_contains(file_contents_7, "Record [6]"), true);

  std::vector<std::string> const file_contents_8 = testing::file_contents(filename_8);
  REQUIRE_EQ(testing::file_contains(file_contents_8, "Record [7]"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [8]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_4);
  testing::remove_file(filename_5);
  testing::remove_file(filename_6);
  testing::remove_file(filename_7);
  testing::remove_file(filename_8);
}

/***/
TEST_CASE("rotating_file_sink_dateandtime_with_backup_limit_overwrite_rolled_files")
{
  // e.g .2023-06-12. is the date the rotation occurred
  uint64_t const timestamp_20230612 = 1686578000000000000;
  uint64_t const timestamp_20230613 = 1686624400000000000;
  uint64_t const timestamp_20230614 = 1686720800000000000;

  fs::path const filename_2 =
    "rotating_file_sink_dateandtime_with_backup_limit_overwrite_rolled_files.20230614_053320.1.log";
  fs::path const filename_1 =
    "rotating_file_sink_dateandtime_with_backup_limit_overwrite_rolled_files.20230614_053320.log";
  fs::path const filename =
    "rotating_file_sink_dateandtime_with_backup_limit_overwrite_rolled_files.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::DateAndTime);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_max_backup_files(2);
        cfg.set_overwrite_rolled_files(true);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    {
      // change the date and rotate again
      std::string s{"Record [" + std::to_string(4) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230613, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    // change the date and rotate again
    for (size_t i = 5; i < 8; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230614, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));

  // Read file and check
  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [5]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [6]"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [7]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
}

/***/
TEST_CASE("rotating_file_sink_dateandtime_with_backup_limit_dont_overwrite_rolled_files")
{
  // e.g .2023-06-12. is the date the rotation occurred
  uint64_t const timestamp_20230612 = 1686548000000000000;
  uint64_t const timestamp_20230613 = 1686619400000000000;
  uint64_t const timestamp_20230614 = 1686710800000000000;

  fs::path const filename_1 =
    "rotating_file_sink_dateandtime_with_backup_limit_dont_overwrite_rolled_files.20230612_053320."
    "3."
    "log";
  fs::path const filename_2 =
    "rotating_file_sink_dateandtime_with_backup_limit_dont_overwrite_rolled_files.20230612_053320."
    "2."
    "log";
  fs::path const filename_3 =
    "rotating_file_sink_dateandtime_with_backup_limit_dont_overwrite_rolled_files.20230612_053320."
    "1."
    "log";
  fs::path const filename_4 =
    "rotating_file_sink_dateandtime_with_backup_limit_dont_overwrite_rolled_files.20230612_053320."
    "log";
  fs::path const filename =
    "rotating_file_sink_dateandtime_with_backup_limit_dont_overwrite_rolled_files.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::DateAndTime);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_max_backup_files(4);
        cfg.set_overwrite_rolled_files(false);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230612, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    {
      // change the date and rotate again
      std::string s{"Record [" + std::to_string(4) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230613, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    // change the date and rotate again
    for (size_t i = 5; i < 8; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230614, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename_4));

  // Read file and check
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [0]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [2]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_4);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Record [3]"), true);

  std::vector<std::string> const file_contents_5 = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Record [4]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Record [5]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Record [6]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents_5, "Record [7]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_4);
}

/***/
TEST_CASE("time_rotation_minutes_rotating_file_sink_index")
{
  uint64_t const timestamp_20230612 = 1686528000000000000;

  fs::path const filename_1 = "time_rotation_minutes_rotating_file_sink_index.20230612.3.log";
  fs::path const filename_2 = "time_rotation_minutes_rotating_file_sink_index.20230612.2.log";
  fs::path const filename_3 = "time_rotation_minutes_rotating_file_sink_index.20230612.1.log";
  fs::path const filename_4 = "time_rotation_minutes_rotating_file_sink_index.20230612.log";
  fs::path const filename = "time_rotation_minutes_rotating_file_sink_index.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_frequency_and_interval('m', 1);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    uint64_t timestamp = timestamp_20230612;

    for (size_t i = 0; i < 5; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);

      timestamp += std::chrono::nanoseconds(std::chrono::minutes(1)).count();
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename_4));

  // Read file and check
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [0]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [2]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_4);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Record [3]"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [4]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_4);
}

/***/
TEST_CASE("time_rotation_hours_rotating_file_sink_index")
{
  uint64_t const timestamp_20230612 = 1686528000000000000;

  fs::path const filename_1 = "time_rotation_hours_rotating_file_sink_index.20230612.1.log";
  fs::path const filename_2 = "time_rotation_hours_rotating_file_sink_index.20230612.log";
  fs::path const filename = "time_rotation_hours_rotating_file_sink_index.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_frequency_and_interval('h', 2);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    uint64_t timestamp = timestamp_20230612;

    for (size_t i = 0; i < 5; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);

      timestamp += std::chrono::nanoseconds(std::chrono::hours(1)).count();
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));

  // Read file and check
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [0]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [2]"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [3]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [4]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
}

/***/
TEST_CASE("time_rotation_daily_at_time_rotating_file_sink_index")
{
  uint64_t const timestamp_20230612 = 1686529000000000000;

  fs::path const filename_1 = "time_rotation_daily_at_time_rotating_file_sink_index.20230612.log";
  fs::path const filename_2 = "time_rotation_daily_at_time_rotating_file_sink_index.20230613.log";
  fs::path const filename_3 = "time_rotation_daily_at_time_rotating_file_sink_index.20230614.log";
  fs::path const filename_4 = "time_rotation_daily_at_time_rotating_file_sink_index.20230615.log";
  fs::path const filename = "time_rotation_daily_at_time_rotating_file_sink_index.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_time_daily("08:00");
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    uint64_t ts = timestamp_20230612;

    for (size_t i = 0; i < 5; ++i)
    {
      struct tm timeinfo;
      char buffer[80];

      // Get the timeinfo struct using std::gmtime#
      time_t timestamp = ts / 1000000000;
      gmtime_rs(&timestamp, &timeinfo);

      // Use std::strftime to format the date
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

      std::string s{"Record [" + std::string(buffer) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, ts, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);

      ts += std::chrono::nanoseconds(std::chrono::hours(24)).count();
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename_4));

  // Read file and check
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [2023-06-12 00:16:40]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [2023-06-13 00:16:40]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [2023-06-14 00:16:40]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_4);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Record [2023-06-15 00:16:40]"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [2023-06-16 00:16:40]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_4);
}

/***/
TEST_CASE("max_size_and_time_rotation_daily_at_time_rotating_file_sink_index")
{
  uint64_t const timestamp_20230612 = 1686529000000000000;

  fs::path const filename_1 =
    "max_size_and_time_rotation_daily_at_time_rotating_file_sink_index.20230612.2.log";
  fs::path const filename_2 =
    "max_size_and_time_rotation_daily_at_time_rotating_file_sink_index.20230612.1.log";
  fs::path const filename_3 =
    "max_size_and_time_rotation_daily_at_time_rotating_file_sink_index.20230612.log";
  fs::path const filename_4 =
    "max_size_and_time_rotation_daily_at_time_rotating_file_sink_index.20230613.log";
  fs::path const filename = "max_size_and_time_rotation_daily_at_time_rotating_file_sink_index.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_time_daily("08:00");
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    uint64_t ts = timestamp_20230612;

    {
      struct tm timeinfo;
      char buffer[80];

      // Get the timeinfo struct using std::gmtime
      time_t timestamp = ts / 1000000000;
      gmtime_rs(&timestamp, &timeinfo);

      // Use std::strftime to format the date
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

      std::string s{"Record [0 " + std::string(buffer) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      rfh.write_log(nullptr, ts, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    {
      struct tm timeinfo;
      char buffer[80];

      // Get the timeinfo struct using std::gmtime#
      time_t timestamp = ts / 1000000000;
      gmtime_rs(&timestamp, &timeinfo);

      // Use std::strftime to format the date
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

      std::string s{"Record [1 " + std::string(buffer) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, ts, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    for (size_t i = 2; i < 4; ++i)
    {
      struct tm timeinfo;
      char buffer[80];

      // Get the timeinfo struct using std::gmtime#
      time_t timestamp = ts / 1000000000;
      gmtime_rs(&timestamp, &timeinfo);

      // Use std::strftime to format the date
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

      std::string s{"Record [" + std::to_string(i) + " " + std::string(buffer) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      rfh.write_log(nullptr, ts, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);

      ts += std::chrono::nanoseconds(std::chrono::hours(24)).count();
    }

    {
      struct tm timeinfo;
      char buffer[80];

      // Get the timeinfo struct using std::gmtime#
      time_t timestamp = ts / 1000000000;
      gmtime_rs(&timestamp, &timeinfo);

      // Use std::strftime to format the date
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

      std::string s{"Record [4 " + std::string(buffer) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, ts, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename_4));

  // Read file and check
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [0 2023-06-12 00:16:40]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1 2023-06-12 00:16:40]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [2 2023-06-12 00:16:40]"), true);

  std::vector<std::string> const file_contents_4 = testing::file_contents(filename_4);
  REQUIRE_EQ(testing::file_contains(file_contents_4, "Record [3 2023-06-13 00:16:40]"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [4 2023-06-14 00:16:40]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
}

/***/
TEST_CASE("rotating_file_sink_index_dont_remove_unrelated_files")
{
  fs::path const filename = "rotating_file_sink_index_dont_remove_unrelated_files.log";
  fs::path const filename_1 = "rotating_file_sink_index_dont_remove_unrelated_files.1.log";
  fs::path const filename_2 = "rotating_file_sink_index_dont_remove_unrelated_files.2.log";
  fs::path const filename_3 = "rotating_file_sink_index_dont_remove_unrelated_files.3.log";
  fs::path const filename_yaml = "rotating_file_sink_index_dont_remove_unrelated_files.yaml";
  fs::path const filename_other = "config_rotating_file_sink_index_dont_remove_unrelated_files";

  // create all files simulating the previous run
  testing::create_file(filename, "Existing [2]");
  testing::create_file(filename_1, "Existing [1]");
  testing::create_file(filename_2, "Existing [0]");
  testing::create_file(filename_3, "Existing [-1]");
  testing::create_file(filename_yaml, "YAML config");
  testing::create_file(filename_other, "YAML config");

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));
  REQUIRE(fs::exists(filename_yaml));
  REQUIRE(fs::exists(filename_other));

  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_remove_old_files(true);
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    REQUIRE(!fs::exists(filename_1));
    REQUIRE(!fs::exists(filename_2));
    REQUIRE(!fs::exists(filename_3));
    REQUIRE(fs::exists(filename_yaml));
    REQUIRE(fs::exists(filename_other));

    // write some records to the file
    for (size_t i = 0; i < 4; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE(fs::exists(filename_3));

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [3]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [2]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [1]"), true);

  std::vector<std::string> const file_contents_3 = testing::file_contents(filename_3);
  REQUIRE_EQ(testing::file_contains(file_contents_3, "Record [0]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
  testing::remove_file(filename_yaml);
  testing::remove_file(filename_other);
}

/***/
TEST_CASE("rotating_file_sink_invalid_params")
{
  fs::path const filename = "rotating_file_sink_invalid_params.log";
  REQUIRE_THROWS(RotatingFileSink{filename, []()
                                  {
                                    RotatingFileSinkConfig cfg;
                                    cfg.set_rotation_max_file_size(128);
                                    cfg.set_open_mode('w');
                                    return cfg;
                                  }()});

  REQUIRE_FALSE(fs::exists(filename));

  REQUIRE_THROWS(RotatingFileSink{filename, []()
                                  {
                                    RotatingFileSinkConfig cfg;
                                    cfg.set_rotation_frequency_and_interval('Z', 123);
                                    cfg.set_open_mode('w');
                                    return cfg;
                                  }()});

  REQUIRE_FALSE(fs::exists(filename));

  REQUIRE_THROWS(RotatingFileSink{filename, []()
                                  {
                                    RotatingFileSinkConfig cfg;
                                    cfg.set_rotation_frequency_and_interval('M', 0);
                                    cfg.set_open_mode('w');
                                    return cfg;
                                  }()});

  REQUIRE_FALSE(fs::exists(filename));
}

TEST_SUITE_END();