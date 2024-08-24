#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/RotatingFileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

static constexpr char const* path_name = "flush_multiple_loggers";
std::vector<fs::path> created_files;

quill::Logger* create_logger()
{
  if (!quill::Backend::is_running())
  {
    quill::BackendOptions backend_options;
    backend_options.sleep_duration = std::chrono::nanoseconds(100);
    quill::Backend::start(backend_options);
  }

  // work out the log index number to add to the filename
  std::string log_index;
  int highest_index = 0;
  fs::path const log_path = fs::current_path() / path_name;

  if (std::filesystem::exists(log_path))
  {
    int file_int = 0;

    // iterate through files in folder, find the highest log index number in filenames.
    for (const auto& file : std::filesystem::directory_iterator(log_path))
    {
      if (file.is_regular_file())
      {
        std::string file_str = file.path().filename().generic_string();
        size_t underscore_1_index = file_str.find('_');
        size_t underscore_2_index = file_str.find('_', underscore_1_index + 1);

        if (underscore_1_index != std::string::npos && underscore_2_index != std::string::npos)
        {
          std::string number_str =
            file_str.substr(underscore_1_index + 1, underscore_2_index - underscore_1_index - 1);

          file_int = std::stoi(number_str);

          if (file_int > highest_index)
          {
            highest_index = file_int;
          }
        }
      }
    }

    log_index = "00" + std::to_string(highest_index + 1);
  }
  else
  {
    log_index = "001";
  }

  auto file_sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(
    (log_path / ("PS_" + log_index + ".log")).generic_string(),
    []()
    {
      quill::RotatingFileSinkConfig cfg;
      cfg.set_do_fsync(true);
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
      cfg.set_rotation_time_daily("00:00");
      cfg.set_rotation_max_file_size(1024 * 1024 * 10); // 10MB
      cfg.set_max_backup_files(100);
      return cfg;
    }());

  fs::path const created_file = reinterpret_cast<quill::RotatingFileSink*>(file_sink.get())->get_filename();
  while (!(std::filesystem::exists(created_file)))
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }

  created_files.push_back(created_file);

  quill::Logger* new_logger = quill::Frontend::create_or_get_logger(
    "logger_" + log_index, std::move(file_sink),
    quill::PatternFormatterOptions{"[%(time)] [%(log_level)] [" + log_index + ":%(tags):%(caller_function)]: %(message)"});

  return new_logger;
}

/***/
TEST_CASE("flush_multiple_loggers")
{
  quill::Logger* logger_a = create_logger();
  quill::Logger* logger_b = create_logger();
  quill::Logger* logger_c = create_logger();

  LOG_INFO(logger_a, "hello! - logger_a");
  LOG_INFO(logger_b, "hello! - logger_b");
  LOG_INFO(logger_c, "hello! - logger_c");

  logger_a->flush_log();

  // Check log is flushed after flush_log()
  bool file_001_checked = false;
  bool file_002_checked = false;
  bool file_003_checked = false;

  // Iterate all created files in our path
  fs::path const log_path = fs::current_path() / path_name;

  REQUIRE_EQ(created_files.size(), 3);
  for (const auto& file : created_files)
  {
    std::vector<std::string> const file_contents = quill::testing::file_contents(file);
    REQUIRE_EQ(file_contents.size(), 1);

    if (file.string().find("PS_001") != std::string::npos)
    {
      file_001_checked = true;
      REQUIRE(quill::testing::file_contains(
        file_contents, std::string{"[INFO] [001::DOCTEST_ANON_FUNC_2]: hello! - logger_a"}));
    }
    else if (file.string().find("PS_002") != std::string::npos)
    {
      file_002_checked = true;
      REQUIRE(quill::testing::file_contains(
        file_contents, std::string{"[002::DOCTEST_ANON_FUNC_2]: hello! - logger_b"}));
    }
    else if (file.string().find("PS_003") != std::string::npos)
    {
      file_003_checked = true;
      REQUIRE(quill::testing::file_contains(
        file_contents, std::string{"[INFO] [003::DOCTEST_ANON_FUNC_2]: hello! - logger_c"}));
    }
  }

  REQUIRE(file_001_checked);
  REQUIRE(file_002_checked);
  REQUIRE(file_003_checked);

  // flush all log and remove all loggers
  for (Logger* logger : Frontend::get_all_loggers())
  {
    logger->flush_log();
    Frontend::remove_logger(logger);
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Remove all files
  for (const auto& file : created_files)
  {
    testing::remove_file(file);
  }

  std::error_code ec;
  fs::remove_all(log_path, ec);
}