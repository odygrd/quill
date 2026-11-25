#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/RotatingFileSink.h"

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <new>
#include <string>
#include <string_view>

#if defined(__linux__)
  #include <csignal>
  #include <sys/resource.h>
  #include <sys/syscall.h>
  #include <sys/types.h>
  #include <unistd.h>

namespace
{
// The number of the next calls to write() that are disturbed. The last one of them fails as it
// does when the disk is full, the one before it writes ten bytes only, and the one before that
// is interrupted before it writes anything
int disturbed_writes{0};

// Set to fail the first allocation that follows the write that the full disk fails
bool fail_allocation_after_full_disk{false};

// Set to fail the next allocation
bool fail_next_allocation{false};

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
      if (fail_allocation_after_full_disk)
      {
        fail_allocation_after_full_disk = false;
        fail_next_allocation = true;
      }

      errno = ENOSPC;
      return -1;
    }

    count = (count < 10) ? count : 10;
  }

  return static_cast<ssize_t>(::syscall(SYS_write, file_descriptor, buffer, count));
}

  // ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced.
  #if !defined(QUILL_NO_EXCEPTIONS) && !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
// This executable replaces global operator new to simulate a single allocation failure without
// exhausting memory. Other test executables use their normal allocator.
void* operator new(std::size_t size)
{
  if (fail_next_allocation)
  {
    // Consume the flag before throwing, so that the test can allocate again
    fail_next_allocation = false;
    throw std::bad_alloc{};
  }

  return ::operator new(size, std::align_val_t{alignof(std::max_align_t)});
}

void operator delete(void* memory) noexcept
{
  ::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});
}
void operator delete(void* memory, std::size_t) noexcept
{
  ::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});
}
  #endif
#endif

/***/
TEST_CASE("file_sink_failed_write")
{
#if defined(__linux__)
  using namespace quill;

  fs::path const filename = "file_sink_failed_write.log";
  fs::path const framed_filename = "file_sink_failed_write_framed.bin";
  fs::path const framed_large_filename = "file_sink_failed_write_framed_large.bin";
  fs::path const removed_filename = "file_sink_failed_write_removed.log";
  fs::path const limited_filename = "file_sink_failed_write_limited.log";
  fs::path const limited_filename_1 = "file_sink_failed_write_limited.1.log";
  fs::path const sized_filename = "file_sink_failed_write_sized.log";
  fs::path const sized_filename_1 = "file_sink_failed_write_sized.1.log";
  fs::path const no_memory_filename = "file_sink_failed_write_no_memory.log";

  testing::remove_file(filename);
  testing::remove_file(no_memory_filename);
  testing::remove_file(framed_filename);
  testing::remove_file(framed_large_filename);
  testing::remove_file(removed_filename);
  testing::remove_file(limited_filename);
  testing::remove_file(limited_filename_1);
  testing::remove_file(sized_filename);
  testing::remove_file(sized_filename_1);

  auto const read_file_bytes = [](fs::path const& file_to_read)
  {
    std::ifstream file{file_to_read, std::ios::binary};
    return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
  };

  auto const write_binary_record = [](FileSink& file_sink, std::string const& bytes)
  {
    file_sink.write_binary(BinaryRecord{nullptr, 0, std::string_view{}, std::string_view{},
                                        std::string_view{}, std::string_view{},
                                        reinterpret_cast<std::byte const*>(bytes.data()), bytes.size()});
  };

  // The bytes of a record after its size, as BinaryRecordFraming::Size writes them
  auto const framed = [](std::string const& bytes)
  {
    uint32_t const size = static_cast<uint32_t>(bytes.size());
    std::string framed_bytes(sizeof(size), '\0');
    std::memcpy(framed_bytes.data(), &size, sizeof(size));
    return framed_bytes + bytes;
  };

  std::string const large_record(16384, 'l');
  std::string const record_after_recovery{"record after the recovery"};
  std::string const record_larger_than_buffer(8192, 'b');

  // A record larger than the buffer is written past it. What the full file does not take of it
  // is kept and written before the next record, so the frames still follow each other. With the
  // buffer disabled, every record is written through the stream, which the sink opens unbuffered
  // for framed records: a buffered one can discard a part of the record that it counted as taken
  for (size_t const write_buffer_size : {size_t{4096}, size_t{0}})
  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_write_buffer_size(write_buffer_size);
    fsc.set_binary_record_framing(BinaryRecordFraming::Size);

    {
      FileSink file_sink{framed_large_filename, fsc};

      {
        FileSizeLimit const file_size_limit{4096};

  #if defined(QUILL_NO_EXCEPTIONS)
        // Outside a poll of the backend, a failure goes to the receiver of this thread
        size_t write_errors{0};
        detail::ErrorReceiver const receiver{
          [](void* context, char const*) { ++*static_cast<size_t*>(context); }, &write_errors};

        detail::error_receiver() = &receiver;
        write_binary_record(file_sink, large_record);
        detail::error_receiver() = nullptr;

        REQUIRE_EQ(write_errors, 1u);
  #else
        REQUIRE_THROWS_AS(write_binary_record(file_sink, large_record), QuillError);
  #endif

        REQUIRE_EQ(fs::file_size(framed_large_filename), 4096u);
      }

      write_binary_record(file_sink, record_after_recovery);
      file_sink.flush_sink(SinkFlushReason::Explicit);

      // The buffer that grew for the rest of the large record is back at its size, so a record
      // larger than that is written to the file at once again
      write_binary_record(file_sink, record_larger_than_buffer);
      REQUIRE_EQ(fs::file_size(framed_large_filename),
                 framed(large_record).size() + framed(record_after_recovery).size() +
                   framed(record_larger_than_buffer).size());
    }

    REQUIRE(read_file_bytes(framed_large_filename) ==
            framed(large_record) + framed(record_after_recovery) + framed(record_larger_than_buffer));

    testing::remove_file(framed_large_filename);
  }

  #if !defined(QUILL_NO_EXCEPTIONS)
  auto const write_log_statement_at = [](FileSink& file_sink, uint64_t timestamp, std::string_view log_statement)
  {
    file_sink.write_log(LogRecord{nullptr,
                                  timestamp,
                                  std::string_view{},
                                  std::string_view{},
                                  std::string_view{},
                                  std::string_view{},
                                  LogLevel::Info,
                                  "INFO",
                                  "I",
                                  nullptr,
                                  "",
                                  log_statement,
                                  {}});
  };

  auto const write_log_statement = [&write_log_statement_at](FileSink& file_sink, std::string_view log_statement)
  { write_log_statement_at(file_sink, 0, log_statement); };

  std::string expected_contents;

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    FileSink file_sink{filename, fsc};

    for (int i = 0; i < 100; ++i)
    {
      std::string const log_statement = "record " + std::to_string(i) + "\n";
      write_log_statement(file_sink, log_statement);
      expected_contents += log_statement;
    }

    // The flush is interrupted, then ten bytes are accepted, and then the disk is full
    disturbed_writes = 3;
    REQUIRE_THROWS_AS(file_sink.flush_sink(SinkFlushReason::Explicit), QuillError);
    REQUIRE_EQ(disturbed_writes, 0);
    REQUIRE_EQ(read_file_bytes(filename), expected_contents.substr(0, 10));

    // The sink keeps what is not in the file. The next flush writes it after the ten bytes, with
    // the records that were written in between, so every record is in the file once
    std::string const log_statement{"record after the failure\n"};
    write_log_statement(file_sink, log_statement);
    expected_contents += log_statement;

    file_sink.flush_sink(SinkFlushReason::Explicit);
    REQUIRE_EQ(read_file_bytes(filename), expected_contents);
  }

  REQUIRE_EQ(read_file_bytes(filename), expected_contents);

    #if !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
  {
    // The message of a failed write needs memory. When there is none, what the file took has
    // still left the buffer of the sink, so the next flush does not write those bytes again
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    FileSink file_sink{no_memory_filename, fsc};

    std::string const log_statement(150, 'm');
    write_log_statement(file_sink, log_statement);

    // Ten bytes are accepted, then the disk is full, and then the message gets no memory
    disturbed_writes = 2;
    fail_allocation_after_full_disk = true;
    REQUIRE_THROWS_AS(file_sink.flush_sink(SinkFlushReason::Explicit), std::bad_alloc);
    REQUIRE_FALSE(fail_allocation_after_full_disk);
    REQUIRE_FALSE(fail_next_allocation);
    REQUIRE_EQ(read_file_bytes(no_memory_filename), log_statement.substr(0, 10));

    file_sink.flush_sink(SinkFlushReason::Explicit);
    REQUIRE_EQ(read_file_bytes(no_memory_filename), log_statement);
  }
    #endif

  // A framed binary record that the full buffer has no room for is dropped as a whole when the
  // buffer cannot be written to the file. Its frame is not kept without its bytes
  std::string const record(1000, 'r');

  {
    FileSinkConfig fsc;
    fsc.set_open_mode('w');
    fsc.set_write_buffer_size(4096);
    fsc.set_binary_record_framing(BinaryRecordFraming::Size);
    FileSink file_sink{framed_filename, fsc};

    for (int i = 0; i < 4; ++i)
    {
      write_binary_record(file_sink, record);
    }

    // The frame of the fifth record fits in what is left of the buffer, and its bytes do not
    disturbed_writes = 1;
    REQUIRE_THROWS_AS(write_binary_record(file_sink, record), QuillError);
    REQUIRE_EQ(disturbed_writes, 0);

    write_binary_record(file_sink, record);
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  std::string expected_framed_contents;
  for (int i = 0; i < 5; ++i)
  {
    expected_framed_contents += framed(record);
  }

  REQUIRE(read_file_bytes(framed_filename) == expected_framed_contents);

  {
    // A rotating sink whose file cannot grow any more, e.g. it has reached the maximum size of
    // a file, keeps the records that it cannot write to it. Once a rotation is due it drops
    // them, so that the file can be rotated and the records that follow go to a new file
    auto const start_time = std::chrono::system_clock::now();
    uint64_t const start_timestamp = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count());

    RotatingFileSinkConfig cfg;
    cfg.set_open_mode('w');
    cfg.set_rotation_frequency_and_interval('M', 1);
    RotatingFileSink rotating_sink{limited_filename, cfg, FileEventNotifier{}, start_time};

    std::string const log_statement = std::string(99, 'x') + "\n";
    std::string const log_statement_after{"record after the rotation\n"};

    {
      // The file has room for ten of the twenty records
      FileSizeLimit const file_size_limit{1000};

      for (int i = 0; i < 20; ++i)
      {
        write_log_statement_at(rotating_sink, start_timestamp, log_statement);
      }

      REQUIRE_THROWS_AS(rotating_sink.flush_sink(SinkFlushReason::Explicit), QuillError);
      REQUIRE_EQ(fs::file_size(limited_filename), 1000u);

      // Two minutes later a rotation is due. Its flush fails as the one before, which is
      // reported, and the next record rotates the file and goes to the new one
      uint64_t const two_minutes{120'000'000'000ull};
      REQUIRE_THROWS_AS(
        write_log_statement_at(rotating_sink, start_timestamp + two_minutes, log_statement), QuillError);
      write_log_statement_at(rotating_sink, start_timestamp + two_minutes, log_statement_after);
      rotating_sink.flush_sink(SinkFlushReason::Explicit);
    }

    REQUIRE_EQ(fs::file_size(limited_filename_1), 1000u);
    REQUIRE_EQ(read_file_bytes(limited_filename), log_statement_after);

    // A sink that rotates by size counts the records that its file cannot take, so it reaches
    // its rotation also when the file stays far below that size
    RotatingFileSinkConfig sized_cfg;
    sized_cfg.set_open_mode('w');
    sized_cfg.set_write_buffer_size(4096);
    sized_cfg.set_rotation_max_file_size(10 * 1024);
    RotatingFileSink sized_sink{sized_filename, sized_cfg};

    {
      // The file has room for 40 records, and the write buffer for the 40 after them
      FileSizeLimit const file_size_limit{4000};

      for (int i = 0; i < 80; ++i)
      {
        write_log_statement(sized_sink, log_statement);
      }

      // Every later record is refused. 22 of them bring the counted size to 10200 bytes, and
      // the one after them is due for the rotation, whose flush fails as well
      for (int i = 0; i < 23; ++i)
      {
        REQUIRE_THROWS_AS(write_log_statement(sized_sink, log_statement), QuillError);
      }

      // The rotation stays due for the next record whatever its size, also for one without bytes
      write_binary_record(sized_sink, std::string{});
      REQUIRE(fs::exists(sized_filename_1));

      write_log_statement(sized_sink, log_statement_after);
      sized_sink.flush_sink(SinkFlushReason::Explicit);
    }

    REQUIRE_EQ(fs::file_size(sized_filename_1), 4000u);
    REQUIRE_EQ(read_file_bytes(sized_filename), log_statement_after);
  }

  // A file that is removed, e.g. to make room on the full disk, is replaced also when the flush
  // fails. Its space is only free when the sink has closed it
  {
    FileSink file_sink{removed_filename};
    write_log_statement(file_sink, "record before\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);

    write_log_statement(file_sink, "record that cannot be written\n");
    REQUIRE(fs::remove(removed_filename));

    disturbed_writes = 1;
    REQUIRE_THROWS_AS(file_sink.flush_sink(SinkFlushReason::Explicit), QuillError);
    REQUIRE_EQ(disturbed_writes, 0);
    REQUIRE(fs::exists(removed_filename));

    write_log_statement(file_sink, "record after\n");
    file_sink.flush_sink(SinkFlushReason::Explicit);
  }

  REQUIRE_EQ(read_file_bytes(removed_filename), "record after\n");
  #endif

  testing::remove_file(filename);
  testing::remove_file(framed_filename);
  testing::remove_file(framed_large_filename);
  testing::remove_file(removed_filename);
  testing::remove_file(limited_filename);
  testing::remove_file(limited_filename_1);
  testing::remove_file(sized_filename);
  testing::remove_file(sized_filename_1);
  testing::remove_file(no_memory_filename);
#else
  return;
#endif
}
