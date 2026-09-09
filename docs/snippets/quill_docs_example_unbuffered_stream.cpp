#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/StreamSink.h"

#include <cstdio>
#include <memory>
#include <optional>

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(push)
  #pragma warning(disable : 4996)
#endif

struct FileCloser
{
  void operator()(FILE* file) const noexcept { std::fclose(file); }
};

int main()
{
  // StreamSink accepts an existing FILE*. The application opens and owns it.
  std::unique_ptr<FILE, FileCloser> stream{std::fopen("unbuffered_stream.log", "w")};
  if (!stream)
  {
    std::perror("fopen");
    return 1;
  }

  // Configure the fresh stream before any reads or writes, including logging.
  if (std::setvbuf(stream.get(), nullptr, _IONBF, 0) != 0)
  {
    std::fputs("Failed to disable stream buffering\n", stderr);
    return 1;
  }

  quill::Backend::start();

  // This flag confirms the successful _IONBF setup above; it does not change buffering.
  constexpr bool stream_is_unbuffered{true};
  auto sink = quill::Frontend::create_or_get_sink<quill::StreamSink>(
    "unbuffered_stream_sink", // Sink registry name
    "unbuffered_stream.log",  // Filename/label; StreamSink does not open it
    stream.get(), std::nullopt, quill::FileEventNotifier{}, stream_is_unbuffered);

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "unbuffered_stream_logger", sink, quill::PatternFormatterOptions{"%(message)"});
  LOG_INFO(logger, "Logging through a caller-owned unbuffered stream");

  // Keep the FILE* alive until the backend has finished using it.
  quill::Frontend::remove_logger_blocking(logger);
  quill::Backend::stop();
  stream.reset(); // The application closes its stream.
}

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif
