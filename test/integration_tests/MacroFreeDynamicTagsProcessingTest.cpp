#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

namespace
{
std::string process_execution_tags(char const* tags)
{
  std::string processed_tags{tags};
  constexpr std::string_view executor_id{"executor_17"};

  size_t const executor_id_pos = processed_tags.find(executor_id);
  if (executor_id_pos != std::string::npos)
  {
    processed_tags.replace(executor_id_pos, executor_id.size(), "executor_render");
  }

  return processed_tags;
}
} // namespace

/***/
TEST_CASE("macro_free_dynamic_tags_processing")
{
  static constexpr char const* filename = "macro_free_dynamic_tags_processing.log";
  static constexpr char const* logger_name = "macro_free_dynamic_tags_processing_logger";

  Backend::start();

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  PatternFormatterOptions formatter_options{"%(tags)%(message)"};
  formatter_options.process_tags = process_execution_tags;

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink), formatter_options);

  size_t constexpr number_of_messages{9};

  {
    std::string const executor_tag{"executor_17"};
    std::string const priority_storage{"priority_1_not_part_of_the_tag"};
    std::string_view const priority_tag{priority_storage.data(), std::string_view{"priority_1"}.size()};

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      if ((i % 3) == 0)
      {
        info(logger, Tags{executor_tag}, "std::string tag {}", i);
      }
      else if ((i % 3) == 1)
      {
        info(logger, Tags{priority_tag}, "std::string_view tag {}", i);
      }
      else
      {
        info(logger, Tags{executor_tag, priority_tag}, "mixed dynamic tags {}", i);
      }
    }
  }

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    if ((i % 3) == 0)
    {
      REQUIRE_EQ(file_contents[i], "#executor_render std::string tag " + std::to_string(i));
    }
    else if ((i % 3) == 1)
    {
      REQUIRE_EQ(file_contents[i], "#priority_1 std::string_view tag " + std::to_string(i));
    }
    else
    {
      REQUIRE_EQ(file_contents[i], "#executor_render #priority_1 mixed dynamic tags " + std::to_string(i));
    }
  }

  testing::remove_file(filename);
}
