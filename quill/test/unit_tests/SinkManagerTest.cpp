#include "doctest/doctest.h"

#include "quill/core/SinkManager.h"
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
    std::shared_ptr<Sink> file_sink_1 = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      file_1,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{});

    // Request the same sink
    std::shared_ptr<Sink> file_sink_2 = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      file_1,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{});

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
    REQUIRE_EQ(file_sink_1, file_sink_2);
    REQUIRE_NE(file_sink_1, file_sink_3);
    REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 0);
  }

  // Pointers are out of score and we except them cleaned up
  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 2);

  std::remove(file_1.data());
  std::remove(file_2.data());
}

TEST_SUITE_END();