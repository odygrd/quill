#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "misc/TestUtilities.h"

#include "quill/core/DynamicFormatArgStore.h"
#include "quill/sinks/FileSink.h"

#if !defined(_WIN32)
  #include <unistd.h>
#endif

TEST_SUITE_BEGIN("FileSink");

using namespace quill;
using namespace quill::detail;

namespace
{
class FileSinkTestHarness : public FileSink
{
public:
  using FileSink::FileSink;

  void open_file_for_test(fs::path const& filename, std::string const& mode)
  {
    FileSink::open_file(filename, mode);
  }

  void close_file_for_test() { FileSink::close_file(); }

  void fsync_file_for_test(bool force_fsync) { FileSink::fsync_file(force_fsync); }
  bool write_occurred_for_test() const noexcept { return _write_occurred; }
  std::chrono::steady_clock::time_point last_fsync_timestamp_for_test() const noexcept
  {
    return _last_fsync_timestamp;
  }

#if defined(_WIN32)
  FileEventNotifierHandle file_handle() const noexcept { return _native_file_handle; }
#else
  FileEventNotifierHandle file_handle() const noexcept { return _file; }

  void close_underlying_fd_for_test()
  {
    REQUIRE_NE(_file, nullptr);
    // fileno is unqualified because BSD libcs define it as a function-like macro
    REQUIRE_EQ(::close(fileno(_file)), 0);
  }

  void close_stream_after_fd_closed_for_test()
  {
    REQUIRE_NE(_file, nullptr);
    std::fclose(_file);
    _file = nullptr;
  }
#endif

  static FileEventNotifierHandle closed_file_handle() noexcept
  {
#if defined(_WIN32)
    return INVALID_HANDLE_VALUE;
#else
    return nullptr;
#endif
  }
};
} // namespace

#if defined(__linux__)
namespace
{
size_t count_open_fds_for_path(fs::path const& filename)
{
  size_t count{0};
  fs::path const normalized_path = detail::normalize_file_sink_path(filename, false);

  for (auto const& fd_entry : fs::directory_iterator{"/proc/self/fd"})
  {
    std::error_code ec;
    fs::path const target = fs::read_symlink(fd_entry.path(), ec);
    if (!ec && (target == normalized_path))
    {
      ++count;
    }
  }

  return count;
}
} // namespace
#endif

/***/
TEST_CASE("file_sink_config")
{
  FileSinkConfig fsc;
  fsc.set_write_buffer_size(8 * 1024);
  REQUIRE_EQ(fsc.write_buffer_size(), 8 * 1024);

  fsc.set_write_buffer_size(128);
  REQUIRE_EQ(fsc.write_buffer_size(), 4 * 1024);

  fsc.set_write_buffer_size(0);
  REQUIRE_EQ(fsc.write_buffer_size(), 0);

  REQUIRE_EQ(fsc.minimum_fsync_interval().count(), 0);
}

/***/
TEST_CASE("fsync_interval_should_throw_when_fsync_disabled")
{
  fs::path const filename = "fsync_interval_should_throw_when_fsync_disabled.log";

  FileSinkConfig fsc;
  fsc.set_filename_append_option(FilenameAppendOption::StartDate);
  fsc.set_timezone(Timezone::GmtTime);
  fsc.set_minimum_fsync_interval(std::chrono::seconds{10});
  fsc.set_fsync_enabled(false);

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS(FileSink{filename, fsc});
#endif
  REQUIRE_FALSE(fs::exists(filename));

  {
    // now enable fsync
    fsc.set_fsync_enabled(true);
    FileSink file_sink{filename, fsc};
  }

  testing::remove_file(filename);
}

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
TEST_CASE("append_custom_timestamp_to_file")
{
  uint64_t const timestamp_20230612 = 1686528321331324000;
  fs::path const filename = "append_custom_timestamp_to_file.log";
  fs::path const expected_filename = "append_custom_timestamp_to_file_0612.log";

  FileSinkConfig fsc;
  fsc.set_filename_append_option(FilenameAppendOption::StartCustomTimestampFormat, "_%m%d");
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

/***/
TEST_CASE("open_unicode_filename")
{
#if defined(_WIN32)
  fs::path const filename = L"unicode_quill_file_sink_Gr\u00fc\u00dfe_\u65e5\u672c\u8a9e.log";

  testing::remove_file(filename);

  FileSinkConfig fsc;
  fsc.set_open_mode('w');

  {
    FileSink file_sink{filename, fsc};
    file_sink.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                        LogLevel::Info, "INFO", "I", nullptr, "", "unicode filename test\n");
    file_sink.flush_sink();
  }

  REQUIRE(fs::exists(filename));

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 1);
  REQUIRE_EQ(file_contents[0], "unicode filename test");

  testing::remove_file(filename);
#else
  return;
#endif
}

/***/
TEST_CASE("dev_null_special_path")
{
  FileSinkTestHarness file_sink{"/dev/null"};
  REQUIRE(file_sink.is_null());

#if defined(_WIN32)
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());
#else
  REQUIRE_NE(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());
#endif

  file_sink.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{}, std::string_view{},
                      LogLevel::Info, "INFO", "I", nullptr, "", "dev null test\n");
  file_sink.flush_sink();
}

/***/
TEST_CASE("windows_append_mode_appends_after_external_writer")
{
#if defined(_WIN32)
  fs::path const filename = "windows_append_mode_appends_after_external_writer.log";
  testing::remove_file(filename);
  testing::create_file(filename, "seed\n");

  auto write_record = [](FileSink& file_sink, std::string_view message)
  {
    file_sink.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                        std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", message);
    file_sink.flush_sink();
  };

  FileSinkConfig fsc;
  fsc.set_open_mode('a');

  {
    FileSink file_sink{filename, fsc};
    write_record(file_sink, "first\n");

    HANDLE external_file_handle =
      ::CreateFileW(filename.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE_NE(external_file_handle, INVALID_HANDLE_VALUE);

    char const external_record[] = "external\n";
    DWORD bytes_written{0};
    REQUIRE(::WriteFile(external_file_handle, external_record,
                        static_cast<DWORD>(sizeof(external_record) - 1u), &bytes_written, nullptr));
    REQUIRE_EQ(bytes_written, static_cast<DWORD>(sizeof(external_record) - 1u));
    REQUIRE(::CloseHandle(external_file_handle));

    write_record(file_sink, "second\n");
  }

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 4);
  REQUIRE_EQ(file_contents[0], "seed");
  REQUIRE_EQ(file_contents[1], "first");
  REQUIRE_EQ(file_contents[2], "external");
  REQUIRE_EQ(file_contents[3], "second");

  testing::remove_file(filename);
#else
  return;
#endif
}

/***/
TEST_CASE("reopen_deleted_file_uses_configured_open_mode")
{
#if defined(__linux__)
  fs::path const filename = "reopen_deleted_file_uses_configured_open_mode.log";

  FileSinkConfig fsc;
  fsc.set_open_mode('a');

  bool recreate_on_next_close = false;
  FileEventNotifier file_event_notifier;
  file_event_notifier.after_close = [&recreate_on_next_close](fs::path const& file_path)
  {
    if (!recreate_on_next_close)
    {
      return;
    }

    recreate_on_next_close = false;
    testing::create_file(file_path, "recreated\n");
  };

  auto write_record = [](FileSink& file_sink, std::string_view message)
  {
    file_sink.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                        std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", message);
  };

  {
    FileSink file_sink{filename, fsc, file_event_notifier};

    write_record(file_sink, "first\n");
    file_sink.flush_sink();

    testing::remove_file(filename);
    REQUIRE_FALSE(fs::exists(filename));

    recreate_on_next_close = true;

    // This record is written to the deleted file handle. After close, the notifier recreates the
    // visible file before Quill reopens it.
    write_record(file_sink, "second\n");
    file_sink.flush_sink();

    write_record(file_sink, "third\n");
    file_sink.flush_sink();
  }

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 2);
  REQUIRE_EQ(file_contents[0], "recreated");
  REQUIRE_EQ(file_contents[1], "third");

  testing::remove_file(filename);
#else
  return;
#endif
}

/***/
TEST_CASE("after_open_throw_does_not_leak_file_descriptor_during_construction")
{
#if defined(__linux__)
  fs::path const filename =
    "after_open_throw_does_not_leak_file_descriptor_during_construction.log";
  testing::remove_file(filename);

  FileEventNotifier file_event_notifier;
  file_event_notifier.after_open = [](fs::path const&, FileEventNotifierHandle)
  { QUILL_THROW(QuillError{"after_open failure"}); };

  REQUIRE_EQ(count_open_fds_for_path(filename), 0);
  #if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS((FileSink{filename, FileSinkConfig{}, file_event_notifier}), QuillError);
  #else
  (void)file_event_notifier;
  #endif
  REQUIRE_EQ(count_open_fds_for_path(filename), 0);

  testing::remove_file(filename);
#else
  return;
#endif
}

/***/
TEST_CASE("after_open_throw_leaves_sink_closed")
{
  fs::path const filename = "after_open_throw_leaves_sink_closed.log";
  testing::remove_file(filename);

  FileEventNotifier file_event_notifier;
  file_event_notifier.after_open = [](fs::path const&, FileEventNotifierHandle)
  { QUILL_THROW(QuillError{"after_open failure"}); };

  FileSinkTestHarness file_sink{filename, FileSinkConfig{}, file_event_notifier, false};
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());
#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(file_sink.open_file_for_test(file_sink.get_filename(), "a"), QuillError);
#endif
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());

  testing::remove_file(filename);
}

/***/
TEST_CASE("after_open_throw_during_reopen_leaves_sink_closed")
{
  fs::path const filename = "after_open_throw_during_reopen_leaves_sink_closed.log";
  testing::remove_file(filename);

  uint32_t after_open_count{0};
  FileEventNotifier file_event_notifier;
  file_event_notifier.after_open = [&after_open_count](fs::path const&, FileEventNotifierHandle)
  {
    ++after_open_count;
    if (after_open_count == 2)
    {
      QUILL_THROW(QuillError{"after_open failure on reopen"});
    }
  };

  FileSinkTestHarness file_sink{filename, FileSinkConfig{}, file_event_notifier, false};
  file_sink.open_file_for_test(file_sink.get_filename(), "a");
  REQUIRE_NE(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());

  file_sink.close_file_for_test();
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(file_sink.open_file_for_test(file_sink.get_filename(), "a"), QuillError);
#endif
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());

  // fsync_file() must tolerate the closed handle state left behind by a failed reopen.
  // Previously this dereferenced a null FILE* / invalid HANDLE and crashed.
  file_sink.fsync_file_for_test(true);
  file_sink.fsync_file_for_test(false);
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());

  testing::remove_file(filename);
}

/***/
TEST_CASE("destructor_close_notifier_exception_does_not_escape")
{
#if defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  fs::path const filename = "destructor_close_notifier_exception_does_not_escape.log";
  testing::remove_file(filename);

  bool before_close_called{false};
  FileEventNotifier file_event_notifier;
  file_event_notifier.before_close = [&before_close_called](fs::path const&, FileEventNotifierHandle)
  {
    before_close_called = true;
    QUILL_THROW(QuillError{"before_close failure"});
  };

  {
    FileSink file_sink{filename, FileSinkConfig{}, file_event_notifier};
  }

  REQUIRE(before_close_called);
  #if defined(__linux__)
  REQUIRE_EQ(count_open_fds_for_path(filename), 0u);
  #endif
  testing::remove_file(filename);
#endif
}

/***/
TEST_CASE("fsync_reports_os_failure")
{
#if defined(_WIN32) || defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  fs::path const filename = "fsync_reports_os_failure.log";
  testing::remove_file(filename);

  FileSinkConfig cfg;
  cfg.set_fsync_enabled(true);

  FileSinkTestHarness file_sink{filename, cfg, FileEventNotifier{}, false};
  file_sink.open_file_for_test(file_sink.get_filename(), "a");
  auto const last_fsync_timestamp_before_failure = file_sink.last_fsync_timestamp_for_test();
  file_sink.close_underlying_fd_for_test();

  REQUIRE_THROWS_AS(file_sink.fsync_file_for_test(false), QuillError);
  REQUIRE(file_sink.write_occurred_for_test());
  REQUIRE_EQ(file_sink.last_fsync_timestamp_for_test(), last_fsync_timestamp_before_failure);

  file_sink.close_stream_after_fd_closed_for_test();
  testing::remove_file(filename);
#endif
}

/***/
TEST_CASE("close_file_reports_fclose_failure")
{
#if defined(_WIN32) || defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  fs::path const filename = "close_file_reports_fclose_failure.log";
  testing::remove_file(filename);

  FileSinkTestHarness file_sink{filename, FileSinkConfig{}, FileEventNotifier{}, false};
  file_sink.open_file_for_test(file_sink.get_filename(), "a");
  file_sink.close_underlying_fd_for_test();

  REQUIRE_THROWS_AS(file_sink.close_file_for_test(), QuillError);
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());

  testing::remove_file(filename);
#endif
}

/***/
TEST_CASE("use_symlink_directory")
{
#if !defined(_WIN32)
  // Symlink test, currently for Linux only but can probably also expanded to windows later
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
#else
  return;
#endif
}

TEST_SUITE_END();
