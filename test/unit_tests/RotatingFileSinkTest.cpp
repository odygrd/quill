#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/core/Common.h"
#include "quill/sinks/RotatingFileSink.h"
#include "quill/sinks/RotatingJsonFileSink.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <string>

#if !defined(_WIN32)
  #include <sys/stat.h>
#endif

TEST_SUITE_BEGIN("RotatingFileSink");

using namespace quill;
using namespace quill::detail;

namespace
{
class FailingRenameRotatingFileSink : public RotatingFileSink
{
public:
  using RotatingFileSink::RotatingFileSink;

protected:
  bool rename_file(fs::path const&, fs::path const&) noexcept override { return false; }
};

class PartialFailingRenameRotatingFileSink : public RotatingFileSink
{
public:
  using RotatingFileSink::RotatingFileSink;

  void set_fail_on_call(uint32_t n) { _fail_on_call = n; }
  void reset_rename_count() { _rename_call_count = 0; }

  std::deque<FileInfo> const& created_files() const { return _created_files; }

protected:
  bool rename_file(fs::path const& previous_file, fs::path const& new_file) noexcept override
  {
    ++_rename_call_count;
    if (_rename_call_count == _fail_on_call)
    {
      return false;
    }
    return RotatingFileSink::rename_file(previous_file, new_file);
  }

private:
  uint32_t _rename_call_count{0};
  uint32_t _fail_on_call{0};
};
} // namespace

/***/
TEST_CASE("rotating_file_sink_config_rejects_invalid_daily_rotation_time")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  RotatingFileSinkConfig cfg;

  REQUIRE_NOTHROW(cfg.set_rotation_time_daily("08:30"));
  REQUIRE_EQ(cfg.daily_rotation_time().first, std::chrono::hours{8});
  REQUIRE_EQ(cfg.daily_rotation_time().second, std::chrono::minutes{30});

  REQUIRE_THROWS_AS(cfg.set_rotation_time_daily("-1:00"), QuillError);
  REQUIRE_THROWS_AS(cfg.set_rotation_time_daily("ab:cd"), QuillError);
  REQUIRE_THROWS_AS(cfg.set_rotation_time_daily("24:00"), QuillError);
  REQUIRE_THROWS_AS(cfg.set_rotation_time_daily("23:60"), QuillError);
#else
  return;
#endif
}

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

TEST_CASE("rotating_file_sink_rename_failure_preserves_current_file_and_state")
{
  fs::path const filename =
    "rotating_file_sink_rename_failure_preserves_current_file_and_state.log";
  fs::path const filename_1 =
    "rotating_file_sink_rename_failure_preserves_current_file_and_state.1.log";

  {
    auto rfh = FailingRenameRotatingFileSink{filename,
                                             []()
                                             {
                                               RotatingFileSinkConfig cfg;
                                               cfg.set_rotation_max_file_size(1024);
                                               cfg.set_open_mode('w');
                                               return cfg;
                                             }(),
                                             FileEventNotifier{}};

    for (size_t i = 0; i < 2; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE_FALSE(fs::exists(filename_1));

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [0]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [1]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
}

/***/
TEST_CASE("rotating_file_sink_size_rotation_after_deleted_file_reopen")
{
#if !defined(_WIN32)
  // Deleting a file that is still held open requires POSIX semantics
  fs::path const filename = "rotating_file_sink_size_rotation_after_deleted_file_reopen.log";
  fs::path const filename_1 = "rotating_file_sink_size_rotation_after_deleted_file_reopen.1.log";

  testing::remove_file(filename);
  testing::remove_file(filename_1);

  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(2048);
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    auto write_record = [&rfh](size_t i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      std::string f;
      f.resize(590);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    };

    write_record(0);
    rfh.flush_sink();

    // Simulate an external process (e.g. logrotate) deleting the active log file
    fs::remove(filename);

    write_record(1);
    rfh.flush_sink(); // detects the deleted file and reopens a fresh one

    // Both records fit inside rotation_max_file_size of the recreated file. A stale
    // _file_size carried over from before the reopen would trigger a bogus rotation here
    write_record(2);
    write_record(3);
  }

  REQUIRE(fs::exists(filename));
  REQUIRE_FALSE(fs::exists(filename_1));

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [2]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [3]"), true);

  testing::remove_file(filename);
#endif
}

TEST_CASE("rotating_json_file_sink_size_rotation_uses_actual_json_size")
{
  fs::path const estimate_filename = "rotating_json_file_sink_size_rotation_estimate.log";
  fs::path const filename = "rotating_json_file_sink_size_rotation_actual.log";
  fs::path const filename_1 = "rotating_json_file_sink_size_rotation_actual.1.log";

  static constexpr MacroMetadata macro_metadata{"rotating_json_file_sink_size_rotation.cpp:42",
                                                "test_fn",
                                                "json message",
                                                nullptr,
                                                LogLevel::Info,
                                                MacroMetadata::Event::Log};

  std::vector<std::pair<std::string, std::string>> named_args{{"payload", std::string(700, 'A')}};

  auto write_json_record = [&named_args](auto& sink)
  {
    sink.write_log(&macro_metadata, 0, "thread_id", "thread_name", "process_id", "logger_name",
                   LogLevel::Info, "INFO", "I", &named_args, "ignored_log_message", "x");
  };

  size_t estimated_json_size = 0;
  {
    FileSinkConfig cfg;
    cfg.set_open_mode('w');

    JsonFileSink json_sink{estimate_filename, cfg, FileEventNotifier{}};
    write_json_record(json_sink);
    json_sink.flush_sink();

    estimated_json_size = static_cast<size_t>(fs::file_size(estimate_filename));
  }

  testing::remove_file(estimate_filename);

  {
    // One JSON record already consumes `estimated_json_size` bytes on disk, so with
    // max_file_size = estimated_json_size + 1 the second write should rotate.
    // This catches the bug where rotation was checking `log_statement.size()` ("x")
    // instead of the actual generated JSON payload size.
    RotatingFileSinkConfig cfg;
    cfg.set_open_mode('w');
    cfg.set_rotation_max_file_size(estimated_json_size + 1);

    RotatingJsonFileSink rotating_json_sink{filename, cfg, FileEventNotifier{}};
    write_json_record(rotating_json_sink);
    rotating_json_sink.flush_sink();
    write_json_record(rotating_json_sink);
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);

  REQUIRE_EQ(file_contents.size(), 1);
  REQUIRE_EQ(file_contents_1.size(), 1);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
}

TEST_CASE("rotating_json_file_sink_failed_rotation_does_not_replay_stale_message")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  // Regression: with size and time rotation both enabled, estimate_write_size() caches the built
  // json message for the record. When the size rotation then threw (e.g. disk full during the
  // reopen), the cached message was never consumed, and the next record that took the time
  // rotation branch (which skips the estimate) wrote the previous record's stale json payload
  // in its place
  fs::path const estimate_filename = "rotating_json_failed_rotation_stale_estimate.log";
  fs::path const filename = "rotating_json_failed_rotation_stale.log";
  fs::path const filename_1 = "rotating_json_failed_rotation_stale.1.log";

  testing::remove_file(estimate_filename);
  testing::remove_file(filename);
  testing::remove_file(filename_1);

  static constexpr MacroMetadata macro_metadata{
    "rotating_json_failed_rotation_stale.cpp:42", "test_fn", "json message", nullptr, LogLevel::Info, MacroMetadata::Event::Log};

  auto write_json_record = [](auto& sink, std::string const& payload, uint64_t timestamp)
  {
    std::vector<std::pair<std::string, std::string>> named_args{{"payload", payload}};
    sink.write_log(&macro_metadata, timestamp, "thread_id", "thread_name", "process_id",
                   "logger_name", LogLevel::Info, "INFO", "I", &named_args, "ignored_log_message",
                   "x");
  };

  // 2023-01-01 00:30:00 UTC; hourly rotation puts the first time boundary at most one hour later
  std::chrono::system_clock::time_point const start_time{std::chrono::seconds{1672533000}};
  uint64_t const start_time_ns = static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count());
  uint64_t const record_ab_timestamp = start_time_ns + 1'000'000'000;                  // + 1 second
  uint64_t const record_c_timestamp = start_time_ns + (2u * 3600u * 1'000'000'000ull); // + 2 hours

  // large enough that a single json record exceeds the 512-byte rotation_max_file_size minimum
  std::string const payload_a(700, 'a');
  std::string const payload_b(700, 'b');
  std::string const payload_c(700, 'c');

  // Measure the size of one json record so the second record triggers the size rotation
  size_t estimated_json_size = 0;
  {
    FileSinkConfig cfg;
    cfg.set_open_mode('w');

    JsonFileSink json_sink{estimate_filename, cfg, FileEventNotifier{}};
    write_json_record(json_sink, payload_a, record_ab_timestamp);
    json_sink.flush_sink();

    estimated_json_size = static_cast<size_t>(fs::file_size(estimate_filename));
  }

  testing::remove_file(estimate_filename);

  bool fail_open{false};
  FileEventNotifier file_event_notifier;
  file_event_notifier.after_open = [&fail_open](fs::path const&, FileEventNotifierHandle)
  {
    if (fail_open)
    {
      QUILL_THROW(QuillError{"after_open failure during rotation"});
    }
  };

  {
    RotatingFileSinkConfig cfg;
    cfg.set_open_mode('w');
    cfg.set_rotation_max_file_size(estimated_json_size + 1);
    cfg.set_rotation_frequency_and_interval('H', 1);

    RotatingJsonFileSink sink{filename, cfg, file_event_notifier, start_time};

    // Record A writes normally
    write_json_record(sink, payload_a, record_ab_timestamp);

    // Record B triggers the size rotation, and the rotation fails with an exception after the
    // estimate already cached B's json message. B is lost; the backend catches and reports this
    fail_open = true;
    REQUIRE_THROWS_AS(write_json_record(sink, payload_b, record_ab_timestamp), QuillError);
    fail_open = false;

    // Record C triggers the time rotation branch, which skips the estimate. It must write C's
    // own json message and not replay B's cached one
    write_json_record(sink, payload_c, record_c_timestamp);
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);

  // The rotated file holds record A
  REQUIRE(testing::file_contains(file_contents_1, payload_a));

  // The current file holds record C's payload; B's stale payload must not appear anywhere
  REQUIRE(testing::file_contains(file_contents, payload_c));
  REQUIRE_FALSE(testing::file_contains(file_contents, payload_b));
  REQUIRE_FALSE(testing::file_contains(file_contents_1, payload_b));

  testing::remove_file(filename);
  testing::remove_file(filename_1);
#endif
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
  testing::remove_file(filename_4);
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
TEST_CASE("rotating_file_sink_date_append_recovers_previous_run_files")
{
  // In append mode with the Date naming scheme, rotated files left behind by previous runs,
  // including previous days, should count towards max_backup_files instead of accumulating
  // forever across restarts (#930)
  uint64_t const timestamp_20230612 = 1686528000000000000;

  fs::path const filename_old_1 =
    "rotating_file_sink_date_append_recovers_previous_run_files.20230610.log";
  fs::path const filename_old_2 =
    "rotating_file_sink_date_append_recovers_previous_run_files.20230611.log";
  fs::path const filename_rotated =
    "rotating_file_sink_date_append_recovers_previous_run_files.20230612.log";
  fs::path const filename_rotated_1 =
    "rotating_file_sink_date_append_recovers_previous_run_files.20230612.1.log";
  fs::path const filename = "rotating_file_sink_date_append_recovers_previous_run_files.log";

  // create files simulating the previous runs
  testing::create_file(filename_old_1, "Old [0]");
  testing::create_file(filename_old_2, "Old [1]");
  testing::create_file(filename, "Old [2]");

  REQUIRE(fs::exists(filename_old_1));
  REQUIRE(fs::exists(filename_old_2));
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
        cfg.set_max_backup_files(1);
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    // the recovered files exceed max_backup_files, the oldest one is removed on startup
    REQUIRE_FALSE(fs::exists(filename_old_1));
    REQUIRE(fs::exists(filename_old_2));
    REQUIRE(fs::exists(filename));

    // write some records to the file
    for (size_t i = 0; i < 2; ++i)
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

  // each rotation displaces the oldest recovered file, keeping max_backup_files enforced
  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_rotated));
  REQUIRE_FALSE(fs::exists(filename_old_1));
  REQUIRE_FALSE(fs::exists(filename_old_2));
  REQUIRE_FALSE(fs::exists(filename_rotated_1));

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [1]"), true);

  std::vector<std::string> const file_contents_rotated = testing::file_contents(filename_rotated);
  REQUIRE_EQ(testing::file_contains(file_contents_rotated, "Record [0]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_rotated);
  testing::remove_file(filename_rotated_1);
  testing::remove_file(filename_old_1);
  testing::remove_file(filename_old_2);
}

/***/
TEST_CASE("rotating_file_sink_dateandtime_append_recovers_previous_run_files")
{
  // In append mode with the DateAndTime naming scheme, rotated files left behind by previous
  // runs should count towards max_backup_files instead of accumulating forever across
  // restarts (#930)
  uint64_t const timestamp_20230612 = 1686538000000000000; // 20230612_024640
  uint64_t const timestamp_20230613 = 1686634400000000000;

  fs::path const filename_old_1 =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files.20230610_080000.1.log";
  fs::path const filename_old_2 =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files.20230610_080000.log";
  fs::path const filename_old_3 =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files.20230611_090000.log";
  fs::path const filename_rotated =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files.20230612_024640.log";
  fs::path const filename_rotated_1 =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files.20230612_024640.1.log";
  fs::path const filename = "rotating_file_sink_dateandtime_append_recovers_previous_run_files.log";

  // create files simulating the previous runs, within the same date_time suffix a greater
  // index means an older file
  testing::create_file(filename_old_1, "Old [0]");
  testing::create_file(filename_old_2, "Old [1]");
  testing::create_file(filename_old_3, "Old [2]");
  testing::create_file(filename, "Old [3]");

  REQUIRE(fs::exists(filename_old_1));
  REQUIRE(fs::exists(filename_old_2));
  REQUIRE(fs::exists(filename_old_3));
  REQUIRE(fs::exists(filename));

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
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    // the recovered files exceed max_backup_files, the oldest one is removed on startup
    REQUIRE_FALSE(fs::exists(filename_old_1));
    REQUIRE(fs::exists(filename_old_2));
    REQUIRE(fs::exists(filename_old_3));
    REQUIRE(fs::exists(filename));

    {
      std::string s{"Record [0]"};
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
      std::string s{"Record [1]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      // Add a big string to rotate the file
      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230613, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  // each rotation displaces the oldest recovered file, keeping max_backup_files enforced
  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_rotated));
  REQUIRE(fs::exists(filename_rotated_1));
  REQUIRE_FALSE(fs::exists(filename_old_1));
  REQUIRE_FALSE(fs::exists(filename_old_2));
  REQUIRE_FALSE(fs::exists(filename_old_3));

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [1]"), true);

  std::vector<std::string> const file_contents_rotated = testing::file_contents(filename_rotated);
  REQUIRE_EQ(testing::file_contains(file_contents_rotated, "Record [0]"), true);

  std::vector<std::string> const file_contents_rotated_1 = testing::file_contents(filename_rotated_1);
  REQUIRE_EQ(testing::file_contains(file_contents_rotated_1, "Old [3]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_rotated);
  testing::remove_file(filename_rotated_1);
  testing::remove_file(filename_old_1);
  testing::remove_file(filename_old_2);
  testing::remove_file(filename_old_3);
}

/***/
TEST_CASE("rotating_file_sink_dateandtime_append_recovers_previous_run_files_without_extension")
{
  // Extensionless base filenames still generate rotated names with suffixes that look like file
  // extensions to std::filesystem::path::extension(); recovery must parse them as rotation
  // suffixes instead of skipping them.
  uint64_t const timestamp_20230612 = 1686538000000000000; // 20230612_024640
  uint64_t const timestamp_20230613 = 1686634400000000000;

  fs::path const filename_old_1 =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files_without_extension.1."
    "20230610_080000";
  fs::path const filename_old_2 =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files_without_extension.20230610_"
    "080000";
  fs::path const filename_old_3 =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files_without_extension.20230611_"
    "090000";
  fs::path const filename_rotated =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files_without_extension.20230612_"
    "024640";
  fs::path const filename_rotated_1 =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files_without_extension.1."
    "20230612_024640";
  fs::path const filename =
    "rotating_file_sink_dateandtime_append_recovers_previous_run_files_without_extension";

  testing::create_file(filename_old_1, "Old [0]");
  testing::create_file(filename_old_2, "Old [1]");
  testing::create_file(filename_old_3, "Old [2]");
  testing::create_file(filename, "Old [3]");

  REQUIRE(fs::exists(filename_old_1));
  REQUIRE(fs::exists(filename_old_2));
  REQUIRE(fs::exists(filename_old_3));
  REQUIRE(fs::exists(filename));

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
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    REQUIRE_FALSE(fs::exists(filename_old_1));
    REQUIRE(fs::exists(filename_old_2));
    REQUIRE(fs::exists(filename_old_3));
    REQUIRE(fs::exists(filename));

    {
      std::string s{"Record [0]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230612, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }

    {
      std::string s{"Record [1]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      std::string f;
      f.resize(1024);
      formatted_log_statement.append(f.data(), f.data() + f.size());

      rfh.write_log(nullptr, timestamp_20230613, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_rotated));
  REQUIRE(fs::exists(filename_rotated_1));
  REQUIRE_FALSE(fs::exists(filename_old_1));
  REQUIRE_FALSE(fs::exists(filename_old_2));
  REQUIRE_FALSE(fs::exists(filename_old_3));

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [1]"), true);

  std::vector<std::string> const file_contents_rotated = testing::file_contents(filename_rotated);
  REQUIRE_EQ(testing::file_contains(file_contents_rotated, "Record [0]"), true);

  std::vector<std::string> const file_contents_rotated_1 = testing::file_contents(filename_rotated_1);
  REQUIRE_EQ(testing::file_contains(file_contents_rotated_1, "Old [3]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_rotated);
  testing::remove_file(filename_rotated_1);
  testing::remove_file(filename_old_1);
  testing::remove_file(filename_old_2);
  testing::remove_file(filename_old_3);
}

/***/
TEST_CASE("rotating_file_sink_dateandtime_append_dont_overwrite_preserves_previous_run_files")
{
  // When overwrite_rolled_files is false, files recovered from previous runs must never be
  // deleted; instead the rotation stops and the current file keeps growing
  uint64_t const timestamp_20230612 = 1686538000000000000; // 20230612_024640

  fs::path const filename_old_1 =
    "rotating_file_sink_dateandtime_append_dont_overwrite_previous_run.20230610_080000.log";
  fs::path const filename_old_2 =
    "rotating_file_sink_dateandtime_append_dont_overwrite_previous_run.20230611_090000.log";
  fs::path const filename_rotated =
    "rotating_file_sink_dateandtime_append_dont_overwrite_previous_run.20230612_024640.log";
  fs::path const filename = "rotating_file_sink_dateandtime_append_dont_overwrite_previous_run.log";

  // create files simulating the previous runs
  testing::create_file(filename_old_1, "Old [0]");
  testing::create_file(filename_old_2, "Old [1]");
  testing::create_file(filename, "Old [2]");

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::DateAndTime);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_max_backup_files(1);
        cfg.set_overwrite_rolled_files(false);
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_20230612 / 1000000000))};

    // nothing is removed on startup when overwrite_rolled_files is false
    REQUIRE(fs::exists(filename_old_1));
    REQUIRE(fs::exists(filename_old_2));
    REQUIRE(fs::exists(filename));

    // write some records to the file
    for (size_t i = 0; i < 2; ++i)
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

  // the recovered files already exceed max_backup_files so the rotation stopped and everything
  // was appended to the current file, nothing was deleted
  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_old_1));
  REQUIRE(fs::exists(filename_old_2));
  REQUIRE_FALSE(fs::exists(filename_rotated));

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "Old [2]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [0]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents, "Record [1]"), true);

  std::vector<std::string> const file_contents_old_1 = testing::file_contents(filename_old_1);
  REQUIRE_EQ(testing::file_contains(file_contents_old_1, "Old [0]"), true);

  std::vector<std::string> const file_contents_old_2 = testing::file_contents(filename_old_2);
  REQUIRE_EQ(testing::file_contains(file_contents_old_2, "Old [1]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_old_1);
  testing::remove_file(filename_old_2);
  testing::remove_file(filename_rotated);
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

  // With interval=2 hours starting at hour 0:
  //   Initial rotation fires at +2h. Records [0],[1] go to first rotated file.
  //   Next rotation fires at +4h. Records [2],[3] go to second rotated file.
  //   Record [4] stays in the current file.
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [0]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Record [1]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [2]"), true);
  REQUIRE_EQ(testing::file_contains(file_contents_2, "Record [3]"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
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
  testing::remove_file(filename_4);
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
#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS(RotatingFileSink{filename, []()
                                  {
                                    RotatingFileSinkConfig cfg;
                                    cfg.set_rotation_max_file_size(128);
                                    cfg.set_open_mode('w');
                                    return cfg;
                                  }()});
#endif

  REQUIRE_FALSE(fs::exists(filename));

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS(RotatingFileSink{filename, []()
                                  {
                                    RotatingFileSinkConfig cfg;
                                    cfg.set_rotation_frequency_and_interval('Z', 123);
                                    cfg.set_open_mode('w');
                                    return cfg;
                                  }()});
#endif

  REQUIRE_FALSE(fs::exists(filename));

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS(RotatingFileSink{filename, []()
                                  {
                                    RotatingFileSinkConfig cfg;
                                    cfg.set_rotation_frequency_and_interval('M', 0);
                                    cfg.set_open_mode('w');
                                    return cfg;
                                  }()});
#endif

  REQUIRE_FALSE(fs::exists(filename));
}

/***/
TEST_CASE("rotating_file_sink_date_append_mode")
{
  fs::path const filename_base = "rotating_file_sink_date_append_mode.log";
  uint32_t iter{0};
  uint64_t const timestamp_20230612 = 1686528000000000000;

  auto make_sink = [&](uint64_t ts)
  {
    return RotatingFileSink{
      filename_base,
      []()
      {
        quill::RotatingFileSinkConfig cfg;
        cfg.set_open_mode('a');
        cfg.set_timezone(quill::Timezone::GmtTime);
        cfg.set_filename_append_option(quill::FilenameAppendOption::StartDate);
        cfg.set_rotation_max_file_size(512);
        cfg.set_max_backup_files(7);
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(ts / 1000000000))};
  };

  auto write_records = [&](RotatingFileSink& rfh, uint32_t iter_id)
  {
    for (size_t i = 0; i < 100; ++i)
    {
      std::string s{"Record [" + std::to_string(iter_id) + "][" + std::to_string(i) + "]"};
      std::string formatted_log_statement = s + "\n";
      rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  };

  auto read_all_records = [&](std::vector<fs::path> const& files)
  {
    std::vector<std::string> all;
    for (auto const& f : files)
    {
      if (fs::exists(f))
      {
        auto contents = testing::file_contents(f);
        all.insert(all.end(), contents.begin(), contents.end());
      }
    }
    return all;
  };

  auto filter_records = [&](std::vector<std::string> const& all, uint32_t iter_id)
  {
    std::vector<std::string> result;
    std::string prefix = "Record [" + std::to_string(iter_id) + "][";
    for (auto const& rec : all)
    {
      if (rec.rfind(prefix, 0) == 0) // starts with prefix
      {
        result.push_back(rec);
      }
    }
    return result;
  };

  auto verify_records_exist = [&](std::vector<std::string> const& records, uint32_t iter_id, uint32_t count)
  {
    REQUIRE_EQ(records.size(), count);
    for (uint32_t j = 0; j < count; ++j)
    {
      std::string expected_record =
        "Record [" + std::to_string(iter_id) + "][" + std::to_string(j) + "]";
      REQUIRE_EQ(std::count(records.begin(), records.end(), expected_record), 1);
    }
  };

  // Run 0
  {
    auto rfh = make_sink(timestamp_20230612);
    write_records(rfh, iter);
    iter++;
  }

  // Collect all existing files after run 0
  std::vector<fs::path> run0_files;
  std::string base_pattern = filename_base.stem().string() + "_20230612";

  // Add the main file
  fs::path main_file = base_pattern + ".log";
  if (fs::exists(main_file))
  {
    run0_files.push_back(main_file);
  }

  // Add numbered backup files that exist after run 0
  for (int i = 1; i <= 10; ++i)
  {
    fs::path backup_file = base_pattern + "." + std::to_string(i) + ".log";
    if (fs::exists(backup_file))
    {
      run0_files.push_back(backup_file);
    }
  }

  {
    auto all = read_all_records(run0_files);
    auto iter0 = filter_records(all, 0);

    REQUIRE_EQ(all.size(), 100);
    REQUIRE_EQ(iter0.size(), all.size());
  }

  // Run 1 (Recovery)
  {
    auto rfh = make_sink(timestamp_20230612);
    write_records(rfh, iter);
    iter++;
  }

  // Collect all existing files after run 1
  std::vector<fs::path> run1_files;
  std::string run1_pattern = filename_base.stem().string() + "_20230612";

  // Add the main file
  fs::path run1_main_file = run1_pattern + ".log";
  if (fs::exists(run1_main_file))
  {
    run1_files.push_back(run1_main_file);
  }

  // Add numbered backup files that exist after run 1
  for (int i = 1; i <= 10; ++i)
  { // Check more files than expected to be thorough
    fs::path backup_file = run1_pattern + "." + std::to_string(i) + ".log";
    if (fs::exists(backup_file))
    {
      run1_files.push_back(backup_file);
    }
  }

  {
    auto all = read_all_records(run1_files);
    auto iter0 = filter_records(all, 0);
    auto iter1 = filter_records(all, 1);

    REQUIRE_GE(iter1.size(), 100);
    REQUIRE_GE(all.size(), 200);
  }

  // Run 2
  {
    auto rfh = make_sink(timestamp_20230612);
    write_records(rfh, iter);
    iter++;
  }

  // Collect all existing files with the date pattern
  std::vector<fs::path> all_files;
  std::string run2_pattern = filename_base.stem().string() + "_20230612";

  // Add the main file
  fs::path run2_main_file = run2_pattern + ".log";
  if (fs::exists(run2_main_file))
  {
    all_files.push_back(run2_main_file);
  }

  // Add numbered backup files (check more than max_backup_files to catch any extra files)
  for (int i = 1; i <= 10; ++i)
  {
    fs::path backup_file = run2_pattern + "." + std::to_string(i) + ".log";
    if (fs::exists(backup_file))
    {
      all_files.push_back(backup_file);
    }
  }

  // Verify we don't exceed max_backup_files (7 + 1 main = 8 total max)
  fs::path filename_8 = run2_pattern + ".8.log";
  REQUIRE_FALSE(fs::exists(filename_8)); // max_backup_files = 7

  {
    auto all = read_all_records(all_files);

    // Due to backup file limit (7), older files may be deleted.
    auto iter0 = filter_records(all, 0);
    auto iter1 = filter_records(all, 1);
    auto iter2 = filter_records(all, 2);

    // The most recent iterations (1 and 2) should be complete
    verify_records_exist(iter1, 1, 100);
    verify_records_exist(iter2, 2, 100);

    // Records from iteration 0 may be partially lost due to backup file limit
    // but iteration 2 (most recent) should always be complete
    REQUIRE_GE(all.size(), 250);
    REQUIRE_LE(all.size(), 300);
  }

  // Clean up all files with the date pattern, regardless of test success/failure
  std::string final_cleanup_pattern = filename_base.stem().string() + "_20230612";
  fs::path final_cleanup_main = final_cleanup_pattern + ".log";
  testing::remove_file(final_cleanup_main);

  for (int i = 1; i <= 15; ++i)
  {
    fs::path final_backup_file = final_cleanup_pattern + "." + std::to_string(i) + ".log";
    testing::remove_file(final_backup_file);
  }
}

/***/
TEST_CASE("rotating_file_sink_rotation_on_creation_enabled")
{
  // Test that when rotation_on_creation is enabled, an existing non-empty file gets rotated on creation
  fs::path const filename = "rotating_file_sink_rotation_on_creation_enabled.log";
  fs::path const filename_1 = "rotating_file_sink_rotation_on_creation_enabled.1.log";

  // First, create a file with some content
  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_rotation_on_creation(false);
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    std::string s{"Initial Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  REQUIRE(fs::exists(filename));
  REQUIRE_FALSE(fs::exists(filename_1));

  // Now create a new sink with rotation_on_creation enabled
  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_rotation_on_creation(true);
                                  cfg.set_open_mode('a');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    // The existing file should have been rotated to .1
    REQUIRE(fs::exists(filename_1));

    // Write a new record to the main file
    std::string s{"New Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  // Verify the rotated file contains the initial record
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Initial Record"), true);

  // Verify the main file contains the new record
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "New Record"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
}

/***/
TEST_CASE("rotating_file_sink_rotation_on_creation_with_date_naming")
{
  // Test rotation_on_creation with Date naming scheme
  fs::path const filename = "rotating_file_sink_rotation_on_creation_date.log";
  fs::path const filename_dated = "rotating_file_sink_rotation_on_creation_date.20230612.log";

  std::chrono::system_clock::time_point const start_time =
    std::chrono::system_clock::time_point{std::chrono::seconds{1686528000}}; // 2023-06-12 00:00:00

  // First, create a file with some content
  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_timezone(quill::Timezone::GmtTime);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_rotation_on_creation(false);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{}, start_time};

    std::string s{"Initial Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  REQUIRE(fs::exists(filename));
  REQUIRE_FALSE(fs::exists(filename_dated));

  // Now create a new sink with rotation_on_creation enabled
  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_timezone(quill::Timezone::GmtTime);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_rotation_on_creation(true);
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{}, start_time};

    // The existing file should have been rotated with date suffix
    REQUIRE(fs::exists(filename_dated));
    REQUIRE(fs::exists(filename));

    // Write a new record to the main file
    std::string s{"New Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  // Verify the rotated file contains the initial record
  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_dated);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Initial Record"), true);

  // Verify the main file contains the new record
  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "New Record"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_dated);
}

TEST_CASE("rotating_file_sink_cleanup_preserves_unrelated_prefix_files")
{
  fs::path const dir = "rotating_file_sink_cleanup_preserves_unrelated_prefix_files";
  fs::path const filename = dir / "app.log";
  fs::path const rotated_filename = dir / "app.1.log";
  fs::path const unrelated_filename = dir / "app.backup.log";
  fs::path const unrelated_numeric_prefix_filename = dir / "app.123.backup.log";
  fs::path const unrelated_alt_suffix_filename = dir / "app.not.log";

  std::error_code ec;
  fs::remove_all(dir, ec);
  REQUIRE_FALSE(ec);
  fs::create_directories(dir, ec);
  REQUIRE_FALSE(ec);

  testing::create_file(rotated_filename, "previous rotated file\n");
  testing::create_file(unrelated_filename, "must survive startup cleanup\n");
  testing::create_file(unrelated_numeric_prefix_filename, "must also survive startup cleanup\n");
  testing::create_file(unrelated_alt_suffix_filename,
                       "must also survive alternate suffix startup cleanup\n");

  REQUIRE(fs::exists(rotated_filename));
  REQUIRE(fs::exists(unrelated_filename));
  REQUIRE(fs::exists(unrelated_numeric_prefix_filename));
  REQUIRE(fs::exists(unrelated_alt_suffix_filename));

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Index);
        cfg.set_remove_old_files(true);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{}};

    REQUIRE(fs::exists(filename));
  }

  REQUIRE_FALSE(fs::exists(rotated_filename));
  REQUIRE(fs::exists(unrelated_filename));
  REQUIRE(fs::exists(unrelated_numeric_prefix_filename));
  REQUIRE(fs::exists(unrelated_alt_suffix_filename));

  fs::remove_all(dir, ec);
  REQUIRE_FALSE(ec);
}

TEST_CASE("rotating_file_sink_date_cleanup_preserves_unrelated_prefix_files")
{
  fs::path const dir = "rotating_file_sink_date_cleanup_preserves_unrelated_prefix_files";
  fs::path const filename = dir / "app.log";
  fs::path const rotated_filename = dir / "app.20230612.log";
  fs::path const rotated_indexed_filename = dir / "app.20230612.1.log";
  fs::path const unrelated_filename = dir / "app.backup.log";
  fs::path const unrelated_date_like_filename = dir / "app.20230612.backup.log";
  fs::path const unrelated_alt_suffix_filename = dir / "app.not.log";

  std::error_code ec;
  fs::remove_all(dir, ec);
  REQUIRE_FALSE(ec);
  fs::create_directories(dir, ec);
  REQUIRE_FALSE(ec);

  testing::create_file(rotated_filename, "previous rotated file\n");
  testing::create_file(rotated_indexed_filename, "previous indexed rotated file\n");
  testing::create_file(unrelated_filename, "must survive startup cleanup\n");
  testing::create_file(unrelated_date_like_filename, "must also survive startup cleanup\n");
  testing::create_file(unrelated_alt_suffix_filename,
                       "must also survive alternate suffix startup cleanup\n");

  REQUIRE(fs::exists(rotated_filename));
  REQUIRE(fs::exists(rotated_indexed_filename));
  REQUIRE(fs::exists(unrelated_filename));
  REQUIRE(fs::exists(unrelated_date_like_filename));
  REQUIRE(fs::exists(unrelated_alt_suffix_filename));

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
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds{1686528000})};

    REQUIRE(fs::exists(filename));
  }

  REQUIRE_FALSE(fs::exists(rotated_filename));
  REQUIRE_FALSE(fs::exists(rotated_indexed_filename));
  REQUIRE(fs::exists(unrelated_filename));
  REQUIRE(fs::exists(unrelated_date_like_filename));
  REQUIRE(fs::exists(unrelated_alt_suffix_filename));

  fs::remove_all(dir, ec);
  REQUIRE_FALSE(ec);
}

TEST_CASE("rotating_file_sink_partial_rename_failure_corrupts_metadata")
{
  // After a partial rotation failure (some renames succeed, one fails midway),
  // _created_files metadata is left inconsistent: entries for successfully-renamed
  // files have updated indices, but the rotation was aborted. The metadata no
  // longer matches the expected pre-rotation state.
  fs::path const filename = "partial_rename_failure_metadata.log";
  fs::path const filename_1 = "partial_rename_failure_metadata.1.log";
  fs::path const filename_2 = "partial_rename_failure_metadata.2.log";
  fs::path const filename_3 = "partial_rename_failure_metadata.3.log";

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);

  auto rfh = PartialFailingRenameRotatingFileSink{filename,
                                                  []()
                                                  {
                                                    RotatingFileSinkConfig cfg;
                                                    cfg.set_rotation_max_file_size(1024);
                                                    cfg.set_open_mode('w');
                                                    return cfg;
                                                  }(),
                                                  FileEventNotifier{}};

  auto write_record = [&](std::string const& msg)
  {
    std::string formatted_log_statement;
    formatted_log_statement.append(msg);
    std::string padding;
    padding.resize(1024);
    formatted_log_statement.append(padding);
    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  };

  // Create 3 rotated files: base.log, base.1.log, base.2.log
  write_record("Record_A");
  write_record("Record_B");
  write_record("Record_C");

  REQUIRE_EQ(rfh.created_files().size(), 3);
  REQUIRE_EQ(rfh.created_files()[0].index, 0);
  REQUIRE_EQ(rfh.created_files()[1].index, 1);
  REQUIRE_EQ(rfh.created_files()[2].index, 2);

  // Fail the 2nd rename call during the next rotation.
  // Loop iterates rbegin->rend:
  //   1st rename: .2 -> .3 (succeeds)
  //   2nd rename: .1 -> .2 (FAILS, loop breaks)
  // Rotation aborted: completed renames are undone (.3 -> .2), base.log reopened.
  rfh.set_fail_on_call(2);
  rfh.reset_rename_count();
  write_record("Record_D");

  // After the fix, the undo logic restores .3 back to .2, so on-disk state
  // is unchanged from before the failed rotation (except base.log has Record_D appended).
  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_1));
  REQUIRE(fs::exists(filename_2));
  REQUIRE_FALSE(fs::exists(filename_3));

  REQUIRE_EQ(rfh.created_files().size(), 3);

  // Metadata should be unchanged from pre-rotation state since the rotation was rolled back.
  CHECK_EQ(rfh.created_files()[0].index, 0);
  CHECK_EQ(rfh.created_files()[1].index, 1);
  CHECK_EQ(rfh.created_files()[2].index, 2);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
  testing::remove_file(filename_3);
}

/***/
TEST_CASE("time_rotation_initial_rotation_respects_interval_hours")
{
  // Start at 2023-06-12 00:00:00 UTC
  uint64_t const timestamp_start = 1686528000000000000;

  fs::path const filename = "time_rotation_initial_respects_interval_hours.log";
  fs::path const filename_rotated = "time_rotation_initial_respects_interval_hours.20230612.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_frequency_and_interval('h', 6);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_start / 1000000000))};

    uint64_t timestamp = timestamp_start;

    // Write logs at +0h, +1h, +2h, +3h, +4h, +5h — all should stay in the same file
    // because the interval is 6 hours. The first rotation should happen at +6h.
    for (size_t i = 0; i < 6; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      rfh.write_log(nullptr, timestamp, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);

      timestamp += std::chrono::nanoseconds(std::chrono::hours(1)).count();
    }

    // Now write at +6h — this should trigger the first rotation
    {
      std::string s{"Record [6]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      rfh.write_log(nullptr, timestamp, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  // Records 0-5 should be in the rotated file, Record 6 in the current file
  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_rotated));

  std::vector<std::string> const rotated_contents = testing::file_contents(filename_rotated);
  REQUIRE_EQ(testing::file_contains(rotated_contents, "Record [0]"), true);
  REQUIRE_EQ(testing::file_contains(rotated_contents, "Record [5]"), true);

  std::vector<std::string> const current_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(current_contents, "Record [6]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_rotated);
}

/***/
TEST_CASE("time_rotation_initial_rotation_respects_interval_minutes")
{
  // Start at 2023-06-12 00:00:00 UTC
  uint64_t const timestamp_start = 1686528000000000000;

  fs::path const filename = "time_rotation_initial_respects_interval_minutes.log";
  fs::path const filename_rotated = "time_rotation_initial_respects_interval_minutes.20230612.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_frequency_and_interval('m', 5);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_start / 1000000000))};

    uint64_t timestamp = timestamp_start;

    // Write logs at +0m, +1m, +2m, +3m, +4m — all should stay in the same file
    // because the interval is 5 minutes.
    for (size_t i = 0; i < 5; ++i)
    {
      std::string s{"Record [" + std::to_string(i) + "]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      rfh.write_log(nullptr, timestamp, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);

      timestamp += std::chrono::nanoseconds(std::chrono::minutes(1)).count();
    }

    // Now write at +5m — this should trigger the first rotation
    {
      std::string s{"Record [5]"};
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());

      rfh.write_log(nullptr, timestamp, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    }
  }

  // Records 0-4 should be in the rotated file, Record 5 in the current file
  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_rotated));

  std::vector<std::string> const rotated_contents = testing::file_contents(filename_rotated);
  REQUIRE_EQ(testing::file_contains(rotated_contents, "Record [0]"), true);
  REQUIRE_EQ(testing::file_contains(rotated_contents, "Record [4]"), true);

  std::vector<std::string> const current_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(current_contents, "Record [5]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_rotated);
}

/***/
TEST_CASE("time_rotation_interval_boundaries_stay_wall_clock_aligned")
{
  // Regression: a rotation triggered late (sparse logging past a missed boundary) used to
  // schedule the next boundary at record_timestamp + interval, permanently drifting all
  // subsequent boundaries off the wall-clock alignment of the initial rotation time point
  uint64_t const timestamp_start = 1686528000000000000; // 2023-06-12 00:00:00 UTC
  uint64_t const one_minute_ns =
    static_cast<uint64_t>(std::chrono::nanoseconds(std::chrono::minutes(1)).count());

  fs::path const filename = "time_rotation_interval_boundaries_aligned.log";
  fs::path const filename_rotated = "time_rotation_interval_boundaries_aligned.20230612.log";
  fs::path const filename_rotated_1 = "time_rotation_interval_boundaries_aligned.20230612.1.log";
  fs::path const filename_rotated_2 = "time_rotation_interval_boundaries_aligned.20230612.2.log";
  fs::path const filename_rotated_3 = "time_rotation_interval_boundaries_aligned.20230612.3.log";

  testing::remove_file(filename);
  testing::remove_file(filename_rotated);
  testing::remove_file(filename_rotated_1);
  testing::remove_file(filename_rotated_2);
  testing::remove_file(filename_rotated_3);

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_frequency_and_interval('m', 5);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(timestamp_start / 1000000000))};

    auto write_record = [&rfh](std::string const& s, uint64_t timestamp)
    {
      std::string formatted_log_statement;
      formatted_log_statement.append(s.data(), s.data() + s.size());
      rfh.write_log(nullptr, timestamp, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
    };

    // Boundaries are aligned at +5m, +10m, +15m, ...
    write_record("Record [0]", timestamp_start);

    // +7m is 2 minutes past the missed +5m boundary and triggers the first rotation. The next
    // boundary must stay at +10m; the drifting behaviour would schedule it at +12m instead
    write_record("Record [1]", timestamp_start + (7 * one_minute_ns));

    // +11m is past the aligned +10m boundary and must trigger the second rotation. With the
    // drifted +12m boundary it would stay in the same file. The next boundary becomes +15m
    write_record("Record [2]", timestamp_start + (11 * one_minute_ns));

    // +27m is more than two whole intervals past the missed +15m boundary. This triggers
    // exactly one rotation (empty windows do not produce empty files) and the next boundary
    // jumps to the next aligned multiple after the record: +30m, not +32m
    write_record("Record [3]", timestamp_start + (27 * one_minute_ns));

    // +29m is before the +30m boundary and must not rotate; it stays with Record [3]
    write_record("Record [4]", timestamp_start + (29 * one_minute_ns));

    // +31m is past the aligned +30m boundary and rotates again
    write_record("Record [5]", timestamp_start + (31 * one_minute_ns));
  }

  REQUIRE(fs::exists(filename));
  REQUIRE(fs::exists(filename_rotated));
  REQUIRE(fs::exists(filename_rotated_1));
  REQUIRE(fs::exists(filename_rotated_2));
  REQUIRE(fs::exists(filename_rotated_3));

  std::vector<std::string> const rotated_contents_3 = testing::file_contents(filename_rotated_3);
  REQUIRE_EQ(testing::file_contains(rotated_contents_3, "Record [0]"), true);

  std::vector<std::string> const rotated_contents_2 = testing::file_contents(filename_rotated_2);
  REQUIRE_EQ(testing::file_contains(rotated_contents_2, "Record [1]"), true);

  std::vector<std::string> const rotated_contents_1 = testing::file_contents(filename_rotated_1);
  REQUIRE_EQ(testing::file_contains(rotated_contents_1, "Record [2]"), true);

  std::vector<std::string> const rotated_contents = testing::file_contents(filename_rotated);
  REQUIRE_EQ(testing::file_contains(rotated_contents, "Record [3]"), true);
  REQUIRE_EQ(testing::file_contains(rotated_contents, "Record [4]"), true);

  std::vector<std::string> const current_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(current_contents, "Record [5]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_rotated);
  testing::remove_file(filename_rotated_1);
  testing::remove_file(filename_rotated_2);
  testing::remove_file(filename_rotated_3);
}

#ifdef QUILL_ENABLE_EXTENSIVE_TESTS
TEST_CASE("time_rotation_daily_at_time_rotating_file_sink_localtime_dst")
{
  std::string previous_tz;
  bool had_previous_tz = false;

  #if defined(_WIN32)
  char* current_tz = nullptr;
  size_t current_tz_len = 0;
  if ((_dupenv_s(&current_tz, &current_tz_len, "TZ") == 0) && current_tz)
  {
    previous_tz = current_tz;
    had_previous_tz = true;
  }
  std::free(current_tz);
  #else
  if (char const* current_tz = std::getenv("TZ"))
  {
    previous_tz = current_tz;
    had_previous_tz = true;
  }
  #endif

  #if defined(_WIN32)
  _putenv_s("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2");
  _tzset();
  #else
  setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
  tzset();
  #endif

  // Hardcoded local timestamps in EST/EDT:
  // 2025-03-08 00:30:00 EST
  uint64_t constexpr start_timestamp = 1741411800000000000ull;
  // 2025-03-08 07:30:00 EST
  uint64_t constexpr before_rotation_timestamp = 1741437000000000000ull;
  // 2025-03-08 08:05:00 EST
  uint64_t constexpr first_post_threshold_timestamp = 1741439100000000000ull;
  // 2025-03-09 08:05:00 EDT
  uint64_t constexpr next_day_post_threshold_timestamp = 1741521900000000000ull;

  auto const elapsed_between_days = std::chrono::duration_cast<std::chrono::hours>(
    std::chrono::nanoseconds{next_day_post_threshold_timestamp - first_post_threshold_timestamp});
  CHECK_EQ(elapsed_between_days, std::chrono::hours{23});

  fs::path const filename = "time_rotation_daily_at_time_rotating_file_sink_localtime_dst.log";
  fs::path const filename_1 =
    "time_rotation_daily_at_time_rotating_file_sink_localtime_dst.20250308.log";
  fs::path const filename_2 =
    "time_rotation_daily_at_time_rotating_file_sink_localtime_dst.20250308.1.log";

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_time_daily("08:00");
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_timezone(Timezone::LocalTime);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{},
      std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>{
          std::chrono::nanoseconds{start_timestamp}})};

    auto write_record = [&rfh](uint64_t ts, std::string const& message)
    {
      rfh.write_log(nullptr, ts, std::string_view{}, std::string_view{}, std::string{},
                    std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", message);
    };

    write_record(before_rotation_timestamp, "Record [before rotation]");
    write_record(first_post_threshold_timestamp, "Record [first post-threshold]");
    write_record(next_day_post_threshold_timestamp, "Record [next day post-threshold]");
  }

  CHECK(fs::exists(filename));
  CHECK(fs::exists(filename_1));
  CHECK(fs::exists(filename_2));

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  CHECK_EQ(testing::file_contains(file_contents, "Record [next day post-threshold]"), true);

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  CHECK_EQ(testing::file_contains(file_contents_1, "Record [first post-threshold]"), true);

  std::vector<std::string> const file_contents_2 = testing::file_contents(filename_2);
  CHECK_EQ(testing::file_contains(file_contents_2, "Record [before rotation]"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::remove_file(filename_2);

  #if defined(_WIN32)
  if (had_previous_tz)
  {
    _putenv_s("TZ", previous_tz.c_str());
  }
  else
  {
    _putenv_s("TZ", "");
  }
  _tzset();
  #else
  if (had_previous_tz)
  {
    setenv("TZ", previous_tz.c_str(), 1);
  }
  else
  {
    unsetenv("TZ");
  }
  tzset();
  #endif
}
#endif

/***/
TEST_CASE("rotating_file_sink_rotation_on_creation_enabled_write_mode")
{
  fs::path const filename = "rotating_file_sink_rotation_on_creation_write.log";
  fs::path const filename_1 = "rotating_file_sink_rotation_on_creation_write.1.log";

  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_rotation_on_creation(false);
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    std::string s{"Initial Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  REQUIRE(fs::exists(filename));
  REQUIRE_FALSE(fs::exists(filename_1));

  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_rotation_on_creation(true);
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    REQUIRE(fs::exists(filename_1));

    std::string s{"New Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_1);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Initial Record"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "New Record"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
}

/***/
TEST_CASE("rotating_file_sink_rotation_on_creation_empty_file_does_not_rotate")
{
  fs::path const filename = "rotating_file_sink_rotation_on_creation_empty.log";
  fs::path const filename_1 = "rotating_file_sink_rotation_on_creation_empty.1.log";

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::create_file(filename);

  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_rotation_on_creation(true);
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    REQUIRE(fs::exists(filename));
    REQUIRE_FALSE(fs::exists(filename_1));

    std::string s{"New Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  REQUIRE_FALSE(fs::exists(filename_1));

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "New Record"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_1);
}

/***/
TEST_CASE(
  "rotating_file_sink_rotation_on_creation_write_mode_without_backup_slots_preserves_existing_file")
{
  fs::path const filename =
    "rotating_file_sink_rotation_on_creation_write_mode_without_backup_slots.log";

  testing::remove_file(filename);
  testing::create_file(filename, "Initial Record");

  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_rotation_on_creation(true);
                                  cfg.set_max_backup_files(0);
                                  cfg.set_overwrite_rolled_files(false);
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};
  }

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 1);
  REQUIRE_EQ(file_contents[0], "Initial Record");

  testing::remove_file(filename);
}

/***/
TEST_CASE(
  "rotating_file_sink_rotation_on_creation_append_mode_without_backup_slots_preserves_and_appends")
{
  fs::path const filename =
    "rotating_file_sink_rotation_on_creation_append_mode_without_backup_slots.log";
  fs::path const filename_1 =
    "rotating_file_sink_rotation_on_creation_append_mode_without_backup_slots.1.log";

  testing::remove_file(filename);
  testing::remove_file(filename_1);
  testing::create_file(filename, "Initial Record\n");

  {
    auto rfh = RotatingFileSink{filename,
                                []()
                                {
                                  RotatingFileSinkConfig cfg;
                                  cfg.set_rotation_max_file_size(1024);
                                  cfg.set_rotation_on_creation(true);
                                  cfg.set_max_backup_files(0);
                                  cfg.set_overwrite_rolled_files(false);
                                  cfg.set_open_mode('a');
                                  return cfg;
                                }(),
                                FileEventNotifier{}};

    std::string s{"New Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  REQUIRE_FALSE(fs::exists(filename_1));

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 2);
  REQUIRE_EQ(file_contents[0], "Initial Record");
  REQUIRE_EQ(file_contents[1], "New Record");

  testing::remove_file(filename);
  testing::remove_file(filename_1);
}

/***/
TEST_CASE("rotating_file_sink_rotation_on_creation_with_date_naming_write_mode")
{
  fs::path const filename = "rotating_file_sink_rotation_on_creation_date_write.log";
  fs::path const filename_dated = "rotating_file_sink_rotation_on_creation_date_write.20230612.log";

  std::chrono::system_clock::time_point const start_time =
    std::chrono::system_clock::time_point{std::chrono::seconds{1686528000}}; // 2023-06-12 00:00:00

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_timezone(quill::Timezone::GmtTime);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_rotation_on_creation(false);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{}, start_time};

    std::string s{"Initial Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  REQUIRE(fs::exists(filename));
  REQUIRE_FALSE(fs::exists(filename_dated));

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_timezone(quill::Timezone::GmtTime);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Date);
        cfg.set_rotation_on_creation(true);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{}, start_time};

    REQUIRE(fs::exists(filename_dated));
    REQUIRE(fs::exists(filename));

    std::string s{"New Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_dated);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Initial Record"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "New Record"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_dated);
}

/***/
TEST_CASE("rotating_file_sink_rotation_on_creation_with_dateandtime_naming_write_mode")
{
  fs::path const filename = "rotating_file_sink_rotation_on_creation_datetime_write.log";
  fs::path const filename_dated =
    "rotating_file_sink_rotation_on_creation_datetime_write.20230612_000521.log";

  std::chrono::system_clock::time_point const start_time =
    std::chrono::system_clock::time_point{std::chrono::seconds{1686528321}}; // 2023-06-12 00:05:21

  testing::remove_file(filename);
  testing::remove_file(filename_dated);

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::DateAndTime);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_rotation_on_creation(false);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{}, start_time};

    std::string s{"Initial Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  REQUIRE(fs::exists(filename));
  REQUIRE_FALSE(fs::exists(filename_dated));

  {
    auto rfh = RotatingFileSink{
      filename,
      []()
      {
        RotatingFileSinkConfig cfg;
        cfg.set_rotation_max_file_size(1024);
        cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::DateAndTime);
        cfg.set_timezone(Timezone::GmtTime);
        cfg.set_rotation_on_creation(true);
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{}, start_time};

    REQUIRE(fs::exists(filename_dated));
    REQUIRE(fs::exists(filename));

    std::string s{"New Record"};
    std::string formatted_log_statement;
    formatted_log_statement.append(s.data(), s.data() + s.size());

    rfh.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                  LogLevel::Info, "INFO", "I", nullptr, "", formatted_log_statement);
  }

  std::vector<std::string> const file_contents_1 = testing::file_contents(filename_dated);
  REQUIRE_EQ(testing::file_contains(file_contents_1, "Initial Record"), true);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(testing::file_contains(file_contents, "New Record"), true);

  testing::remove_file(filename);
  testing::remove_file(filename_dated);
}

/***/
TEST_CASE("rotating_file_sink_reopen_failure_does_not_crash")
{
#if defined(QUILL_NO_EXCEPTIONS)
  // This test deliberately forces open_file() to throw via a before_open callback to
  // simulate a disk-full reopen failure, so it requires exceptions-enabled builds.
  return;
#else
  fs::path const log_dir = "rotating_file_sink_reopen_failure";
  fs::path const filename = log_dir / "rotating_file_sink_reopen_failure.log";

  std::error_code ec;
  fs::remove_all(log_dir, ec);
  fs::create_directories(log_dir, ec);
  REQUIRE_FALSE(ec);

  // Fail only the second open_file() — the reopen that happens during the first
  // rotation — to leave the sink with no open file. Subsequent writes must recover
  // without crashing.
  size_t before_open_calls{0};
  FileEventNotifier file_event_notifier;
  file_event_notifier.before_open = [&before_open_calls](fs::path const&)
  {
    ++before_open_calls;
    if (before_open_calls == 2)
    {
      throw std::runtime_error{"simulated disk full during reopen"};
    }
  };

  RotatingFileSinkConfig cfg;
  cfg.set_rotation_max_file_size(512);
  cfg.set_open_mode('w');

  RotatingFileSink sink{filename, cfg, file_event_notifier};

  auto write = [&sink](std::string_view log_statement)
  {
    sink.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                   std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", log_statement);
  };

  write("Record [0]\n");

  std::string oversized_record = "Record [1]\n";
  oversized_record.append(600, 'x');

  // First oversized write triggers rotation. The reopen throws from before_open and
  // leaves the sink without an open file.
  try
  {
    write(oversized_record);
  }
  catch (std::exception const&)
  {
  }
  REQUIRE_EQ(before_open_calls, 2u);

  // Second oversized write re-enters rotation with a null file. Before the fix this
  // reached fsync_file() on a null handle and crashed with SIGSEGV. With the fix the
  // sink detects the missing handle, reopens the file (before_open call #3 now
  // succeeds), and the write goes through.
  write(oversized_record);
  REQUIRE_EQ(before_open_calls, 3u);

  // A third write should work normally — the sink is fully recovered.
  write("Record [2]\n");

  fs::remove_all(log_dir, ec);
#endif
}

/***/
TEST_CASE("rotating_file_sink_directory_scan_failure_preserves_existing_backups")
{
#if defined(_WIN32)
  // Removing directory-list permission portably requires ACL manipulation on Windows.
  return;
#else
  fs::path const log_dir = "rotating_file_sink_scan_failure";
  fs::path const filename = log_dir / "application.log";
  fs::path const backup_filename = log_dir / "application.1.log";

  std::error_code ec;
  fs::remove_all(log_dir, ec);
  fs::create_directories(log_dir, ec);
  REQUIRE_FALSE(ec);

  std::string const active_prefix(500, 'A');
  std::string const backup_sentinel{"existing backup must survive"};
  testing::create_file(filename, active_prefix);
  testing::create_file(backup_filename, backup_sentinel);

  struct DirectoryCleanup
  {
    explicit DirectoryCleanup(fs::path path) : path{std::move(path)} {}
    ~DirectoryCleanup()
    {
      restore_permissions();
      std::error_code remove_ec;
      fs::remove_all(path, remove_ec);
    }

    void restore_permissions() const noexcept { (void)::chmod(path.string().c_str(), 0700); }

    fs::path path;
  } directory_cleanup{log_dir};

  if (::chmod(log_dir.string().c_str(), 0300) != 0)
  {
    return;
  }

  // Root and some filesystems can still enumerate this directory; skip when the failure cannot
  // be reproduced on the current platform.
  std::error_code probe_ec;
  fs::directory_iterator probe{log_dir, probe_ec};
  (void)probe;
  if (!probe_ec)
  {
    return;
  }

  {
    RotatingFileSinkConfig cfg;
    cfg.set_rotation_max_file_size(512);
    cfg.set_open_mode('a');

    RotatingFileSink sink{filename, cfg, FileEventNotifier{}};
    std::string const appended_record(64, 'B');
    sink.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                   std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", appended_record);
  }

  directory_cleanup.restore_permissions();

  std::vector<std::string> const backup_contents = testing::file_contents(backup_filename);
  REQUIRE_EQ(backup_contents.size(), 1u);
  REQUIRE_EQ(backup_contents[0], backup_sentinel);

  std::vector<std::string> const active_contents = testing::file_contents(filename);
  REQUIRE_EQ(active_contents.size(), 1u);
  REQUIRE_EQ(active_contents[0], active_prefix + std::string(64, 'B'));
#endif
}

TEST_SUITE_END();
