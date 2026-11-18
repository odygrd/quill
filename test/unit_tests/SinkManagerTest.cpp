#include "doctest/doctest.h"

#include "quill/core/Filesystem.h"
#include "quill/core/QuillError.h"
#include "quill/core/SinkManager.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/NullSink.h"
#include "quill/sinks/RotatingFileSink.h"
#include <cstdio>
#include <fstream>

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
#if defined(__APPLE__)
  fs::path const leaf_name{u8"équivalent.log"};
#else
  fs::path const leaf_name{"equivalent.log"};
#endif
  fs::path const canonical_path = base_dir / leaf_name;
  fs::path const equivalent_path = fs::path{"."} / base_dir / ".." / base_dir / leaf_name;
  fs::path const symlink_path = base_dir / "linked.log";

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

    auto file_sink = std::static_pointer_cast<FileSink>(file_sink_a);
    file_sink->write_log(nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "preserved\n");
    file_sink->flush_sink();
    FileSinkConfig overwrite_config;
    overwrite_config.set_open_mode('w');

#if !defined(_WIN32)
    fs::create_symlink(leaf_name, symlink_path, ec);
    if (!ec)
    {
      REQUIRE_EQ(SinkManager::instance().create_or_get_sink<FileSink>(symlink_path.string(), overwrite_config), file_sink);
      REQUIRE_EQ(SinkManager::instance().get_sink(symlink_path.string()), file_sink);
      REQUIRE(fs::remove(symlink_path));
    }
#endif

#if defined(__APPLE__)
    for (auto const* alias_name : {u8"Équivalent.log", u8"e\u0301quivalent.log"})
    {
      fs::path const alias_path = base_dir / alias_name;
      if (fs::exists(alias_path))
      {
        REQUIRE(fs::equivalent(canonical_path, alias_path));
        REQUIRE_EQ(SinkManager::instance().create_or_get_sink<FileSink>(alias_path.string(), overwrite_config), file_sink);
        REQUIRE_EQ(SinkManager::instance().get_sink(alias_path.string()), file_sink);
#if !defined(QUILL_NO_EXCEPTIONS)
        REQUIRE_THROWS_AS(SinkManager::instance().create_sink<FileSink>(alias_path.string(), overwrite_config), QuillError);
#endif
      }
    }
#endif
    REQUIRE_EQ(fs::file_size(canonical_path), 10u);
    REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 0);
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1);

  fs::remove(canonical_path, ec);
  REQUIRE_FALSE(ec);
  fs::remove(base_dir, ec);
  REQUIRE_FALSE(ec);
}

TEST_CASE("file_sink_symlink_created_before_target_returns_same_sink")
{
#if !defined(_WIN32)
  fs::path const base_dir = fs::path{"sink_manager_symlink_before_target_dir"};
  fs::path const target_path = base_dir / "target.log";
  fs::path const symlink_path = base_dir / "current.log";

  std::error_code ec;
  fs::create_directories(base_dir, ec);
  REQUIRE_FALSE(ec);

  // The link exists before its target, so the first lookup cannot resolve it
  fs::create_symlink(target_path.filename(), symlink_path, ec);
  if (ec)
  {
    fs::remove(base_dir, ec);
    return;
  }

  {
    FileSinkConfig overwrite_config;
    overwrite_config.set_open_mode('w');

    std::shared_ptr<Sink> file_sink_a =
      SinkManager::instance().create_or_get_sink<FileSink>(symlink_path.string(), overwrite_config);
    REQUIRE(fs::exists(target_path));

    auto file_sink = std::static_pointer_cast<FileSink>(file_sink_a);
    file_sink->write_log(nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "preserved\n");
    file_sink->flush_sink();

    // Now the link resolves to its target; both spellings must still return the first sink
    REQUIRE_EQ(SinkManager::instance().create_or_get_sink<FileSink>(symlink_path.string(), overwrite_config),
               file_sink_a);
    REQUIRE_EQ(SinkManager::instance().create_or_get_sink<FileSink>(target_path.string(), overwrite_config),
               file_sink_a);
    REQUIRE_EQ(SinkManager::instance().get_sink(symlink_path.string()), file_sink_a);
    REQUIRE_EQ(fs::file_size(target_path), 10u);
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1);

  fs::remove(symlink_path, ec);
  fs::remove(target_path, ec);
  fs::remove(base_dir, ec);
  REQUIRE_FALSE(ec);
#endif
}

TEST_CASE("get_sink_finds_non_path_sink_names_while_a_file_sink_exists")
{
  std::string const file_name = "sink_manager_non_path_lookup.log";

  {
    // A live file sink makes get_sink() consider path normalization for misses
    std::shared_ptr<Sink> file_sink = SinkManager::instance().create_or_get_sink<FileSink>(file_name);

    for (auto const* name : {"tcp://collector:4317", "audit/syslog"})
    {
      std::shared_ptr<Sink> null_sink = SinkManager::instance().create_or_get_sink<NullSink>(name);
      REQUIRE_EQ(SinkManager::instance().get_sink(name), null_sink);
    }

#if !defined(QUILL_NO_EXCEPTIONS)
    REQUIRE_THROWS_AS((void)SinkManager::instance().get_sink("missing/sink"), QuillError);
#endif
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 3);
  std::remove(file_name.data());
}

TEST_CASE("file_sink_retargeted_symlink_returns_sink_of_new_target")
{
#if !defined(_WIN32)
  fs::path const base_dir = fs::path{"sink_manager_retargeted_symlink_dir"};
  fs::path const first_target = base_dir / "first.log";
  fs::path const second_target = base_dir / "second.log";
  fs::path const symlink_path = base_dir / "current.log";

  std::error_code ec;
  fs::create_directories(base_dir, ec);
  REQUIRE_FALSE(ec);

  // Both targets exist throughout, so the link always resolves
  std::ofstream{first_target}.close();
  std::ofstream{second_target}.close();

  fs::create_symlink(first_target.filename(), symlink_path, ec);
  if (ec)
  {
    fs::remove(first_target, ec);
    fs::remove(second_target, ec);
    fs::remove(base_dir, ec);
    return;
  }

  {
    std::shared_ptr<Sink> first_sink =
      SinkManager::instance().create_or_get_sink<FileSink>(symlink_path.string(), FileSinkConfig{});
    REQUIRE(fs::exists(first_target));

    // The link now points to another file, so the same name must not return the old file's sink
    fs::remove(symlink_path, ec);
    fs::create_symlink(second_target.filename(), symlink_path, ec);
    REQUIRE_FALSE(ec);

    std::shared_ptr<Sink> second_sink =
      SinkManager::instance().create_or_get_sink<FileSink>(symlink_path.string(), FileSinkConfig{});
    REQUIRE_NE(second_sink, first_sink);

    // Replacing the link with a regular file at the same path is another file again
    fs::remove(symlink_path, ec);
    std::ofstream{symlink_path}.close();

    std::shared_ptr<Sink> third_sink =
      SinkManager::instance().create_or_get_sink<FileSink>(symlink_path.string(), FileSinkConfig{});
    REQUIRE_NE(third_sink, first_sink);
    REQUIRE_NE(third_sink, second_sink);
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 3);

  fs::remove(symlink_path, ec);
  fs::remove(first_target, ec);
  fs::remove(second_target, ec);
  fs::remove(base_dir, ec);
  REQUIRE_FALSE(ec);
#endif
}

/***/
TEST_CASE("file_sink_windows_case_aliases_return_same_sink")
{
#if defined(_WIN32)
  fs::path const directory{"sink_manager_case_alias_test"};
  fs::create_directories(directory);

  for (FilenameAppendOption const append_option :
       {FilenameAppendOption::None, FilenameAppendOption::StartDate})
  {
    FileSinkConfig config;
    config.set_open_mode('w');
    config.set_filename_append_option(append_option);

    fs::path actual_filename;
    fs::path distinct_case_filename;
    uint32_t expected_cleanup_count{1};
    {
      auto file_sink = std::static_pointer_cast<FileSink>(
        SinkManager::instance().create_or_get_sink<FileSink>((directory / "case_alias.log").string(), config));
      actual_filename = file_sink->get_filename();
      file_sink->write_log(nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "preserved\n");
      file_sink->flush_sink();

      auto case_name = actual_filename.filename().native();
      case_name[0] = L'C';
      bool const aliases = fs::exists(actual_filename.parent_path() / case_name);
      std::string const case_path = (directory / "Case_alias.log").string();
      auto case_sink = SinkManager::instance().create_or_get_sink<FileSink>(case_path, config);

      REQUIRE_EQ(case_sink == file_sink, aliases);
      REQUIRE_EQ(SinkManager::instance().get_sink(case_path), case_sink);
      REQUIRE_EQ(fs::file_size(actual_filename), 10u);

      if (!aliases)
      {
        distinct_case_filename = std::static_pointer_cast<FileSink>(case_sink)->get_filename();
        ++expected_cleanup_count;
      }
    }

    REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), expected_cleanup_count);

    REQUIRE(fs::remove(actual_filename));
    if (!distinct_case_filename.empty())
    {
      REQUIRE(fs::remove(distinct_case_filename));
    }
  }

  REQUIRE(fs::remove(directory));
#endif
}

TEST_CASE("file_sink_windows_extended_path_aliases_return_same_sink")
{
#if defined(_WIN32)
  fs::path const directory = fs::absolute("sink_manager_extended_alias_test");
  fs::create_directories(directory);
  fs::path const ordinary_path = directory / "prefix_alias.log";
  std::wstring const ordinary_name = ordinary_path.native();
  fs::path const extended_path{ordinary_name.compare(0, 2, L"\\\\") == 0
                                ? L"\\\\?\\UNC\\" + ordinary_name.substr(2)
                                : L"\\\\?\\" + ordinary_name};

  for (bool const extended_first : {false, true})
  {
    for (FilenameAppendOption const append_option :
         {FilenameAppendOption::None, FilenameAppendOption::StartDate})
    {
      FileSinkConfig config;
      config.set_open_mode('w');
      config.set_filename_append_option(append_option);
      fs::path actual_filename;

      {
        auto file_sink = std::static_pointer_cast<FileSink>(
          SinkManager::instance().create_or_get_sink<FileSink>(
            (extended_first ? extended_path : ordinary_path).string(), config));
        actual_filename = file_sink->get_filename();
        file_sink->write_log(nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "preserved\n");
        file_sink->flush_sink();

        auto alias_sink = SinkManager::instance().create_or_get_sink<FileSink>(
          (extended_first ? ordinary_path : extended_path).string(), config);

        REQUIRE_EQ(alias_sink, file_sink);
        REQUIRE_EQ(SinkManager::instance().get_sink(ordinary_path.string()), file_sink);
        REQUIRE_EQ(SinkManager::instance().get_sink(extended_path.string()), file_sink);
        REQUIRE_EQ(fs::file_size(actual_filename), 10u);
      }

      REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1u);
      REQUIRE(fs::remove(actual_filename));
    }
  }

  REQUIRE(fs::remove(directory));
#endif
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

/***/
TEST_CASE("create_or_get_sink_rejects_incompatible_existing_type")
{
#if QUILL_USE_RTTI && !defined(QUILL_NO_EXCEPTIONS)
  std::string const filename = "sink_manager_incompatible_type.log";

  {
    std::shared_ptr<Sink> file_sink = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      filename,
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{});

    // Requesting the same name as the same or a base type returns the existing sink
    std::shared_ptr<Sink> same_sink = SinkManager::instance().create_or_get_sink<quill::FileSink>(
      filename, quill::FileSinkConfig{}, FileEventNotifier{});
    REQUIRE_EQ(file_sink.get(), same_sink.get());

    // Requesting the same name as an incompatible type must throw instead of returning a sink
    // that the caller would cast to the wrong type
    quill::RotatingFileSinkConfig rotating_cfg;
    rotating_cfg.set_rotation_max_file_size(1024);
    REQUIRE_THROWS_AS((void)SinkManager::instance().create_or_get_sink<quill::RotatingFileSink>(
                        filename, rotating_cfg, FileEventNotifier{}),
                      QuillError);
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1);
  std::remove(filename.data());
#endif
}

TEST_SUITE_END();
