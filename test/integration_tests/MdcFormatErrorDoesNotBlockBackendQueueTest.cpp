#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/format.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

using namespace quill;

#if !defined(QUILL_NO_EXCEPTIONS)
struct MdcFormatterThrows
{
};

template <>
struct fmtquill::formatter<MdcFormatterThrows>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(MdcFormatterThrows const&, FormatContext& ctx) const -> decltype(ctx.out())
  {
    throw std::runtime_error{"mdc_formatter_throws"};
  }
};

template <>
struct quill::Codec<MdcFormatterThrows> : quill::DeferredFormatCodec<MdcFormatterThrows>
{
};
#endif

TEST_CASE("mdc_format_error_does_not_block_backend_queue")
{
#if defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  static constexpr char const* filename = "mdc_format_error_does_not_block_backend_queue.log";

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger("logger_mdc_format_error", std::move(file_sink));

  std::atomic<size_t> error_notifier_invoked{0};

  BackendOptions backend_options;
  backend_options.error_notifier = [&error_notifier_invoked](std::string const&)
  { error_notifier_invoked.fetch_add(1, std::memory_order_relaxed); };

  Backend::start(backend_options);

  logger->set_mdc("bad", MdcFormatterThrows{});
  LOG_INFO(logger, "valid_message_after_mdc_error");

  logger->flush_log();
  Frontend::remove_logger_blocking(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_GE(error_notifier_invoked.load(std::memory_order_relaxed), 1);
  REQUIRE(quill::testing::file_contains(file_contents, "valid_message_after_mdc_error"));

  testing::remove_file(filename);
#endif
}
