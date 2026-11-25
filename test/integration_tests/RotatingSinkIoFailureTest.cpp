#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/UserClockSource.h"
#include "quill/sinks/RotatingFileSink.h"

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(__linux__)
  #include <csignal>
  #include <sys/resource.h>
  #include <sys/syscall.h>
  #include <sys/types.h>
  #include <unistd.h>

namespace
{
// While this is set every call to write() fails as it does when the disk is full
bool disk_is_full{false};

// Set for the disk to be full after the first four bytes of the next call to write()
bool disk_fills_up{false};

// While this is set every call to fsync() fails as it does on a failing disk
bool sync_fails{false};

// The calls to fsync() that failed
size_t failed_syncs{0};

// How long a call to fsync() takes to fail
std::chrono::milliseconds sync_failure_duration{0};

/** Limits the size of the files that the process writes, as long as it exists */
class FileSizeLimit
{
public:
  explicit FileSizeLimit(rlim_t size)
  {
    REQUIRE_EQ(::getrlimit(RLIMIT_FSIZE, &_previous_limit), 0);

    // The process is not ended when a write goes past the limit, which then fails with EFBIG
    _previous_handler = std::signal(SIGXFSZ, SIG_IGN);

    rlimit limit = _previous_limit;
    limit.rlim_cur = size;
    REQUIRE_EQ(::setrlimit(RLIMIT_FSIZE, &limit), 0);
  }

  ~FileSizeLimit()
  {
    ::setrlimit(RLIMIT_FSIZE, &_previous_limit);
    std::signal(SIGXFSZ, _previous_handler);
  }

  FileSizeLimit(FileSizeLimit const&) = delete;
  FileSizeLimit& operator=(FileSizeLimit const&) = delete;

private:
  rlimit _previous_limit{};
  void (*_previous_handler)(int){nullptr};
};

/** A clock that the test sets, for records that are an hour apart */
class ManualClock : public quill::UserClockSource
{
public:
  uint64_t now() const noexcept override { return timestamp_ns; }

  uint64_t timestamp_ns{0};
};
} // namespace

// This executable replaces write() to simulate a full disk while the sink writes to the file.
// Other test executables use the one of the C library.
extern "C" ssize_t write(int file_descriptor, void const* buffer, size_t count)
{
  if (disk_is_full)
  {
    errno = ENOSPC;
    return -1;
  }

  if (disk_fills_up)
  {
    disk_fills_up = false;
    disk_is_full = true;
    count = (count < 4) ? count : 4;
  }

  return static_cast<ssize_t>(::syscall(SYS_write, file_descriptor, buffer, count));
}

// And fsync() to simulate a failing disk
extern "C" int fsync(int file_descriptor)
{
  if (sync_fails)
  {
    ++failed_syncs;
    std::this_thread::sleep_for(sync_failure_duration);
    errno = EIO;
    return -1;
  }

  return static_cast<int>(::syscall(SYS_fsync, file_descriptor));
}
#endif

/**
 * A rotation that meets an I/O failure stays due, and the sink keeps working. Without exceptions
 * the rotation must see the failures that its steps report and return from.
 */
TEST_CASE("rotating_sink_io_failure")
{
#if defined(__linux__)
  using namespace quill;

  fs::path const time_filename = "rotating_sink_io_failure_time.log";
  fs::path const time_backup_filename = "rotating_sink_io_failure_time.1.log";
  fs::path const size_filename = "rotating_sink_io_failure_size.log";
  fs::path const size_backup_filename = "rotating_sink_io_failure_size.1.log";
  fs::path const stream_filename = "rotating_sink_io_failure_stream.log";
  fs::path const stream_backup_filename = "rotating_sink_io_failure_stream.1.log";
  fs::path const reopen_filename = "rotating_sink_io_failure_reopen.log";
  fs::path const reopen_backup_filename = "rotating_sink_io_failure_reopen.1.log";
  fs::path const close_filename = "rotating_sink_io_failure_close.log";
  fs::path const close_backup_filename = "rotating_sink_io_failure_close.1.log";

  for (fs::path const& file : {time_filename, time_backup_filename, size_filename, size_backup_filename,
                               stream_filename, stream_backup_filename, reopen_filename,
                               reopen_backup_filename, close_filename, close_backup_filename})
  {
    fs::remove_all(file);
  }

  auto const read_file_bytes = [](fs::path const& file_to_read)
  {
    std::ifstream file{file_to_read, std::ios::binary};
    return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
  };

  std::vector<std::string> errors;

  auto const count_errors_with = [&errors](std::string const& text)
  {
    size_t count{0};

    for (std::string const& error : errors)
    {
      count += (error.find(text) != std::string::npos) ? 1u : 0u;
    }

    return count;
  };

  std::string const disk_full_error = "write failed errno: " + std::to_string(ENOSPC);

  // This thread runs the backend. poll_one() processes the records without flushing the sinks,
  // so a record stays in the write buffer of its sink until the poll() that follows. The backend
  // does not repeat the error that it has just reported, so the full disk is not the failure of
  // two parts in a row
  BackendOptions backend_options;
  backend_options.error_notifier = [&errors](std::string const& error) { errors.push_back(error); };
  backend_options.sink_min_flush_interval = std::chrono::milliseconds{0};
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(backend_options);

  PatternFormatterOptions const pattern_formatter_options{"%(message)"};
  ManualClock clock;

  {
    // A rotation by time of a file that holds no records yet: they are in the write buffer, and
    // the full disk does not take them. The rotation stays due instead of waiting for the next
    // hour, and the file is rotated once the disk has room
    constexpr uint64_t start_time_s{1686546600}; // 2023-06-12 05:10:00 UTC

    RotatingFileSinkConfig config;
    config.set_open_mode('w');
    config.set_rotation_frequency_and_interval('H', 1);
    config.set_timezone(Timezone::GmtTime);

    auto sink = Frontend::create_or_get_sink<RotatingFileSink>(
      time_filename, config, FileEventNotifier{},
      std::chrono::system_clock::time_point{std::chrono::seconds{start_time_s}});
    Logger* logger = Frontend::create_or_get_logger("rotating_sink_io_failure_time", std::move(sink),
                                                    pattern_formatter_options, ClockSource::custom(clock));

    clock.timestamp_ns = (start_time_s + 1u) * 1'000'000'000u;
    LOG_INFO(logger, "first hour");
    manual_backend_worker->poll_one();

    clock.timestamp_ns = (start_time_s + 3600u) * 1'000'000'000u;
    LOG_INFO(logger, "second hour, disk full");
    disk_is_full = true;
    manual_backend_worker->poll_one();
    disk_is_full = false;

    REQUIRE_EQ(count_errors_with(disk_full_error), 1u);
    REQUIRE_FALSE(fs::exists(time_backup_filename));

    LOG_INFO(logger, "second hour");
    manual_backend_worker->poll();

  #if defined(QUILL_NO_EXCEPTIONS)
    // The record that met the full disk was still appended to the write buffer
    REQUIRE_EQ(read_file_bytes(time_backup_filename), "first hour\nsecond hour, disk full\n");
  #else
    REQUIRE_EQ(read_file_bytes(time_backup_filename), "first hour\n");
  #endif
    REQUIRE_EQ(read_file_bytes(time_filename), "second hour\n");
    REQUIRE_EQ(count_errors_with(disk_full_error), 1u);

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
  }

  {
    // The file cannot be opened again after a rotation has renamed it: the disk is full after
    // the first bytes of the header of the new file. These bytes do not stay in the file. The
    // sink is left without a file, and opens it after the retry interval, with its whole header
    size_t open_attempts{0};

    FileEventNotifier file_event_notifier;
    file_event_notifier.before_open = [&open_attempts](fs::path const&) { ++open_attempts; };

    std::string const file_header{"header of the file\n"};
    std::string const header_error{"Failed to write the file header"};

    RotatingFileSinkConfig config;
    config.set_open_mode('w');
    config.set_rotation_max_file_size(1024);
    config.set_file_header(file_header);

    auto sink = Frontend::create_or_get_sink<RotatingFileSink>(reopen_filename, config, file_event_notifier);
    Logger* logger = Frontend::create_or_get_logger("rotating_sink_io_failure_reopen",
                                                    std::move(sink), pattern_formatter_options);

    std::string const in_file(600, 'a');
    std::string const too_large_for_file(600, 'b');

    LOG_INFO(logger, "{}", in_file);
    manual_backend_worker->poll();
    REQUIRE_EQ(open_attempts, 1u);
    REQUIRE_EQ(read_file_bytes(reopen_filename), file_header + in_file + "\n");

    size_t const header_errors = count_errors_with(header_error);
    LOG_INFO(logger, "{}", too_large_for_file);
    disk_fills_up = true;
    manual_backend_worker->poll_one();

    REQUIRE_FALSE(disk_fills_up);
    REQUIRE_EQ(open_attempts, 2u);
    REQUIRE_EQ(count_errors_with(header_error), header_errors + 1);
    REQUIRE_EQ(read_file_bytes(reopen_backup_filename), file_header + in_file + "\n");
    REQUIRE(fs::exists(reopen_filename));
    REQUIRE_EQ(read_file_bytes(reopen_filename), "");

  #if defined(QUILL_NO_EXCEPTIONS)
    // The next record does not try to open the file before the retry interval has passed
    LOG_INFO(logger, "record without a file");
    manual_backend_worker->poll_one();

    REQUIRE_EQ(open_attempts, 2u);
    REQUIRE_EQ(count_errors_with(header_error), header_errors + 1);
  #endif

    disk_is_full = false;

    // The retry interval of the rotating sink is one second
    std::this_thread::sleep_for(std::chrono::milliseconds{1100});

    LOG_INFO(logger, "record after");
    manual_backend_worker->poll();

    REQUIRE_EQ(read_file_bytes(reopen_filename), file_header + "record after\n");
    REQUIRE_EQ(read_file_bytes(reopen_backup_filename), file_header + in_file + "\n");

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
  }

  {
    // A rotation by size of a file that holds records, on a full disk: what the write buffer
    // has cannot be written, so it is dropped and the file is rotated without it
    RotatingFileSinkConfig config;
    config.set_open_mode('w');
    config.set_rotation_max_file_size(1024);

    auto sink = Frontend::create_or_get_sink<RotatingFileSink>(size_filename, config, FileEventNotifier{});
    Logger* logger = Frontend::create_or_get_logger("rotating_sink_io_failure_size",
                                                    std::move(sink), pattern_formatter_options);

    std::string const in_file(600, 'a');
    std::string const in_write_buffer(300, 'b');
    std::string const too_large_for_file(600, 'c');

    LOG_INFO(logger, "{}", in_file);
    manual_backend_worker->poll();
    REQUIRE_EQ(read_file_bytes(size_filename), in_file + "\n");

    LOG_INFO(logger, "{}", in_write_buffer);
    manual_backend_worker->poll_one();

    size_t const disk_full_errors = count_errors_with(disk_full_error);
    LOG_INFO(logger, "{}", too_large_for_file);
    disk_is_full = true;
    manual_backend_worker->poll_one();
    disk_is_full = false;

    REQUIRE_EQ(count_errors_with(disk_full_error), disk_full_errors + 1);

    LOG_INFO(logger, "record after");
    manual_backend_worker->poll();

    REQUIRE_EQ(read_file_bytes(size_backup_filename), in_file + "\n");
  #if defined(QUILL_NO_EXCEPTIONS)
    // The file was rotated at once, and the record that asked for the rotation is in the new file
    REQUIRE_EQ(read_file_bytes(size_filename), too_large_for_file + "\nrecord after\n");
  #else
    REQUIRE_EQ(read_file_bytes(size_filename), "record after\n");
  #endif
    REQUIRE_EQ(count_errors_with(disk_full_error), disk_full_errors + 1);

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
  }

  {
    // The same rotation with the write buffer disabled: the stream has the record, and cannot
    // write it to a file that has the size that the process may write. The stream drops it, and
    // the file is rotated without it
    RotatingFileSinkConfig config;
    config.set_open_mode('w');
    config.set_rotation_max_file_size(1024);
    config.set_write_buffer_size(0);

    auto sink = Frontend::create_or_get_sink<RotatingFileSink>(stream_filename, config, FileEventNotifier{});
    Logger* logger = Frontend::create_or_get_logger("rotating_sink_io_failure_stream",
                                                    std::move(sink), pattern_formatter_options);

    std::string const in_file(600, 'a');
    std::string const in_stream(300, 'b');
    std::string const too_large_for_file(600, 'c');

    LOG_INFO(logger, "{}", in_file);
    manual_backend_worker->poll();
    REQUIRE_EQ(read_file_bytes(stream_filename), in_file + "\n");

    LOG_INFO(logger, "{}", in_stream);
    manual_backend_worker->poll_one();

    size_t const flush_errors = count_errors_with("fflush failed");
    LOG_INFO(logger, "{}", too_large_for_file);

    {
      FileSizeLimit const file_size_limit{in_file.size() + 1};
      manual_backend_worker->poll_one();
    }

    REQUIRE_EQ(count_errors_with("fflush failed"), flush_errors + 1);

    LOG_INFO(logger, "record after");
    manual_backend_worker->poll();

    REQUIRE_EQ(read_file_bytes(stream_backup_filename), in_file + "\n");
  #if defined(QUILL_NO_EXCEPTIONS)
    REQUIRE_EQ(read_file_bytes(stream_filename), too_large_for_file + "\nrecord after\n");
  #else
    REQUIRE_EQ(read_file_bytes(stream_filename), "record after\n");
  #endif
    REQUIRE_EQ(count_errors_with("fflush failed"), flush_errors + 1);

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
  }

  {
    // The file cannot be closed at a rotation: what before_close writes to it does not fit in
    // the size that the process may write. The file is not complete, so it does not take the
    // place of the backup, which is the only one that is kept. The rotation stays due
    std::string const footer{"footer\n"};
    size_t close_attempts{0};

    FileEventNotifier file_event_notifier;
    file_event_notifier.before_close = [&footer, &close_attempts](fs::path const&, FILE* file)
    {
      ++close_attempts;
      std::fputs(footer.c_str(), file);
    };

    RotatingFileSinkConfig config;
    config.set_open_mode('w');
    config.set_rotation_max_file_size(512);
    config.set_max_backup_files(1);

    auto sink = Frontend::create_or_get_sink<RotatingFileSink>(close_filename, config, file_event_notifier);
    Logger* logger = Frontend::create_or_get_logger("rotating_sink_io_failure_close",
                                                    std::move(sink), pattern_formatter_options);

    std::string const first(300, 'a');
    std::string const second(300, 'b');
    std::string const third(300, 'c');
    std::string const fourth(300, 'd');
    std::string const fifth(300, 'e');

    LOG_INFO(logger, "{}", first);
    manual_backend_worker->poll();
    LOG_INFO(logger, "{}", second);
    manual_backend_worker->poll();

    REQUIRE_EQ(close_attempts, 1u);
    REQUIRE_EQ(read_file_bytes(close_backup_filename), first + "\n" + footer);
    REQUIRE_EQ(read_file_bytes(close_filename), second + "\n");

    size_t const close_errors = count_errors_with("fclose failed");
    LOG_INFO(logger, "{}", third);

    {
      // The file has the size that the process may write, so its footer does not fit
      FileSizeLimit const file_size_limit{second.size() + 1};
      manual_backend_worker->poll_one();
      REQUIRE_EQ(close_attempts, 2u);

      // The next records open the file again and wait in its write buffer. The close would fail
      // in the same way, so it is not tried again before the retry interval
      LOG_INFO(logger, "{}", fourth);
      LOG_INFO(logger, "{}", fifth);
      manual_backend_worker->poll_one();
      manual_backend_worker->poll_one();
      REQUIRE_EQ(close_attempts, 2u);
    }

    REQUIRE_EQ(count_errors_with("fclose failed"), close_errors + 1);
    REQUIRE_EQ(read_file_bytes(close_backup_filename), first + "\n" + footer);

    // After the retry interval of one second, the rotation that is still due gives the file its footer
    std::this_thread::sleep_for(std::chrono::milliseconds{1100});
    LOG_INFO(logger, "record after");
    manual_backend_worker->poll();

    REQUIRE_EQ(close_attempts, 3u);
    REQUIRE_EQ(read_file_bytes(close_backup_filename), second + "\n" + fourth + "\n" + fifth + "\n" + footer);
    REQUIRE_EQ(read_file_bytes(close_filename), "record after\n");

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
  }

  for (bool const defer_periodic_sync : {false, true})
  {
    // A rotation whose sync fails, e.g. on a failing disk: the sync of the flush before it, or,
    // when minimum_fsync_interval defers that one, the sync that the rotation forces. The sync
    // would fail in the same way for every record, so the rotation is retried after the retry
    // interval instead. The interval starts when the sync fails, here after longer than the
    // interval, as it can on a network file system
    std::string const name =
      std::string{"rotating_sink_io_failure_sync_"} + (defer_periodic_sync ? "deferred" : "periodic");
    fs::path const filename = name + ".log";
    fs::path const backup_filename = name + ".1.log";
    fs::remove(filename);
    fs::remove(backup_filename);

    RotatingFileSinkConfig config;
    config.set_open_mode('w');
    config.set_rotation_max_file_size(512);
    config.set_fsync_enabled(true);

    if (defer_periodic_sync)
    {
      config.set_minimum_fsync_interval(std::chrono::hours{1});
    }

    auto sink = Frontend::create_or_get_sink<RotatingFileSink>(filename, config, FileEventNotifier{});
    Logger* logger = Frontend::create_or_get_logger(name, std::move(sink), pattern_formatter_options);

    std::string const record(300, 'a');
    LOG_INFO(logger, "{}", record);
    manual_backend_worker->poll();

    failed_syncs = 0;
    sync_fails = true;
    sync_failure_duration = std::chrono::milliseconds{1050};

    // A record that waits in the write buffer, so that the flush before the rotation writes it
    // and syncs the file, unless minimum_fsync_interval defers that
    LOG_INFO(logger, "{}", std::string(100, 'b'));
    manual_backend_worker->poll_one();

    // Each record is due for a rotation by size
    for (size_t i = 0; i < 4; ++i)
    {
      LOG_INFO(logger, "{}", record);
      manual_backend_worker->poll_one();
    }

    sync_fails = false;
    sync_failure_duration = std::chrono::milliseconds{0};

    REQUIRE_EQ(failed_syncs, 1u);
    REQUIRE_FALSE(fs::exists(backup_filename));

    // After the retry interval of one second, the rotation that is still due succeeds
    std::this_thread::sleep_for(std::chrono::milliseconds{1100});
    LOG_INFO(logger, "record after");
    manual_backend_worker->poll();

    REQUIRE(fs::exists(backup_filename));
    REQUIRE_EQ(read_file_bytes(filename), "record after\n");

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
    testing::remove_file(filename);
    testing::remove_file(backup_filename);
  }

  #if defined(QUILL_NO_EXCEPTIONS)
  for (size_t const buffer_size : {FileSinkConfig::default_write_buffer_size, size_t{0}})
  {
    std::string const name = "rotating_sink_io_failure_retry_" + std::to_string(buffer_size);
    fs::path const filename = name + ".log";
    fs::path const backup_filename = name + ".1.log";
    fs::remove(filename);
    fs::remove(backup_filename);

    // A failed stream flush can leave bytes buffered on some platforms. On Linux, put bytes
    // back into the stream before fclose() to exercise the same close failure.
    bool leave_bytes_in_stream{true};
    FileEventNotifier notifier;
    notifier.before_close = [&leave_bytes_in_stream](fs::path const&, FILE* file)
    {
      if (leave_bytes_in_stream)
      {
        std::fputs("unwritten bytes", file);
        leave_bytes_in_stream = false;
      }
    };

    RotatingFileSinkConfig config;
    config.set_open_mode('w');
    config.set_rotation_max_file_size(1024);
    config.set_write_buffer_size(buffer_size);

    auto sink = Frontend::create_or_get_sink<RotatingFileSink>(filename, config, notifier);
    Logger* logger = Frontend::create_or_get_logger(name, std::move(sink), pattern_formatter_options);

    std::string const in_file(600, 'a');
    LOG_INFO(logger, "{}", in_file);
    manual_backend_worker->poll();
    REQUIRE_EQ(read_file_bytes(filename), in_file + "\n");

    LOG_INFO(logger, "{}", std::string(300, 'b'));
    manual_backend_worker->poll_one();
    LOG_INFO(logger, "{}", std::string(600, 'c'));

    {
      FileSizeLimit const file_size_limit{in_file.size() + 1};
      manual_backend_worker->poll_one();
      REQUIRE_FALSE(leave_bytes_in_stream);
      REQUIRE_FALSE(fs::exists(backup_filename));

      manual_backend_worker->poll();
      REQUIRE_EQ(read_file_bytes(filename), in_file + "\n");
    }

    // The next record opens the file again without truncating it, and is written to it. The
    // rotation is retried after the retry interval of one second
    LOG_INFO(logger, "record after");
    manual_backend_worker->poll();
    REQUIRE_FALSE(fs::exists(backup_filename));
    REQUIRE_EQ(read_file_bytes(filename), in_file + "\nrecord after\n");

    // Then even a record below the size threshold rotates the file
    std::this_thread::sleep_for(std::chrono::milliseconds{1100});
    LOG_INFO(logger, "record after the interval");
    manual_backend_worker->poll();
    REQUIRE_EQ(read_file_bytes(backup_filename), in_file + "\nrecord after\n");
    REQUIRE_EQ(read_file_bytes(filename), "record after the interval\n");

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
    testing::remove_file(filename);
    testing::remove_file(backup_filename);
  }
  #endif

  manual_backend_worker->shutdown();

  for (fs::path const& file : {time_filename, time_backup_filename, size_filename, size_backup_filename,
                               stream_filename, stream_backup_filename, reopen_filename,
                               reopen_backup_filename, close_filename, close_backup_filename})
  {
    testing::remove_file(file);
  }
#else
  return;
#endif
}
