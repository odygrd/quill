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
  file_event_notifier.after_open = [&after_open_cnt, &file](fs::path const& filename, FILE*) mutable
  {
    ++after_open_cnt;
    REQUIRE_EQ(file.string(), filename.string());
  };

  uint32_t before_close_cnt{0};
  file_event_notifier.before_close = [&before_close_cnt, &file](fs::path const& filename, FILE*) mutable
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
  file_event_notifier.after_open = [&after_open_cnt, &file](fs::path const& file_path, FILE*) mutable
  {
    ++after_open_cnt;
    REQUIRE_EQ(file.string(), file_path.filename().string());
  };

  uint32_t before_close_cnt{0};
  file_event_notifier.before_close = [&before_close_cnt, &file](fs::path const& file_path, FILE*) mutable
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

TEST_SUITE_END();