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
    file_sink->write_log(LogRecord{
      nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "preserved\n", {}});
    file_sink->flush_sink(SinkFlushReason::Explicit);
    FileSinkConfig overwrite_config;
    overwrite_config.set_open_mode('w');

#if !defined(_WIN32)
    fs::create_symlink(leaf_name, symlink_path, ec);
    if (!ec)
    {
      REQUIRE_EQ(SinkManager::instance().create_or_get_sink<FileSink>(symlink_path.string(), overwrite_config),
                 file_sink);
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
        REQUIRE_EQ(SinkManager::instance().create_or_get_sink<FileSink>(alias_path.string(), overwrite_config),
                   file_sink);
        REQUIRE_EQ(SinkManager::instance().get_sink(alias_path.string()), file_sink);
  #if !defined(QUILL_NO_EXCEPTIONS)
        REQUIRE_THROWS_AS(
          SinkManager::instance().create_sink<FileSink>(alias_path.string(), overwrite_config), QuillError);
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
    file_sink->write_log(LogRecord{
      nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "preserved\n", {}});
    file_sink->flush_sink(SinkFlushReason::Explicit);

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

/***/
class VirtualBaseSink : public virtual Sink
{
public:
  void write_log(LogRecord const&) override {}
  void flush_sink(SinkFlushReason) override {}
};

/***/
TEST_CASE("sink_with_virtual_base_can_be_created_and_retrieved")
{
  {
    // Sinks may inherit quill::Sink virtually, which static_pointer_cast cannot downcast
    std::shared_ptr<VirtualBaseSink> const created =
      SinkManager::instance().create_sink<VirtualBaseSink>("virtual_base_sink");
    REQUIRE(created);

#if QUILL_USE_RTTI
    REQUIRE_EQ(SinkManager::instance().create_or_get_sink<VirtualBaseSink>("virtual_base_sink"), created);
#endif
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1);
}

/***/
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
TEST_CASE("file_sink_windows_factory_accepts_a_unicode_parent")
{
#if defined(_WIN32)
  fs::path const previous{fs::current_path()};
  fs::path const directory{previous / L"sink_manager_unicode_\u4e2d\U0001f680"};
  fs::create_directories(directory);

  struct RestoreDirectory
  {
    fs::path previous;
    ~RestoreDirectory()
    {
      std::error_code ec;
      fs::current_path(previous, ec);
    }
  };

  {
    RestoreDirectory const restore{previous};
    fs::current_path(directory);
    auto& manager = SinkManager::instance();
    auto sink = manager.create_or_get_sink<FileSink>("ascii.log");
    REQUIRE(fs::equivalent(sink->get_filename(), directory / "ascii.log"));
    REQUIRE_EQ(manager.get_sink("ascii.log"), sink);
  }
  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1u);
  fs::remove(directory / "ascii.log");
  fs::remove(directory);
#endif
}

/***/
TEST_CASE("file_sink_path_and_name_overloads_share_sinks")
{
  // Cyrillic characters, which most Windows ANSI code pages cannot encode
#if defined(_WIN32)
  fs::path const directory{L"sink_manager_path_overload_\u041a\u0443\u0437\u044c\u043c\u0430"};
#else
  fs::path const directory{
    "sink_manager_path_overload_\xd0\x9a\xd1\x83\xd0\xb7\xd1\x8c\xd0\xbc\xd0\xb0"};
#endif
  fs::path const unicode_file{directory / "unicode.log"};
  fs::path const ascii_file{"sink_manager_path_overload_ascii.log"};

  {
    auto& manager = SinkManager::instance();
    auto unicode_sink = manager.create_or_get_file_sink<FileSink>(unicode_file);
    REQUIRE(fs::equivalent(unicode_sink->get_filename(), unicode_file));
    REQUIRE_EQ(manager.create_or_get_file_sink<FileSink>(fs::absolute(unicode_file)), unicode_sink);

    // A path and a narrow name that spell the same file share one sink
    auto ascii_sink = manager.create_file_sink<FileSink>(ascii_file);
    REQUIRE_EQ(manager.create_or_get_sink<FileSink>(ascii_file.string()), ascii_sink);
    REQUIRE_EQ(manager.get_sink(ascii_file.string()), ascii_sink);

#if !defined(QUILL_NO_EXCEPTIONS)
    REQUIRE_THROWS_AS((void)manager.create_file_sink<FileSink>(unicode_file), QuillError);
#endif
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 2u);
  fs::remove(unicode_file);
  fs::remove(ascii_file);
  fs::remove(directory);
}

/***/
TEST_CASE("file_sink_windows_ids_do_not_match_ansi_names")
{
#if defined(_WIN32)
  if (::GetACP() != 1252)
  {
    return;
  }

  fs::path const directory{"sink_manager_windows_encoding"};
  fs::path const first{directory / L"\u00c3\u2030.LOG"};
  fs::path const second{directory / L"\u00e9.log"};

  {
    auto& manager = SinkManager::instance();
    auto first_sink = manager.create_or_get_sink<FileSink>(first.string());
    auto second_sink = manager.create_or_get_sink<FileSink>(second.string());
    REQUIRE_NE(first_sink, second_sink);
    REQUIRE(fs::exists(first));
    REQUIRE(fs::exists(second));
    REQUIRE_EQ(manager.get_sink(fs::absolute(first).string()), first_sink);
    REQUIRE_EQ(manager.get_sink(fs::absolute(second).string()), second_sink);
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 2u);
  fs::remove(first);
  fs::remove(second);
  fs::remove(directory);
#endif
}

/***/
TEST_CASE("file_sink_windows_case_aliases_return_same_sink")
{
#if defined(_WIN32)
  fs::path const directory{"sink_manager_case_alias_test"};
  fs::create_directories(directory);

  for (FilenameAppendOption const append_option : {FilenameAppendOption::None, FilenameAppendOption::StartDate})
  {
    FileSinkConfig config;
    config.set_open_mode('w');
    config.set_filename_append_option(append_option);

    fs::path actual_filename;
    fs::path distinct_case_filename;
    uint32_t expected_cleanup_count{1};
    {
      auto file_sink = SinkManager::instance().create_or_get_sink<FileSink>(
        (directory / "case_alias.log").string(), config);
      actual_filename = file_sink->get_filename();
      file_sink->write_log(LogRecord{
        nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "preserved\n", {}});
      file_sink->flush_sink(SinkFlushReason::Explicit);

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
        distinct_case_filename = case_sink->get_filename();
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

/***/
TEST_CASE("file_sink_windows_parent_case_aliases_return_same_sink")
{
#if defined(_WIN32)
  fs::path const root{"sink_manager_parent_case_alias_test"};
  fs::path const filename{root / "aliasParent" / "alias.log"};
  fs::path const alias_filename{root / "ALIASPARENT" / "alias.log"};
  fs::create_directories(filename.parent_path());

  // Only a case-insensitive directory, the default, resolves the other spelling of its name
  if (!fs::exists(alias_filename.parent_path()))
  {
    fs::remove_all(root);
    return;
  }

  FileSinkConfig config;
  config.set_open_mode('w');

  {
    auto file_sink = SinkManager::instance().create_or_get_sink<FileSink>(filename.string(), config);
    file_sink->write_log(LogRecord{
      nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "preserved\n", {}});
    file_sink->flush_sink(SinkFlushReason::Explicit);

    // Another sink would open the file again and truncate it in 'w' mode
    REQUIRE_EQ(SinkManager::instance().create_or_get_sink<FileSink>(alias_filename.string(), config), file_sink);
    REQUIRE_EQ(SinkManager::instance().get_sink(alias_filename.string()), file_sink);
    REQUIRE_EQ(fs::file_size(filename), 10u);
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1u);
  fs::remove_all(root);
#endif
}

/***/
TEST_CASE("file_sink_windows_trailing_dot_alias_does_not_reopen_the_file")
{
#if defined(_WIN32)
  fs::path const directory{"sink_manager_trailing_dot"};
  fs::path const filename{directory / "ordinary.log"};
  fs::path const alias{directory / "ordinary.log. "};
  fs::path extended;

  {
    auto& manager = SinkManager::instance();
    FileSinkConfig config;
    config.set_open_mode('w');
    auto sink = manager.create_or_get_sink<FileSink>(filename.string(), config);
    LogRecord record;
    record.log_statement = "sentinel";
    sink->write_log(record);
    sink->flush_sink(SinkFlushReason::Explicit);

    REQUIRE_EQ(manager.create_or_get_sink<FileSink>(alias.string(), config), sink);
    REQUIRE_EQ(manager.get_sink(alias.string()), sink);
    REQUIRE_EQ(fs::file_size(filename), record.log_statement.size());

    auto const absolute = fs::absolute(filename).native();
    if (absolute.size() > 2 && absolute[1] == L':')
    {
      extended = fs::path{L"\\\\?\\" + absolute + L"."};
      auto distinct_sink = manager.create_or_get_sink<FileSink>(extended.string(), config);
      REQUIRE_NE(distinct_sink, sink);
      REQUIRE_EQ(fs::file_size(filename), record.log_statement.size());
    }
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), extended.empty() ? 1u : 2u);
  if (!extended.empty())
  {
    fs::remove(extended);
  }
  fs::remove(filename);
  fs::remove(directory);
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
    for (FilenameAppendOption const append_option : {FilenameAppendOption::None, FilenameAppendOption::StartDate})
    {
      FileSinkConfig config;
      config.set_open_mode('w');
      config.set_filename_append_option(append_option);
      fs::path actual_filename;

      {
        auto file_sink = SinkManager::instance().create_or_get_sink<FileSink>(
          (extended_first ? extended_path : ordinary_path).string(), config);
        actual_filename = file_sink->get_filename();
        file_sink->write_log(LogRecord{
          nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, {}, "preserved\n", {}});
        file_sink->flush_sink(SinkFlushReason::Explicit);

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

/***/
class StringLvalueFileSink : public FileSink
{
public:
  // A file sink without an fs::path constructor receives its name as a std::string lvalue
  explicit StringLvalueFileSink(std::string const& filename) : FileSink{filename} {}
  explicit StringLvalueFileSink(std::string&&) = delete;
};

/***/
TEST_CASE("file_sink_with_string_lvalue_constructor_can_be_created")
{
  std::string const filename{"file_sink_with_string_lvalue_constructor.log"};

  {
    std::shared_ptr<StringLvalueFileSink> const sink =
      SinkManager::instance().create_or_get_sink<StringLvalueFileSink>(filename);
    REQUIRE(sink);
    REQUIRE_EQ(SinkManager::instance().create_or_get_file_sink<StringLvalueFileSink>(fs::path{filename}), sink);
  }

  REQUIRE_EQ(SinkManager::instance().cleanup_unused_sinks(), 1u);
  REQUIRE(fs::remove(filename));
}

TEST_SUITE_END();
