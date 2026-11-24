#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/RotatingFileSink.h"
#include "quill/sinks/StreamSink.h"

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(__linux__)
  #include <fcntl.h>
  #include <sys/stat.h>
  #include <sys/syscall.h>
  #include <sys/types.h>
  #include <unistd.h>

namespace
{
// The number of the next calls to write() that are disturbed. The last one of them fails as it
// does when the disk is full, the one before it writes ten bytes only, and the one before that
// is interrupted before it writes anything
int disturbed_writes{0};
} // namespace

// This executable replaces write() to simulate a signal, a short write and a full disk while the
// sink writes to the file. Other test executables use the one of the C library.
extern "C" ssize_t write(int file_descriptor, void const* buffer, size_t count)
{
  if (disturbed_writes > 0)
  {
    --disturbed_writes;

    if (disturbed_writes == 2)
    {
      errno = EINTR;
      return -1;
    }

    if (disturbed_writes == 0)
    {
      errno = ENOSPC;
      return -1;
    }

    count = (count < 10) ? count : 10;
  }

  return static_cast<ssize_t>(::syscall(SYS_write, file_descriptor, buffer, count));
}
#endif

/** Lets the test close the file */
class SinkIoFailureNotifierClosableFileSink : public quill::FileSink
{
public:
  using quill::FileSink::close_file;
  using quill::FileSink::FileSink;
};

/**
 * The I/O failures of a sink are reported through error_notifier, and the backend keeps running.
 * Without exceptions they must not end the process either.
 */
TEST_CASE("sink_io_failure_notifier")
{
#if defined(__linux__)
  using namespace quill;

  fs::path const filename = "sink_io_failure_notifier.log";
  fs::path const removed_directory = "sink_io_failure_notifier_removed";
  fs::path const removed_filename = removed_directory / "removed.log";
  fs::path const rotated_filename = removed_directory / "rotated.log";
  fs::path const pipe_filename = "sink_io_failure_notifier.pipe";
  fs::path const held_filename = "sink_io_failure_notifier_held.log";
  fs::path const footer_filename = "sink_io_failure_notifier_footer.log";

  testing::remove_file(filename);
  testing::remove_file(held_filename);
  testing::remove_file(footer_filename);
  testing::remove_file(pipe_filename);
  fs::remove_all(removed_directory);
  fs::create_directories(removed_directory);

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

  // This thread runs the backend, so that each poll below makes one attempt to flush the sinks
  BackendOptions backend_options;
  backend_options.error_notifier = [&errors](std::string const& error) { errors.push_back(error); };
  backend_options.sink_min_flush_interval = std::chrono::milliseconds{0};
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(backend_options);

  FileSinkConfig file_sink_config;
  file_sink_config.set_open_mode('w');

  PatternFormatterOptions const pattern_formatter_options{"%(message)"};

  {
    // A flush that the full disk fails keeps its records, and a later flush writes them
    auto file_sink = Frontend::create_or_get_sink<FileSink>(filename, file_sink_config, FileEventNotifier{});
    Logger* logger = Frontend::create_or_get_logger(
      "sink_io_failure_notifier_file", std::move(file_sink), pattern_formatter_options);

    std::string expected_contents;

    for (int i = 0; i < 100; ++i)
    {
      LOG_INFO(logger, "record {}", i);
      expected_contents += "record " + std::to_string(i) + "\n";
    }

    // The flush is interrupted, then ten bytes are accepted, and then the disk is full
    disturbed_writes = 3;
    manual_backend_worker->poll();

    REQUIRE_EQ(disturbed_writes, 0);
    REQUIRE_EQ(count_errors_with("write failed errno: " + std::to_string(ENOSPC)), 1u);
    REQUIRE_EQ(read_file_bytes(filename), expected_contents.substr(0, 10));

    LOG_INFO(logger, "record after the failure");
    expected_contents += "record after the failure\n";
    manual_backend_worker->poll();

    REQUIRE_EQ(read_file_bytes(filename), expected_contents);
    REQUIRE_EQ(errors.size(), 1u);

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
  }

  {
    // A file whose directory was removed cannot be opened again until the directory is back
    auto file_sink =
      Frontend::create_or_get_sink<FileSink>(removed_filename, file_sink_config, FileEventNotifier{});
    Logger* logger = Frontend::create_or_get_logger(
      "sink_io_failure_notifier_removed", std::move(file_sink), pattern_formatter_options);

    LOG_INFO(logger, "record before");
    manual_backend_worker->poll();
    REQUIRE_EQ(read_file_bytes(removed_filename), "record before\n");

    fs::remove_all(removed_directory);
    LOG_INFO(logger, "record of the removed file");
    manual_backend_worker->poll();
    REQUIRE_EQ(count_errors_with("fopen failed"), 1u);

    fs::create_directories(removed_directory);
    manual_backend_worker->poll();

    LOG_INFO(logger, "record after");
    manual_backend_worker->poll();
    REQUIRE_EQ(read_file_bytes(removed_filename), "record after\n");

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
  }

  {
    // A rotation flushes the file first. The flush finds the file removed and closes it, and it
    // cannot open the file again, as the directory was removed too. The rotation ends there
    RotatingFileSinkConfig rotating_sink_config;
    rotating_sink_config.set_open_mode('w');
    rotating_sink_config.set_rotation_max_file_size(1024);

    auto rotating_sink = Frontend::create_or_get_sink<RotatingFileSink>(
      rotated_filename, rotating_sink_config, FileEventNotifier{});
    Logger* logger = Frontend::create_or_get_logger(
      "sink_io_failure_notifier_rotated", std::move(rotating_sink), pattern_formatter_options);

    // The record stays in the write buffer of the sink: poll_one() does not flush
    LOG_INFO(logger, "{}", std::string(600, 'a'));
    manual_backend_worker->poll_one();

    // Another record of this size does not fit in the file, so it is rotated
    size_t const open_errors = count_errors_with("fopen failed");
    fs::remove_all(removed_directory);
    LOG_INFO(logger, "{}", std::string(600, 'b'));
    manual_backend_worker->poll();
    REQUIRE_EQ(count_errors_with("fopen failed"), open_errors + 1);

    fs::create_directories(removed_directory);
    LOG_INFO(logger, "record after");
    manual_backend_worker->poll();
    REQUIRE_EQ(read_file_bytes(rotated_filename), "record after\n");

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
  }

  {
    // A binary record with a frame is written through the stream, to a pipe that has no room
    // for the frame but has room for the bytes of the record. They are not written without it
    REQUIRE_EQ(::mkfifo(pipe_filename.c_str(), 0600), 0);

    // The pipe is opened for reading first: the sink would wait for a reader otherwise
    int const read_descriptor = ::open(pipe_filename.c_str(), O_RDONLY | O_NONBLOCK);
    REQUIRE_NE(read_descriptor, -1);

    FileSinkConfig pipe_sink_config;
    pipe_sink_config.set_write_buffer_size(0);
    pipe_sink_config.set_binary_record_framing(BinaryRecordFraming::TimestampAndSize);

    FileEventNotifier pipe_notifier;
    pipe_notifier.after_open = [](fs::path const&, FILE* file)
    {
      // Each write goes to the pipe as it is made, and fails when the pipe has no room for it
      std::setvbuf(file, nullptr, _IONBF, 0);
      int const descriptor = fileno(file);
      ::fcntl(descriptor, F_SETFL, ::fcntl(descriptor, F_GETFL) | O_NONBLOCK);
    };

    auto pipe_sink = Frontend::create_or_get_sink<FileSink>(pipe_filename, pipe_sink_config, pipe_notifier);
    Logger* logger = Frontend::create_or_get_logger(
      "sink_io_failure_notifier_pipe", std::move(pipe_sink), pattern_formatter_options);

    // Fill the pipe up to the last eight bytes of its last page
    int const fill_descriptor = ::open(pipe_filename.c_str(), O_WRONLY | O_NONBLOCK);
    REQUIRE_NE(fill_descriptor, -1);

    size_t const page_size = static_cast<size_t>(::sysconf(_SC_PAGESIZE));
    size_t const pipe_capacity = static_cast<size_t>(::fcntl(fill_descriptor, F_GETPIPE_SZ));
    std::string const page(page_size, 'p');
    size_t filled_bytes{0};

    while (filled_bytes + page_size < pipe_capacity)
    {
      REQUIRE_EQ(::syscall(SYS_write, fill_descriptor, page.data(), page_size), static_cast<long>(page_size));
      filled_bytes += page_size;
    }

    size_t const room{8};
    REQUIRE_EQ(::syscall(SYS_write, fill_descriptor, page.data(), page_size - room),
               static_cast<long>(page_size - room));
    filled_bytes += page_size - room;

    // The frame has twelve bytes, and the record has four
    size_t const write_errors = count_errors_with("fwrite failed errno: " + std::to_string(EAGAIN));
    char const bytes[4] = {'b', 'y', 't', 'e'};
    LOG_BINARY(logger, bytes, sizeof(bytes));
    manual_backend_worker->poll();
    REQUIRE_EQ(count_errors_with("fwrite failed errno: " + std::to_string(EAGAIN)), write_errors + 1);

    std::string pipe_contents(pipe_capacity, '\0');
    ssize_t const read_bytes = ::read(read_descriptor, pipe_contents.data(), pipe_contents.size());
    REQUIRE_EQ(read_bytes, static_cast<ssize_t>(filled_bytes));

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
    ::close(fill_descriptor);
    ::close(read_descriptor);
  }

  #if defined(QUILL_NO_EXCEPTIONS)
  {
    // Without exceptions a flush that fails at the close of the file returns. What it did not
    // write is dropped, so that it does not follow the footer that before_close writes
    FileEventNotifier footer_notifier;
    footer_notifier.before_close = [](fs::path const&, FILE* file)
    {
      std::fputs("footer\n", file);
      std::fflush(file);
    };

    SinkIoFailureNotifierClosableFileSink footer_sink{footer_filename, file_sink_config, footer_notifier};
    footer_sink.write_log(LogRecord{
      nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "buffered record\n", {}});

    // Outside a poll of the backend, a failure goes to the receiver of this thread
    std::vector<std::string> close_errors;
    detail::ErrorReceiver const receiver{
      [](void* context, char const* message)
      { static_cast<std::vector<std::string>*>(context)->emplace_back(message); },
      &close_errors};

    disturbed_writes = 1;
    detail::error_receiver() = &receiver;
    bool const closed = footer_sink.close_file();
    detail::error_receiver() = nullptr;

    REQUIRE_FALSE(closed);
    REQUIRE_EQ(disturbed_writes, 0);
    REQUIRE_EQ(close_errors.size(), 1u);
    REQUIRE_EQ(read_file_bytes(footer_filename), "footer\n");
  }
  #endif

  {
    // A stream that fails to flush
    FILE* full_device = std::fopen("/dev/full", "w");
    REQUIRE(full_device);

    auto stream_sink = Frontend::create_or_get_sink<StreamSink>(
      "sink_io_failure_notifier_stream", fs::path{"sink_io_failure_notifier_stream"}, full_device);
    Logger* logger = Frontend::create_or_get_logger(
      "sink_io_failure_notifier_stream", std::move(stream_sink), pattern_formatter_options);

    LOG_INFO(logger, "record that the device has no room for");
    manual_backend_worker->poll();
    REQUIRE_EQ(count_errors_with("fflush failed errno: " + std::to_string(ENOSPC)), 1u);

    Frontend::remove_logger(logger);
    manual_backend_worker->poll();
    std::fclose(full_device);
  }

  // A sink that the application still holds when the backend has stopped. The disk is full
  // while the backend makes its last flush, so the record stays in the write buffer of the sink
  std::shared_ptr<Sink> held_sink =
    Frontend::create_or_get_sink<FileSink>(held_filename, file_sink_config, FileEventNotifier{});
  Logger* held_logger =
    Frontend::create_or_get_logger("sink_io_failure_notifier_held", held_sink, pattern_formatter_options);

  LOG_INFO(held_logger, "record that the full disk does not take");
  Frontend::remove_logger(held_logger);

  size_t const write_errors = count_errors_with("write failed errno: " + std::to_string(ENOSPC));
  disturbed_writes = 1;
  manual_backend_worker->shutdown();

  REQUIRE_EQ(disturbed_writes, 0);
  REQUIRE_EQ(count_errors_with("write failed errno: " + std::to_string(ENOSPC)), write_errors + 1);

  // The sink is destroyed on this thread, where no backend takes an error any more. It tries to
  // write the record once more and gives it up, as the disk is still full
  disturbed_writes = 1;
  held_sink.reset();

  REQUIRE_EQ(disturbed_writes, 0);
  REQUIRE_EQ(read_file_bytes(held_filename), "");
  REQUIRE_EQ(count_errors_with("write failed errno: " + std::to_string(ENOSPC)), write_errors + 1);

  testing::remove_file(filename);
  testing::remove_file(held_filename);
  testing::remove_file(footer_filename);
  testing::remove_file(pipe_filename);
  fs::remove_all(removed_directory);
#else
  return;
#endif
}
