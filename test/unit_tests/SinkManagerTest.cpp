#include "doctest/doctest.h"

#include "quill/core/Filesystem.h"
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

TEST_CASE("file_sink_equivalent_paths_return_same_sink")
{
  fs::path const base_dir = fs::path{"sink_manager_test_dir"};
  fs::path const canonical_path = base_dir / "equivalent.log";
  fs::path const equivalent_path = fs::path{"."} / base_dir / ".." / base_dir / "equivalent.log";

  std::error_code ec;
  fs::create_directories(base_dir, ec);
  REQUIRE_FALSE(ec);

  {
    std::shared_ptr<Sink> file_sink_a = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      canonical_path.string(),
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{});

    std::shared_ptr<Sink> file_sink_b = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      equivalent_path.string(),
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{});

    // These two paths resolve to the same file, so SinkManager should return the same sink.
    REQUIRE_EQ(file_sink_a.get(), file_sink_b.get());
    REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 0);
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1);

  fs::remove(canonical_path, ec);
  REQUIRE_FALSE(ec);
  fs::remove(base_dir, ec);
  REQUIRE_FALSE(ec);
}

TEST_CASE("recreating_expired_sink_with_same_name_keeps_single_registry_entry")
{
  std::string const file_name = "sink_manager_recreate_same_name.log";

  {
    std::shared_ptr<Sink> sink_a = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      file_name,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{});
  }

  {
    std::shared_ptr<Sink> sink_b = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      file_name,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{});

    std::shared_ptr<Sink> sink_c = SinkManager::instance().get_sink(file_name);
    REQUIRE_EQ(sink_b.get(), sink_c.get());
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1);

  std::remove(file_name.data());
}

TEST_CASE("create_sink_succeeds_and_rejects_duplicate")
{
  std::string const file_name = "sink_manager_create_sink_test.log";

  {
    std::shared_ptr<Sink> sink = SinkManager::instance().create_sink<quill::FileSink>(
      file_name,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{});

    REQUIRE_NE(sink.get(), nullptr);

    // Attempting to create again with the same name should throw
#if !defined(QUILL_NO_EXCEPTIONS)
    REQUIRE_THROWS_AS(SinkManager::instance().create_sink<quill::FileSink>(
                        file_name,
                        []()
                        {
                          quill::FileSinkConfig cfg;
                          cfg.set_open_mode('w');
                          return cfg;
                        }(),
                        FileEventNotifier{}),
                      QuillError);
#endif

    // create_or_get_sink should still return the existing one without throwing
    std::shared_ptr<Sink> same_sink = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      file_name,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('a');
        return cfg;
      }(),
      FileEventNotifier{});

    REQUIRE_EQ(sink.get(), same_sink.get());
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1);

  std::remove(file_name.data());
}

TEST_CASE("file_event_notifier_after_open_can_create_another_sink")
{
  std::string const outer_file_name = "sink_manager_reentrant_outer.log";
  std::string const inner_file_name = "sink_manager_reentrant_inner.log";

  std::shared_ptr<Sink> inner_sink;
  bool after_open_called{false};

  FileEventNotifier file_event_notifier;
  file_event_notifier.after_open = [&](fs::path const&, FileEventNotifierHandle)
  {
    after_open_called = true;

    inner_sink = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      inner_file_name,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{});
  };

  {
    std::shared_ptr<Sink> outer_sink = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      outer_file_name,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        return cfg;
      }(),
      file_event_notifier);

    REQUIRE(after_open_called);
    REQUIRE_NE(outer_sink.get(), nullptr);
    REQUIRE_NE(inner_sink.get(), nullptr);
    REQUIRE_NE(outer_sink.get(), inner_sink.get());
    REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 0);
  }

  inner_sink.reset();
  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 2);

  std::remove(outer_file_name.data());
  std::remove(inner_file_name.data());
}

TEST_SUITE_END();
