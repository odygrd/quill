#include "doctest/doctest.h"

#include "quill/core/SinkManager.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/FileSink.h"
#include <cstdio>

TEST_SUITE_BEGIN("SinkManager");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("subscribe_get_active_different_sinks")
{
  std::string file_1 = "file1.log";
  std::string file_2 = "file2.log";

  {
    // Create a file sink
    std::shared_ptr<Sink> file_sink_1_a = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      file_1,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{});

    // Request the same sink
    std::shared_ptr<Sink> file_sink_1_b = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      file_1,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{});

    std::shared_ptr<Sink> file_sink_1_c = SinkManager::instance().get_sink(file_1);

    // Request a new sink of the same file
    std::shared_ptr<Sink> file_sink_3 = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      file_2,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{});

    // Compare the pointers
    REQUIRE_EQ(file_sink_1_a.get(), file_sink_1_b.get());
    REQUIRE_EQ(file_sink_1_a.get(), file_sink_1_c.get());
    REQUIRE_NE(file_sink_1_a.get(), file_sink_3.get());
    REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 0);
  }

  // Pointers are out of score and we except them cleaned up
  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 2);

  std::remove(file_1.data());
  std::remove(file_2.data());
}

/***/
TEST_CASE("sink_throws_with_unsupported_pattern_formatter_option")
{
  quill::PatternFormatterOptions pattern_formatter_options;
  pattern_formatter_options.format_pattern =
    "[%(time) %(logger)[%(process_id)][%(log_level)]] - %(message)";
  pattern_formatter_options.timestamp_pattern = "%D %H:%M:%S.%Qms";
  pattern_formatter_options.timestamp_timezone = quill::Timezone::LocalTime;
  pattern_formatter_options.add_metadata_to_multi_line_logs = false;

  {
    quill::ConsoleSinkConfig config;
    config.set_override_pattern_formatter_options(pattern_formatter_options);
    REQUIRE_THROWS(SinkManager::instance().create_or_get_sink<quill::ConsoleSink>(
      "console_sink", std::move(config)));
  }

  {
    quill::FileSinkConfig config;
    config.set_override_pattern_formatter_options(pattern_formatter_options);
    REQUIRE_THROWS(SinkManager::instance().create_or_get_sink<quill::FileSink>("file_sink", std::move(config)));
  }
}

TEST_SUITE_END();