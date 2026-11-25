#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "misc/TestUtilities.h"

#include "quill/core/DynamicFormatArgStore.h"
#include "quill/sinks/FileSink.h"

#include <chrono>
#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#if !defined(_WIN32)
  #include <sys/stat.h>
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
  bool fsync_pending_for_test() const noexcept { return _fsync_pending; }
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

/** The bytes of a file, without the newline translation of a text stream */
std::string read_file_bytes(fs::path const& filename)
{
  std::ifstream file{filename, std::ios::binary};
  return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

void write_log_statement(FileSink& file_sink, std::string_view log_statement)
{
  file_sink.write_log(LogRecord{nullptr,
                                0,
                                std::string_view{},
                                std::string_view{},
                                std::string{},
                                std::string_view{},
                                LogLevel::Info,
                                "INFO",
                                "I",
                                nullptr,
                                "",
                                log_statement,
                                {}});
}

void write_binary_record(FileSink& file_sink, std::string_view bytes, uint64_t timestamp = 0)
{
  file_sink.write_binary(BinaryRecord{nullptr, timestamp, std::string_view{}, std::string_view{},
                                      std::string_view{}, std::string_view{},
                                      reinterpret_cast<std::byte const*>(bytes.data()), bytes.size()});
}

/** The bytes of a value in the byte order of the machine, as a framed file has them */
template <typename T>
std::string bytes_of(T value)
{
  return std::string{reinterpret_cast<char const*>(&value), sizeof(value)};
}
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

  // Each write to a file costs more on Windows, so the default buffer is larger there
#if defined(_WIN32)
  REQUIRE_EQ(fsc.write_buffer_size(), 256u * 1024u);
#else
  REQUIRE_EQ(fsc.write_buffer_size(), 64u * 1024u);
#endif
  REQUIRE_EQ(fsc.write_buffer_size(), FileSinkConfig::default_write_buffer_size);

  fsc.set_write_buffer_size(8 * 1024);
  REQUIRE_EQ(fsc.write_buffer_size(), 8 * 1024);

  fsc.set_write_buffer_size(128);
  REQUIRE_EQ(fsc.write_buffer_size(), 4 * 1024);

  fsc.set_write_buffer_size(0);
  REQUIRE_EQ(fsc.write_buffer_size(), 0);

  REQUIRE_EQ(fsc.minimum_fsync_interval().count(), 0);

  fsc.set_open_mode('w');
  REQUIRE_EQ(fsc.open_mode(), "w");
  fsc.set_open_mode("ab");
  REQUIRE_EQ(fsc.open_mode(), "ab");

#if !defined(QUILL_NO_EXCEPTIONS)
  // Modes the sink cannot honour are rejected instead of failing or misbehaving at open
  for (auto const* invalid_mode : {"a+", "w+", "r", "r+", "x", ""})
  {
    REQUIRE_THROWS_AS(fsc.set_open_mode(std::string_view{invalid_mode}), QuillError);
  }
  REQUIRE_THROWS_AS(fsc.set_open_mode('r'), QuillError);
  REQUIRE_EQ(fsc.open_mode(), "ab");
#endif
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
TEST_CASE("append_epoch_to_gmt_time_file_name")
{
#if !defined(_WIN32)
  // %s is the epoch value whatever the local timezone, here five hours behind UTC
  struct ScopedTimezone
  {
    explicit ScopedTimezone(char const* time_zone)
    {
      char const* const current_time_zone = std::getenv("TZ");

      if (current_time_zone)
      {
        previous_time_zone = current_time_zone;
        had_time_zone = true;
      }

      setenv("TZ", time_zone, 1);
      tzset();
    }

    ~ScopedTimezone()
    {
      if (had_time_zone)
      {
        setenv("TZ", previous_time_zone.c_str(), 1);
      }
      else
      {
        unsetenv("TZ");
      }

      tzset();
    }

    std::string previous_time_zone;
    bool had_time_zone{false};
  } const scoped_timezone{"EST5"};
#endif

  fs::path const filename = "append_epoch_to_gmt_time_file_name.log";
  fs::path const expected_filename = "append_epoch_to_gmt_time_file_name_1704067200_%s.log";

  FileSinkConfig fsc;
  fsc.set_filename_append_option(FilenameAppendOption::StartCustomTimestampFormat, "_%s_%%s");
  fsc.set_timezone(Timezone::GmtTime);

  {
    FileSink file_sink{filename, fsc, FileEventNotifier{}, true,
                       std::chrono::system_clock::time_point{std::chrono::seconds{1704067200}}};

    REQUIRE_EQ(file_sink.get_filename().filename(), expected_filename);
    REQUIRE(fs::exists(expected_filename));
  }

  testing::remove_file(expected_filename);
}

/***/
TEST_CASE("append_custom_timestamp_with_empty_locale_expansion")
{
  struct ScopedLcTime
  {
    std::string previous{std::setlocale(LC_TIME, nullptr)};
    ~ScopedLcTime() { std::setlocale(LC_TIME, previous.c_str()); }
  } scoped_lc_time;

  if (std::setlocale(LC_TIME, "fr_FR.UTF-8") == nullptr)
  {
    return;
  }

  std::tm time_info{};
  char am_pm[32];
  if (std::strftime(am_pm, sizeof(am_pm), "%p", &time_info) != 0)
  {
    return;
  }

  fs::path const filename{"append_custom_timestamp_with_empty_locale_expansion.log"};
  FileSinkConfig config;
  config.set_timezone(Timezone::GmtTime);
  config.set_filename_append_option(FilenameAppendOption::StartCustomTimestampFormat, "%p");

  {
    FileSink file_sink{filename, config};
    REQUIRE(fs::exists(filename));
  }

  testing::remove_file(filename);
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
TEST_CASE("created_file_permissions_follow_umask")
{
#if !defined(_WIN32)
  fs::path const filename = "created_file_permissions_follow_umask.log";
  testing::remove_file(filename);

  mode_t const previous_umask = ::umask(0002);
  {
    FileSink file_sink{filename};
  }
  ::umask(previous_umask);

  struct stat file_stat
  {
  };
  REQUIRE_EQ(::stat(filename.string().data(), &file_stat), 0);
  REQUIRE_EQ(file_stat.st_mode & 0777, 0664);

  testing::remove_file(filename);
#endif
}

/***/
TEST_CASE("windows_file_handle_is_not_inheritable")
{
#if defined(_WIN32)
  fs::path const filename{"windows_file_handle_inheritance.log"};
  {
    FileSinkTestHarness sink{filename};
    DWORD flags{0};
    REQUIRE(::GetHandleInformation(sink.file_handle(), &flags));
    REQUIRE_EQ(flags & HANDLE_FLAG_INHERIT, 0u);
  }
  fs::remove(filename);
#endif
}

/***/
TEST_CASE("windows_file_open_reports_actual_attempt_count")
{
#if defined(_WIN32) && !defined(QUILL_NO_EXCEPTIONS)
  // An exclusive handle makes every open attempt fail with a sharing violation
  fs::path const filename{"windows_file_open_attempt_count.log"};
  HANDLE const handle{::CreateFileW(filename.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, nullptr)};
  REQUIRE(handle != INVALID_HANDLE_VALUE);

  std::string error;
  try
  {
    FileSink sink{filename};
  }
  catch (QuillError const& e)
  {
    error = e.what();
  }

  ::CloseHandle(handle);
  fs::remove(filename);
  REQUIRE_NE(error.find("after 3 attempt(s)"), std::string::npos);
#endif
}

/***/
TEST_CASE("open_failure_is_reported_without_retry_delay")
{
#if !defined(_WIN32) && !defined(QUILL_NO_EXCEPTIONS)
  // A directory cannot be opened for writing, and retrying would not help
  fs::path const directory = "open_failure_is_reported_without_retry_delay_dir";
  fs::create_directories(directory);

  auto const start = std::chrono::steady_clock::now();
  REQUIRE_THROWS_AS(FileSink{directory}, QuillError);
  auto const elapsed = std::chrono::steady_clock::now() - start;

  // Previously three attempts with 200 ms sleeps took at least 400 ms
  REQUIRE_LT(elapsed, std::chrono::milliseconds{200});

  fs::remove(directory);
#endif
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
    file_sink.write_log(LogRecord{nullptr,
                                  0,
                                  std::string_view{},
                                  std::string_view{},
                                  std::string{},
                                  std::string_view{},
                                  LogLevel::Info,
                                  "INFO",
                                  "I",
                                  nullptr,
                                  "",
                                  "unicode filename test\n",
                                  {}});
    file_sink.flush_sink(SinkFlushReason::Explicit);
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
TEST_CASE("open_extended_length_filename")
{
#if defined(_WIN32)
  auto const absolute_root = fs::absolute("file_sink_extended_path_test").native();
  fs::path const root{absolute_root.compare(0, 2, L"\\\\") == 0 ? L"\\\\?\\UNC\\" + absolute_root.substr(2)
                                                                : L"\\\\?\\" + absolute_root};
  std::vector<fs::path> directories{root};
  while (directories.back().native().size() <= MAX_PATH)
  {
    directories.push_back(directories.back() / std::wstring(40, L'x'));
  }

  fs::create_directories(directories.back());
  fs::path const filename = directories.back() / "extended.log";

  {
    FileSink sink{filename};
    REQUIRE_EQ(sink.get_filename().native().compare(0, 4, L"\\\\?\\"), 0);
    sink.write_log(LogRecord{
      nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "long path\n", {}});
    sink.flush_sink(SinkFlushReason::Explicit);
  }

  REQUIRE(testing::file_contents(filename) == std::vector<std::string>{"long path"});

  REQUIRE(fs::remove(filename));
  for (auto it = directories.rbegin(); it != directories.rend(); ++it)
  {
    REQUIRE(fs::remove(*it));
  }
#endif
}

TEST_CASE("dev_null_special_path")
{
  FileSinkTestHarness file_sink{"/dev/null"};
  REQUIRE(file_sink.is_null());

#if defined(_WIN32)
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());
#else
  REQUIRE_NE(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());
#endif

  file_sink.write_log(LogRecord{nullptr,
                                0,
                                std::string_view{},
                                std::string_view{},
                                std::string{},
                                std::string_view{},
                                LogLevel::Info,
                                "INFO",
                                "I",
                                nullptr,
                                "",
                                "dev null test\n",
                                {}});
  file_sink.flush_sink(SinkFlushReason::Explicit);
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
    file_sink.write_log(LogRecord{nullptr,
                                  0,
                                  std::string_view{},
                                  std::string_view{},
                                  std::string{},
                                  std::string_view{},
                                  LogLevel::Info,
                                  "INFO",
                                  "I",
                                  nullptr,
                                  "",
                                  message,
                                  {}});
    file_sink.flush_sink(SinkFlushReason::Explicit);
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
    file_sink.write_log(LogRecord{nullptr,
                                  0,
                                  std::string_view{},
                                  std::string_view{},
                                  std::string{},
                                  std::string_view{},
                                  LogLevel::Info,
                                  "INFO",
                                  "I",
                                  nullptr,
                                  "",
                                  message,
                                  {}});
  };

  {
    FileSink file_sink{filename, fsc, file_event_notifier};

    write_record(file_sink, "first\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);

    testing::remove_file(filename);
    REQUIRE_FALSE(fs::exists(filename));

    recreate_on_next_close = true;

    // This record is written to the deleted file handle. After close, the notifier recreates the
    // visible file before Quill reopens it.
    write_record(file_sink, "second\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);

    write_record(file_sink, "third\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);
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
TEST_CASE("deleted_file_retries_failed_reopen")
{
#if defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  fs::path const filename = "deleted_file_retries_failed_reopen.log";
  uint32_t open_attempts{0};
  FileEventNotifier notifier;
  notifier.before_open = [&open_attempts](fs::path const&)
  {
    ++open_attempts;
    if (open_attempts == 2)
    {
      QUILL_THROW(QuillError{"temporary reopen failure"});
    }
  };

  auto write_record = [](FileSink& sink, std::string_view message)
  {
    sink.write_log(LogRecord{nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, "", message, {}});
  };

  {
    FileSinkTestHarness sink{filename, FileSinkConfig{}, notifier};
    write_record(sink, "before deletion\n");
    sink.flush_sink(SinkFlushReason::Explicit);
    REQUIRE(fs::remove(filename));
    write_record(sink, "deleted file\n");
    CHECK_THROWS_AS(sink.flush_sink(SinkFlushReason::Explicit), QuillError);
    CHECK_EQ(sink.file_handle(), FileSinkTestHarness::closed_file_handle());

    sink.flush_sink(SinkFlushReason::Explicit);
    CHECK_NE(sink.file_handle(), FileSinkTestHarness::closed_file_handle());
    CHECK_EQ(open_attempts, 3u);
    write_record(sink, "recovered\n");
    sink.flush_sink(SinkFlushReason::Explicit);
  }

  auto const contents = testing::file_contents(filename);
  REQUIRE_EQ(contents.size(), 1);
  CHECK_EQ(contents[0], "recovered");
  testing::remove_file(filename);
#endif
}

TEST_CASE("deleted_file_retries_failed_before_close")
{
#if defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  fs::path const filename = "deleted_file_retries_failed_before_close.log";
  uint32_t open_attempts{0};
  uint32_t close_attempts{0};
  FileEventNotifier notifier;
  notifier.before_open = [&open_attempts](fs::path const&) { ++open_attempts; };
  notifier.before_close = [&close_attempts](fs::path const&, FileEventNotifierHandle)
  {
    ++close_attempts;
    if (close_attempts == 1)
    {
      QUILL_THROW(QuillError{"temporary before_close failure"});
    }
  };

  auto write_record = [](FileSink& sink, std::string_view message)
  {
    sink.write_log(LogRecord{nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, "", message, {}});
  };

  {
    FileSinkTestHarness sink{filename, FileSinkConfig{}, notifier};
    write_record(sink, "before deletion\n");
    sink.flush_sink(SinkFlushReason::Explicit);
    REQUIRE(fs::remove(filename));
    write_record(sink, "deleted file\n");
    CHECK_THROWS_AS(sink.flush_sink(SinkFlushReason::Explicit), QuillError);
    CHECK_EQ(sink.file_handle(), FileSinkTestHarness::closed_file_handle());

    sink.flush_sink(SinkFlushReason::Explicit);
    CHECK_NE(sink.file_handle(), FileSinkTestHarness::closed_file_handle());
    CHECK_EQ(open_attempts, 2u);
    write_record(sink, "recovered\n");
    sink.flush_sink(SinkFlushReason::Explicit);
  }

  auto const contents = testing::file_contents(filename);
  REQUIRE_EQ(contents.size(), 1);
  CHECK_EQ(contents[0], "recovered");
  testing::remove_file(filename);
#endif
}

TEST_CASE("deleted_file_retries_failed_after_close")
{
#if defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  fs::path const filename = "deleted_file_retries_failed_after_close.log";
  uint32_t open_attempts{0};
  uint32_t close_attempts{0};
  FileEventNotifier notifier;
  notifier.before_open = [&open_attempts](fs::path const&) { ++open_attempts; };
  notifier.after_close = [&close_attempts](fs::path const&)
  {
    ++close_attempts;
    if (close_attempts == 1)
    {
      QUILL_THROW(QuillError{"temporary after_close failure"});
    }
  };

  auto write_record = [](FileSink& sink, std::string_view message)
  {
    sink.write_log(LogRecord{nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, "", message, {}});
  };

  {
    FileSinkTestHarness sink{filename, FileSinkConfig{}, notifier};
    write_record(sink, "before deletion\n");
    sink.flush_sink(SinkFlushReason::Explicit);
    REQUIRE(fs::remove(filename));
    write_record(sink, "deleted file\n");
    CHECK_THROWS_AS(sink.flush_sink(SinkFlushReason::Explicit), QuillError);
    CHECK_EQ(sink.file_handle(), FileSinkTestHarness::closed_file_handle());

    sink.flush_sink(SinkFlushReason::Explicit);
    CHECK_NE(sink.file_handle(), FileSinkTestHarness::closed_file_handle());
    CHECK_EQ(open_attempts, 2u);
    write_record(sink, "recovered\n");
    sink.flush_sink(SinkFlushReason::Explicit);
  }

  auto const contents = testing::file_contents(filename);
  REQUIRE_EQ(contents.size(), 1);
  CHECK_EQ(contents[0], "recovered");
  testing::remove_file(filename);
#endif
}

TEST_CASE("reopen_different_filename_keeps_flushes_on_new_path")
{
  fs::path const original_filename{"reopen_different_filename.log"};
  fs::path const new_filename{"reopen_different_filename_destination.log"};
  fs::path const normalized_new_filename = normalize_file_sink_path(new_filename);
  testing::remove_file(original_filename);
  testing::remove_file(new_filename);

  fs::path last_closed_filename;
  FileEventNotifier notifier;
  notifier.after_close = [&last_closed_filename](fs::path const& filename)
  { last_closed_filename = filename; };
  FileSinkConfig config;
  config.set_open_mode('w');

  auto write_record = [](FileSink& sink, std::string_view message)
  {
    sink.write_log(LogRecord{nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, message, {}});
  };

  {
    FileSinkTestHarness sink{original_filename, config, notifier};
    write_record(sink, "before move\n");
    sink.close_file_for_test();
    fs::rename(sink.get_filename(), new_filename);

    // A subclass can reopen a relative destination after moving its original file.
    sink.open_file_for_test(new_filename, "a");
    CHECK_EQ(sink.get_filename(), normalized_new_filename);

    write_record(sink, "after move 1\n");
    sink.flush_sink(SinkFlushReason::Explicit);
    write_record(sink, "after move 2\n");
    sink.flush_sink(SinkFlushReason::Explicit);
  }

  // Flushing must not mistake the old path for a deleted file and reopen it.
  CHECK_FALSE(fs::exists(original_filename));
  std::vector<std::string> const expected_lines{"before move", "after move 1", "after move 2"};
  CHECK(testing::file_contents(new_filename) == expected_lines);
  CHECK_EQ(last_closed_filename, normalized_new_filename);

  testing::remove_file(original_filename);
  testing::remove_file(new_filename);
}

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
  fs::path const new_filename = "after_open_throw_during_reopen_new_filename.log";
  testing::remove_file(filename);
  testing::remove_file(new_filename);

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
  fs::path const original_filename = file_sink.get_filename();
  file_sink.open_file_for_test(file_sink.get_filename(), "a");
  REQUIRE_NE(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());

  file_sink.close_file_for_test();
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(file_sink.open_file_for_test(new_filename, "a"), QuillError);
#endif
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());
  CHECK_EQ(file_sink.get_filename(), original_filename);

  // fsync_file() must tolerate the closed handle state left behind by a failed reopen.
  // Previously this dereferenced a null FILE* / invalid HANDLE and crashed.
  file_sink.fsync_file_for_test(true);
  file_sink.fsync_file_for_test(false);
  REQUIRE_EQ(file_sink.file_handle(), FileSinkTestHarness::closed_file_handle());

  testing::remove_file(filename);
  testing::remove_file(new_filename);
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
TEST_CASE("windows_destructor_closes_handle_after_write_failure")
{
#if !defined(_WIN32) || defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  fs::path const filename = "windows_destructor_write_failure.log";
  FileSinkConfig config;
  config.set_open_mode('w');
  struct HandleGuard
  {
    HANDLE handle;
    ~HandleGuard()
    {
      if (handle != INVALID_HANDLE_VALUE)
      {
        ::CloseHandle(handle);
      }
    }
  };

  {
    auto sink = std::make_unique<FileSinkTestHarness>(filename, config);
    HANDLE const sink_handle = sink->file_handle();
    HandleGuard lock_file{::CreateFileW(filename.c_str(), GENERIC_READ | GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    REQUIRE_NE(lock_file.handle, INVALID_HANDLE_VALUE);
    OVERLAPPED range{};
    REQUIRE(::LockFileEx(lock_file.handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0,
                         4096, 0, &range));
    sink->write_log(LogRecord{
      nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, "", "buffered\n", {}});
    sink.reset(); // Flushing fails because another handle holds the byte-range lock.

    DWORD flags{0};
    bool const still_open = ::GetHandleInformation(sink_handle, &flags) != 0;
    CHECK_FALSE(still_open);
    if (still_open)
    {
      ::CloseHandle(sink_handle);
    }
  }
  testing::remove_file(filename);
#endif
}

TEST_CASE("fsync_skipped_by_interval_runs_on_final_flush")
{
  fs::path const filename = "fsync_skipped_by_interval_runs_on_final_flush.log";

  FileSinkConfig config;
  config.set_fsync_enabled(true);
  config.set_minimum_fsync_interval(std::chrono::hours{1});

  {
    FileSinkTestHarness sink{filename, config};

    // The first fsync runs, the second is within the interval and is deferred
    sink.write_log(LogRecord{
      nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "first\n", {}});
    sink.flush_sink(SinkFlushReason::Periodic);
    REQUIRE_FALSE(sink.fsync_pending_for_test());

    sink.write_log(LogRecord{
      nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "second\n", {}});
    sink.flush_sink(SinkFlushReason::Periodic);
    REQUIRE(sink.fsync_pending_for_test());

    // Nothing new was written, but the deferred fsync is still owed until the final flush
    sink.flush_sink(SinkFlushReason::Periodic);
    REQUIRE(sink.fsync_pending_for_test());

    sink.flush_sink(SinkFlushReason::Final);
    REQUIRE_FALSE(sink.fsync_pending_for_test());
  }

  testing::remove_file(filename);
}

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

TEST_CASE("stream_sink_reports_persistent_write_errors")
{
#if defined(__linux__) && !defined(QUILL_NO_EXCEPTIONS)
  FILE* stream = std::fopen("/dev/full", "w");
  REQUIRE_NE(stream, nullptr);
  REQUIRE_EQ(std::setvbuf(stream, nullptr, _IONBF, 0), 0);

  bool error_reported{false};
  try
  {
    StreamSink::safe_fwrite_unbuffered("test", 1, 4, stream);
  }
  catch (QuillError const&)
  {
    error_reported = true;
  }

  std::fclose(stream);
  REQUIRE(error_reported);
#endif
}

/***/
TEST_CASE("write_binary_writes_the_bytes_unchanged")
{
  fs::path const filename = "write_binary_writes_the_bytes_unchanged.bin";
  testing::remove_file(filename);

  // Every byte value, including the ones a text stream or the pattern formatter would change
  std::string all_bytes;
  for (int value = 0; value < 256; ++value)
  {
    all_bytes.push_back(static_cast<char>(value));
  }

  std::string const line_breaks{"\n\r\n\n"};

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    FileSink file_sink{filename, fsc};

    write_binary_record(file_sink, all_bytes);
    write_binary_record(file_sink, std::string_view{});
    write_binary_record(file_sink, line_breaks);
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  REQUIRE(read_file_bytes(filename) == all_bytes + line_breaks);

  testing::remove_file(filename);
}

/***/
TEST_CASE("write_binary_applies_before_write_before_the_frame")
{
  fs::path const filename = "write_binary_applies_before_write_before_the_frame.bin";
  testing::remove_file(filename);

  // before_write transforms everything the sink writes: the header and the binary records here.
  // The frame of a record has the size of the bytes that are in the file after it
  FileEventNotifier file_event_notifier;
  file_event_notifier.before_write = [](std::string_view bytes)
  { return "<" + std::string{bytes} + ">"; };

  std::string const record{"\x01\x00\n", 3};

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_file_header("HEAD");
    fsc.set_binary_record_framing(BinaryRecordFraming::Size);

    FileSink file_sink{filename, fsc, file_event_notifier};

    write_binary_record(file_sink, record);
    write_binary_record(file_sink, std::string_view{});
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  REQUIRE(read_file_bytes(filename) ==
          "<HEAD>" + bytes_of(uint32_t{5}) + "<" + record + ">" + bytes_of(uint32_t{2}) + "<>");

  testing::remove_file(filename);
}

/***/
TEST_CASE("write_binary_frames_the_records_with_their_size")
{
  fs::path const filename = "write_binary_frames_the_records_with_their_size.bin";
  testing::remove_file(filename);

  std::string const first_record{"\x01\x00\n", 3};
  std::string const second_record(300, '\xff');

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_binary_record_framing(BinaryRecordFraming::Size);
    REQUIRE(fsc.binary_record_framing() == BinaryRecordFraming::Size);

    FileSink file_sink{filename, fsc};

    write_binary_record(file_sink, first_record);
    write_binary_record(file_sink, std::string_view{});
    write_binary_record(file_sink, second_record);
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  // A record without bytes has a frame too, so that a reader counts the same records
  REQUIRE(read_file_bytes(filename) ==
          bytes_of(uint32_t{3}) + first_record + bytes_of(uint32_t{0}) + bytes_of(uint32_t{300}) + second_record);

  testing::remove_file(filename);
}

/***/
TEST_CASE("write_binary_frames_the_records_with_their_timestamp_and_size")
{
  fs::path const filename = "write_binary_frames_the_records_with_their_timestamp_and_size.bin";
  testing::remove_file(filename);

  std::string const file_header{"FEED0001"};
  std::string const first_record{"\x01\x00\n", 3};
  std::string const second_record(300, '\xff');

  uint64_t const first_timestamp{1'686'528'000'000'000'001ull};
  uint64_t const second_timestamp{1'686'528'000'123'456'789ull};

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_file_header(file_header);
    fsc.set_binary_record_framing(BinaryRecordFraming::TimestampAndSize);

    FileSink file_sink{filename, fsc};

    write_binary_record(file_sink, first_record, first_timestamp);
    write_binary_record(file_sink, second_record, second_timestamp);
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  // The header of the file has no frame
  REQUIRE(read_file_bytes(filename) ==
          file_header + bytes_of(first_timestamp) + bytes_of(uint32_t{3}) + first_record +
            bytes_of(second_timestamp) + bytes_of(uint32_t{300}) + second_record);

  testing::remove_file(filename);
}

/***/
TEST_CASE("file_header_starts_a_new_file")
{
  fs::path const filename = "file_header_starts_a_new_file.csv";
  testing::remove_file(filename);

  // before_write transforms everything the sink writes, the header too
  FileEventNotifier file_event_notifier;
  file_event_notifier.before_write = [](std::string_view message)
  { return ">" + std::string{message}; };

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_file_header("id,name\n");
    REQUIRE_EQ(fsc.file_header(), "id,name\n");

    FileSink file_sink{filename, fsc, file_event_notifier};

    // The header is in the file as soon as it is opened
    REQUIRE(read_file_bytes(filename) == ">id,name\n");

    write_log_statement(file_sink, "1,first\n");
    write_log_statement(file_sink, "2,second\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  REQUIRE(read_file_bytes(filename) == ">id,name\n>1,first\n>2,second\n");

  testing::remove_file(filename);
}

/***/
TEST_CASE("after_open_is_called_before_the_file_header_is_written")
{
  fs::path const filename = "after_open_is_called_before_the_file_header_is_written.csv";
  testing::remove_file(filename);

  // The callback gets the file as it was opened, so it can still configure the stream, or
  // refuse a file that has content
  size_t after_open_calls{0};
  uint64_t file_size_in_after_open{1};

  FileEventNotifier file_event_notifier;
  file_event_notifier.after_open = [&after_open_calls, &file_size_in_after_open](
                                     fs::path const& file_path, FileEventNotifierHandle)
  {
    ++after_open_calls;
    file_size_in_after_open = fs::file_size(file_path);
  };

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_file_header("id,name\n");

    FileSink file_sink{filename, fsc, file_event_notifier};

    REQUIRE_EQ(after_open_calls, 1);
    REQUIRE_EQ(file_size_in_after_open, 0);
    REQUIRE(read_file_bytes(filename) == "id,name\n");
  }

  testing::remove_file(filename);
}

/***/
TEST_CASE("file_header_is_not_repeated_when_appending_to_a_file_with_content")
{
  fs::path const filename = "file_header_is_not_repeated_when_appending.csv";
  testing::remove_file(filename);

  FileSinkConfig fsc;
  fsc.set_open_mode('a');
  fsc.set_file_header("id,name\n");

  {
    // The file does not exist yet, so appending starts it with the header
    FileSink file_sink{filename, fsc};
    write_log_statement(file_sink, "1,first\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  {
    FileSink file_sink{filename, fsc};
    write_log_statement(file_sink, "2,second\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  REQUIRE(read_file_bytes(filename) == "id,name\n1,first\n2,second\n");

  testing::remove_file(filename);
}

/***/
TEST_CASE("file_header_starts_the_file_that_replaces_a_deleted_one")
{
#if defined(__linux__)
  fs::path const filename = "file_header_starts_the_file_that_replaces_a_deleted_one.csv";
  testing::remove_file(filename);

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('a');
    fsc.set_file_header("id,name\n");
    FileSink file_sink{filename, fsc};

    write_log_statement(file_sink, "1,first\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);

    testing::remove_file(filename);
    REQUIRE_FALSE(fs::exists(filename));

    // This record is written to the deleted file. The flush then finds the file gone and opens
    // a new one
    write_log_statement(file_sink, "2,second\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);

    write_log_statement(file_sink, "3,third\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  REQUIRE(read_file_bytes(filename) == "id,name\n3,third\n");

  testing::remove_file(filename);
#else
  return;
#endif
}

/***/
TEST_CASE("file_header_is_synced_when_fsync_is_enabled")
{
  fs::path const filename = "file_header_is_synced_when_fsync_is_enabled.csv";
  testing::remove_file(filename);

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_fsync_enabled(true);
    fsc.set_file_header("id,name\n");

    FileSinkTestHarness file_sink{filename, fsc};

    // No record is written. The header is in the file, and the next flush syncs it
    REQUIRE(file_sink.fsync_pending_for_test());

    file_sink.flush_sink(SinkFlushReason::Explicit);
    REQUIRE_FALSE(file_sink.fsync_pending_for_test());
  }

  REQUIRE(read_file_bytes(filename) == "id,name\n");

  testing::remove_file(filename);
}

/***/
TEST_CASE("write_buffer_keeps_records_until_it_is_full_or_the_sink_is_flushed")
{
  fs::path const filename = "write_buffer_keeps_records_until_it_is_full.bin";
  testing::remove_file(filename);

  std::string const record(1000, 'a');

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_write_buffer_size(4096);
    FileSink file_sink{filename, fsc};

    // Four records fit in the buffer, so the file has none of them yet
    for (int i = 0; i < 4; ++i)
    {
      write_binary_record(file_sink, record);
    }

    REQUIRE_EQ(read_file_bytes(filename).size(), 0);

    // The fifth does not fit: the buffer is written to the file, and the record starts it again
    write_binary_record(file_sink, record);
    REQUIRE_EQ(read_file_bytes(filename).size(), 4000);

    file_sink.flush_sink(SinkFlushReason::Explicit);
    REQUIRE_EQ(read_file_bytes(filename).size(), 5000);
  }

  REQUIRE_EQ(read_file_bytes(filename).size(), 5000);

  testing::remove_file(filename);
}

/***/
TEST_CASE("buffered_file_sink_allows_empty_formatted_log_records")
{
  fs::path const filename = "buffered_file_sink_empty_formatted_log_records.log";
  FileSinkConfig config;
  config.set_open_mode('w');

  LogRecord const record{nullptr, 0,   {},      {}, {}, {}, LogLevel::Info,
                         "INFO",  "I", nullptr, {}, {}, {}};
  {
    FileSink sink{filename, config};
    sink.write_log(record);
    sink.flush_sink(SinkFlushReason::Explicit);
    CHECK(read_file_bytes(filename).empty());
  }

  {
    FileEventNotifier notifier;
    notifier.before_write = [](std::string_view statement)
    {
      CHECK(statement.empty());
      return std::string{"replacement"};
    };
    FileSink sink{filename, config, notifier};
    sink.write_log(record);
    sink.flush_sink(SinkFlushReason::Explicit);
    CHECK_EQ(read_file_bytes(filename), "replacement");
  }

  testing::remove_file(filename);
}

/***/
TEST_CASE("record_larger_than_the_write_buffer_is_written_in_order")
{
  fs::path const filename = "record_larger_than_the_write_buffer.bin";
  testing::remove_file(filename);

  std::string const first_record{"first|"};
  std::string const large_record(10000, 'b');
  std::string const last_record{"|last"};

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_write_buffer_size(4096);
    FileSink file_sink{filename, fsc};

    write_binary_record(file_sink, first_record);
    write_binary_record(file_sink, large_record);

    // The large record does not fit in the buffer. It is written to the file on its own, after
    // the record that was in the buffer
    REQUIRE(read_file_bytes(filename) == first_record + large_record);

    write_log_statement(file_sink, last_record);
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  REQUIRE(read_file_bytes(filename) == first_record + large_record + last_record);

  testing::remove_file(filename);
}

/***/
TEST_CASE("write_buffer_disabled_writes_records_through_the_file_handle")
{
  fs::path const filename = "write_buffer_disabled_writes_records_through_the_file_handle.bin";
  testing::remove_file(filename);

  auto const write_through_handle = [](FileEventNotifierHandle handle, std::string_view text)
  {
#if defined(_WIN32)
    DWORD written{0};
    REQUIRE(::WriteFile(handle, text.data(), static_cast<DWORD>(text.size()), &written, nullptr));
    REQUIRE_EQ(written, text.size());
#else
    REQUIRE_EQ(std::fwrite(text.data(), 1, text.size(), handle), text.size());
#endif
  };

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_write_buffer_size(0);

    // Without the buffer of the sink, the callback decides how the file is buffered and can keep
    // the handle
    FileEventNotifierHandle file_handle{};
    FileEventNotifier file_event_notifier;
    file_event_notifier.after_open = [&file_handle](fs::path const&, FileEventNotifierHandle handle)
    {
#if !defined(_WIN32)
      REQUIRE_EQ(std::setvbuf(handle, nullptr, _IONBF, 0), 0);
#endif
      file_handle = handle;
    };

    FileSink file_sink{filename, fsc, file_event_notifier};

    // Each record is in the file when it is written, without a flush of the sink
    write_binary_record(file_sink, "first|");
    REQUIRE(read_file_bytes(filename) == "first|");

    // What is written through the handle between two records is between them in the file
    write_through_handle(file_handle, "handle|");
    write_log_statement(file_sink, "second");
    REQUIRE(read_file_bytes(filename) == "first|handle|second");

    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  REQUIRE(read_file_bytes(filename) == "first|handle|second");

  testing::remove_file(filename);
}

/***/
TEST_CASE("write_buffer_disabled_keeps_the_buffering_of_the_stream")
{
#if defined(_WIN32)
  return;
#else
  fs::path const filename = "write_buffer_disabled_keeps_the_buffering_of_the_stream.bin";
  testing::remove_file(filename);

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_write_buffer_size(0);

    // The stream is buffered by the C library, as the callback leaves it
    FileSink file_sink{filename, fsc};

    write_binary_record(file_sink, "first|");
    write_log_statement(file_sink, "second");
    REQUIRE(read_file_bytes(filename).empty());

    file_sink.flush_sink(SinkFlushReason::Explicit);
    REQUIRE(read_file_bytes(filename) == "first|second");
  }

  testing::remove_file(filename);
#endif
}

/***/
TEST_CASE("failed_flush_is_reported_and_retried")
{
#if !defined(__linux__) || defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  // Every write to this device fails, as to a disk that is full
  fs::path const filename = "/dev/full";

  if (!fs::exists(filename))
  {
    return;
  }

  FileSinkTestHarness file_sink{filename};

  write_binary_record(file_sink, "record");
  REQUIRE_THROWS_AS(file_sink.flush_sink(SinkFlushReason::Explicit), QuillError);

  // The record is still in the buffer, and the next flush tries to write it again
  REQUIRE(file_sink.write_occurred_for_test());
  REQUIRE_THROWS_AS(file_sink.flush_sink(SinkFlushReason::Explicit), QuillError);

  // The destructor closes the file although the buffer cannot be written
#endif
}

TEST_SUITE_END();
