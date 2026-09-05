#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "misc/TestUtilities.h"

#include "quill/sinks/FileSink.h"

TEST_SUITE_BEGIN("FileEventNotifier");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("file_event_notifier_no_open")
{
  FileEventNotifier file_event_notifier;
  fs::path const file = "file_event_notifier_no_open.log";

  uint32_t before_open_cnt{0};
  file_event_notifier.before_open = [&before_open_cnt, &file](fs::path const& filename) mutable
  {
    ++before_open_cnt;
    REQUIRE_EQ(file.string(), filename.string());
  };

  uint32_t after_open_cnt{0};
  file_event_notifier.after_open = [&after_open_cnt, &file](fs::path const& filename, FileEventNotifierHandle) mutable
  {
    ++after_open_cnt;
    REQUIRE_EQ(file.string(), filename.string());
  };

  uint32_t before_close_cnt{0};
  file_event_notifier.before_close = [&before_close_cnt, &file](fs::path const& filename, FileEventNotifierHandle) mutable
  {
    ++before_close_cnt;
    REQUIRE_EQ(file.string(), filename.string());
  };

  uint32_t after_close_cnt{0};
  file_event_notifier.after_close = [&after_close_cnt, &file](fs::path const& filename) mutable
  {
    ++after_close_cnt;
    REQUIRE_EQ(file.string(), filename.string());
  };

  uint32_t before_write_cnt{0};
  file_event_notifier.before_write = [&before_write_cnt](std::string_view message) mutable
  {
    ++before_write_cnt;
    return std::string{message};
  };

  {
    FileSink fs_no_open{file, FileSinkConfig{}, file_event_notifier, false};
  }

  REQUIRE_EQ(before_open_cnt, 0);
  REQUIRE_EQ(after_open_cnt, 0);
  REQUIRE_EQ(before_close_cnt, 0);
  REQUIRE_EQ(after_close_cnt, 0);
  REQUIRE_EQ(before_write_cnt, 0);
  REQUIRE_FALSE(fs::exists(file));
}

/***/
TEST_CASE("file_event_notifier_nullptr")
{
  FileEventNotifier file_event_notifier;
  fs::path const file = "file_event_notifier_nullptr.log";

  file_event_notifier.before_open = nullptr;
  file_event_notifier.after_open = nullptr;
  file_event_notifier.before_close = nullptr;
  file_event_notifier.after_close = nullptr;
  file_event_notifier.before_write = nullptr;

  {
    FileSink fs{file, FileSinkConfig{}, file_event_notifier};
    fs.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                 std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", "test");
  }

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(file);
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"test"}));
  testing::remove_file(file);
}

/***/
TEST_CASE("file_event_notifier")
{
  FileEventNotifier file_event_notifier;
  fs::path const file = "file_event_notifier.log";
  std::string const output_msg = "Test Message";

  uint32_t before_open_cnt{0};
  file_event_notifier.before_open = [&before_open_cnt, &file](fs::path const& file_path) mutable
  {
    ++before_open_cnt;
    REQUIRE_EQ(file.string(), file_path.filename().string());
  };

  uint32_t after_open_cnt{0};
  file_event_notifier.after_open = [&after_open_cnt, &file](fs::path const& file_path, FileEventNotifierHandle) mutable
  {
    ++after_open_cnt;
    REQUIRE_EQ(file.string(), file_path.filename().string());
  };

  uint32_t before_close_cnt{0};
  file_event_notifier.before_close = [&before_close_cnt, &file](fs::path const& file_path, FileEventNotifierHandle) mutable
  {
    ++before_close_cnt;
    REQUIRE_EQ(file.string(), file_path.filename().string());
  };

  uint32_t after_close_cnt{0};
  file_event_notifier.after_close = [&after_close_cnt, &file](fs::path const& file_path) mutable
  {
    ++after_close_cnt;
    REQUIRE_EQ(file.string(), file_path.filename().string());
  };

  uint32_t before_write_cnt{0};
  file_event_notifier.before_write = [&before_write_cnt, &output_msg](std::string_view message) mutable
  {
    ++before_write_cnt;
    auto const input = std::string{message};
    REQUIRE_EQ(input, output_msg);
    return input;
  };

  {
    FileSink fs{file, FileSinkConfig{}, file_event_notifier};

    REQUIRE_EQ(before_open_cnt, 1);
    REQUIRE_EQ(after_open_cnt, 1);
    REQUIRE_EQ(before_close_cnt, 0);
    REQUIRE_EQ(after_close_cnt, 0);
    REQUIRE_EQ(before_write_cnt, 0);

    fs.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                 std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "", output_msg);

    REQUIRE_EQ(before_open_cnt, 1);
    REQUIRE_EQ(after_open_cnt, 1);
    REQUIRE_EQ(before_close_cnt, 0);
    REQUIRE_EQ(after_close_cnt, 0);
    REQUIRE_EQ(before_write_cnt, 1);
  }

  REQUIRE_EQ(before_open_cnt, 1);
  REQUIRE_EQ(after_open_cnt, 1);
  REQUIRE_EQ(before_close_cnt, 1);
  REQUIRE_EQ(after_close_cnt, 1);
  REQUIRE_EQ(before_write_cnt, 1);

  std::vector<std::string> const file_contents = quill::testing::file_contents(file);
  REQUIRE(quill::testing::file_contains(file_contents, output_msg));
  testing::remove_file(file);
}

TEST_CASE("file_event_notifier_write_order")
{
  auto write_text = [](FileEventNotifierHandle handle, std::string_view text)
  {
#if defined(_WIN32)
    DWORD written{0};
    REQUIRE(::WriteFile(handle, text.data(), static_cast<DWORD>(text.size()), &written, nullptr));
    REQUIRE_EQ(written, text.size());
#else
    REQUIRE_EQ(std::fwrite(text.data(), 1, text.size(), handle), text.size());
#endif
  };

  for (char const mode : {'w', 'a'})
  {
    fs::path const filename = std::string{"file_event_notifier_write_order_"} + mode + ".log";
    testing::create_file(filename, "existing\n");
    FileSinkConfig config;
    config.set_open_mode(mode);
    FileEventNotifier notifier;
    notifier.after_open = [write_text](fs::path const&, FileEventNotifierHandle handle)
    { write_text(handle, "header\n"); };
    notifier.before_close = [write_text](fs::path const&, FileEventNotifierHandle handle)
    { write_text(handle, "footer\n"); };
    {
      FileSink sink{filename, config, notifier};
      sink.write_log(nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, "", "body\n");
    }

    auto const contents = testing::file_contents(filename);
    size_t const offset = mode == 'a' ? 1u : 0u;
    REQUIRE_EQ(contents.size(), offset + 3u);
    if (offset != 0)
    {
      CHECK_EQ(contents[0], "existing");
    }
    CHECK_EQ(contents[offset], "header");
    CHECK_EQ(contents[offset + 1], "body");
    CHECK_EQ(contents[offset + 2], "footer");
    testing::remove_file(filename);
  }
}

TEST_SUITE_END();
