#define FUZZER_LOG_FILENAME "json_structured_logging_fuzz_root.log"
#define FUZZER_IMMEDIATE_FLUSH_LIMIT 64
#include "FuzzerHelper.h"

#include "FuzzDataExtractor.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/JsonSink.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace
{
class VerboseJsonFileSink : public quill::JsonFileSink
{
public:
  using quill::JsonFileSink::JsonFileSink;

  void generate_json_message(quill::MacroMetadata const* log_metadata, uint64_t log_timestamp,
                             std::string_view thread_id, std::string_view, std::string const&,
                             std::string_view logger_name, quill::LogLevel,
                             std::string_view log_level_description, std::string_view,
                             std::vector<std::pair<std::string, std::string>> const* named_args,
                             std::string_view log_message, std::string_view log_statement,
                             char const* message_format) override
  {
    this->_json_message.append(std::string_view{"{\"timestamp\":\""});
    _append_json_escaped(this->_json_message, std::to_string(log_timestamp));
    this->_json_message.append(std::string_view{"\",\"thread_id\":\""});
    _append_json_escaped(this->_json_message, thread_id);
    this->_json_message.append(std::string_view{"\",\"logger\":\""});
    _append_json_escaped(this->_json_message, logger_name);
    this->_json_message.append(std::string_view{"\",\"level\":\""});
    _append_json_escaped(this->_json_message, log_level_description);
    this->_json_message.append(std::string_view{"\",\"format\":\""});
    _append_json_escaped(this->_json_message, message_format);
    this->_json_message.append(std::string_view{"\",\"statement\":\""});
    _append_json_escaped(this->_json_message, log_statement);
    this->_json_message.append(std::string_view{"\",\"message\":\""});
    _append_json_escaped(this->_json_message, log_message);
    this->_json_message.append(std::string_view{"\",\"file\":\""});
    _append_json_escaped(this->_json_message, log_metadata->file_name());
    this->_json_message.append(std::string_view{"\""});

    if (named_args)
    {
      for (auto const& [key, value] : *named_args)
      {
        this->_json_message.append(std::string_view{",\""});
        _append_json_escaped(this->_json_message, key);
        this->_json_message.append(std::string_view{"\":\""});
        _append_json_escaped(this->_json_message, value);
        this->_json_message.append(std::string_view{"\""});
      }
    }
  }
};

std::atomic<bool> g_json_loggers_initialized{false};
quill::Logger* g_json_logger{nullptr};
quill::Logger* g_verbose_json_logger{nullptr};

void ensure_json_loggers()
{
  if (g_json_loggers_initialized.load(std::memory_order_acquire))
  {
    return;
  }

  auto json_sink = quill::Frontend::create_or_get_sink<quill::JsonFileSink>(
    "json_structured_logging_fuzz.json",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::None);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  auto verbose_json_sink = quill::Frontend::create_or_get_sink<VerboseJsonFileSink>(
    "json_structured_logging_verbose_fuzz.json",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::None);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  quill::PatternFormatterOptions pattern_options;
  pattern_options.format_pattern = "%(message)";
  pattern_options.add_metadata_to_multi_line_logs = false;
  pattern_options.pattern_suffix = quill::PatternFormatterOptions::NO_SUFFIX;

  g_json_logger = quill::Frontend::create_or_get_logger("json_structured_logging",
                                                        std::move(json_sink), pattern_options);
  g_verbose_json_logger = quill::Frontend::create_or_get_logger(
    "json_structured_logging_verbose", std::move(verbose_json_sink), pattern_options);

  g_json_logger->set_log_level(quill::LogLevel::TraceL3);
  g_verbose_json_logger->set_log_level(quill::LogLevel::TraceL3);
  g_json_logger->set_immediate_flush(64);
  g_verbose_json_logger->set_immediate_flush(64);

  g_json_loggers_initialized.store(true, std::memory_order_release);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 4)
  {
    return 0;
  }

  ensure_json_loggers();

  FuzzDataExtractor extractor{data, size};

  std::string symbol = extractor.get_bytes(static_cast<size_t>(extractor.get_byte() % 48u));
  std::string venue = extractor.get_bytes(static_cast<size_t>(extractor.get_byte() % 24u));
  std::string note = extractor.get_bytes(static_cast<size_t>(extractor.get_byte() % 96u));
  std::string trader = extractor.get_bytes(static_cast<size_t>(extractor.get_byte() % 40u));
  std::string escaped = extractor.get_bytes(static_cast<size_t>(extractor.get_byte() % 64u));
  int32_t qty = extractor.get_int32();
  uint64_t order_id = extractor.get_uint64();
  double price = extractor.get_double();

  switch (extractor.get_byte() % 6u)
  {
  case 0:
    LOG_INFO(g_json_logger, "trade {symbol} {venue} {note} {qty} {price}", symbol, venue, note, qty, price);
    break;
  case 1:
    LOG_WARNING(g_json_logger, "multi\nline {trader} {venue} {order_id}", trader, venue, order_id);
    break;
  case 2:
    LOG_INFO(g_verbose_json_logger, "verbose {symbol} {note} {escaped} {qty} {price} {order_id}",
             symbol, note, escaped, qty, price, order_id);
    break;
  case 3:
    LOGJ_INFO(g_json_logger, "jsonj", symbol, venue, qty, price);
    break;
  case 4:
    for (uint32_t i = 0; i < (extractor.get_byte() % 8u + 1u); ++i)
    {
      if ((i % 3u) == 0u)
      {
        LOG_INFO(g_json_logger, "no params");
      }
      else if ((i % 3u) == 1u)
      {
        LOG_INFO(g_json_logger, "single {symbol}", symbol);
      }
      else
      {
        LOG_INFO(g_json_logger, "pair {symbol} {escaped}", symbol, escaped);
      }
    }
    break;
  case 5:
    LOG_DYNAMIC(g_verbose_json_logger, static_cast<quill::LogLevel>(extractor.get_byte() % 9u),
                "dynamic {} {} {}", symbol, venue, note);
    break;
  }

  return 0;
}
