#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/StreamSink.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>

#if defined(__GLIBC__) && !defined(QUILL_NO_EXCEPTIONS)
namespace
{
struct FileCloser
{
  void operator()(FILE* file) const noexcept { std::fclose(file); }
};

struct BufferedInterruptedStream
{
  std::string received;
  bool interrupted{false};
};

ssize_t write_buffered_stream(void* cookie, char const* data, size_t count)
{
  auto& output = *static_cast<BufferedInterruptedStream*>(cookie);
  if (!output.interrupted)
  {
    output.interrupted = true;
    errno = EINTR;
    return 0;
  }

  output.received.append(data, count);
  return static_cast<ssize_t>(count);
}
} // namespace
#endif

TEST_CASE("stream_sink_reports_interruption_while_flushing_a_partial_buffer")
{
#if defined(__GLIBC__) && !defined(QUILL_NO_EXCEPTIONS)
  using namespace quill;

  BufferedInterruptedStream output;
  char buffer[4096];
  cookie_io_functions_t functions{};
  functions.write = write_buffered_stream;
  std::unique_ptr<FILE, FileCloser> stream{fopencookie(&output, "w", functions)};
  REQUIRE_NE(stream.get(), nullptr);
  REQUIRE_EQ(std::setvbuf(stream.get(), buffer, _IOFBF, sizeof(buffer)), 0);

  auto sink = Frontend::create_or_get_sink<StreamSink>(
    "stream_sink_buffered_interrupted_write_sink", "stream_sink_buffered_interrupted_write_stream",
    stream.get());
  auto* logger = Frontend::create_or_get_logger("stream_sink_buffered_interrupted_write_logger",
                                                sink, PatternFormatterOptions{"%(message)"});

  // The second record fills the partial buffer. glibc discards that buffer on
  // EINTR but includes newly buffered bytes in fwrite's returned count.
  LOG_INFO(logger, "prior");
  LOG_INFO(logger, "{}", std::string(8192, 'x'));
  LOG_INFO(logger, "after interruption");

  size_t error_count{0};
  std::string last_error;
  BackendOptions options;
  options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
  options.error_notifier = [&](std::string const& error)
  {
    ++error_count;
    last_error = error;
  };

  ManualBackendWorker* backend = Backend::acquire_manual_backend_worker();
  backend->init(options);
  backend->poll();
  Frontend::remove_logger(logger);
  backend->shutdown();
  stream.reset();

  REQUIRE(output.interrupted);
  REQUIRE_EQ(error_count, 1);
  REQUIRE_NE(last_error.find("fwrite failed errno: " + std::to_string(EINTR)), std::string::npos);
  REQUIRE_EQ(output.received, "after interruption\n");
#else
  return;
#endif
}
