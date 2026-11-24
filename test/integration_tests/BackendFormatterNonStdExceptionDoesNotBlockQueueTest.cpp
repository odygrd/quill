#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace quill;

#if !defined(QUILL_NO_EXCEPTIONS)
namespace
{
struct NonStdThrowingFormat
{
  int value{0};
};

/**
 * Has no formatter: it is formatted through its conversion to std::string, which the backend
 * makes when it decodes the record
 */
struct ThrowingConversionOnBackendDecode
{
  explicit ThrowingConversionOnBackendDecode(int in_value) : value(in_value) {}

  // Not trivially copyable, so the object is constructed in the queue
  ThrowingConversionOnBackendDecode(ThrowingConversionOnBackendDecode const& other)
    : value(other.value)
  {
  }

  operator std::string() const { throw std::runtime_error{"backend_decode_throws"}; }

  int value{0};
};
} // namespace

template <>
struct fmtquill::formatter<NonStdThrowingFormat>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(NonStdThrowingFormat const&, format_context& ctx) const -> decltype(ctx.out())
  {
    throw 123;
  }
};

template <>
struct quill::Codec<NonStdThrowingFormat> : quill::DeferredFormatCodec<NonStdThrowingFormat>
{
};

template <>
struct quill::Codec<ThrowingConversionOnBackendDecode>
  : quill::DeferredFormatCodec<ThrowingConversionOnBackendDecode>
{
};
#endif

TEST_CASE("backend_formatter_non_std_exception_does_not_block_queue")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  static constexpr char const* filename =
    "backend_formatter_non_std_exception_does_not_block_queue.log";
  static std::string const logger_name = "backend_formatter_non_std_exception_logger";

  BackendOptions backend_options;
  std::string formatter_error;
  backend_options.error_notifier = [&formatter_error](std::string const& error_message)
  { formatter_error = error_message; };
  Backend::start(backend_options);

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  LOG_INFO(logger, "bad {}", NonStdThrowingFormat{1});
  ThrowingConversionOnBackendDecode throwing_decode{7};
  LOG_INFO(logger, "decode {}", throwing_decode);
  LOG_INFO(logger, "good {}", 42);

  logger->flush_log();
  Frontend::remove_logger_blocking(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);

  REQUIRE(formatter_error.find("Caught unhandled exception.") != std::string::npos);
  REQUIRE(testing::file_contains(file_contents, "Could not format log statement"));
  REQUIRE(testing::file_contains(file_contents, "decode [Quill deferred decode failed]"));
  REQUIRE(testing::file_contains(file_contents, "good 42"));

  testing::remove_file(filename);
#else
  return;
#endif
}
