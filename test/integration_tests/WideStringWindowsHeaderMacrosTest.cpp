#include "doctest/doctest.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#if defined(_WIN32)
  #include <windows.h>

  #if defined(_MSC_VER) && (!defined(min) || !defined(max))
    #error This test expects windows.h to define the min and max macros on MSVC.
  #endif
#endif

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/WideString.h"

namespace
{
std::vector<std::string> file_contents(char const* filename)
{
  std::ifstream file(filename);
  std::vector<std::string> lines;

  for (std::string line; std::getline(file, line);)
  {
    lines.push_back(line);
  }

  return lines;
}

bool file_contains(std::vector<std::string> const& lines, std::string const& expected)
{
  return std::find_if(lines.cbegin(), lines.cend(), [&expected](std::string const& line)
                      { return line.find(expected) != std::string::npos; }) != lines.cend();
}
} // namespace

/***/
TEST_CASE("wide_string_logging_after_windows_h_min_max_macros")
{
#if defined(_WIN32)
  static constexpr char const* filename = "wide_string_windows_header_macros.log";
  static std::string const logger_name = "wide_string_windows_header_macros_logger";

  quill::Backend::start();

  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    filename,
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    quill::FileEventNotifier{});

  quill::Logger* logger = quill::Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  std::wstring const wide_message = L"windows header wide string";
  LOG_INFO(logger, "wide [{}]", wide_message);

  logger->flush_log();
  quill::Frontend::remove_logger(logger);
  quill::Backend::stop();

  std::vector<std::string> const lines = file_contents(filename);
  std::remove(filename);

  REQUIRE(file_contains(lines, "LOG_INFO      " + logger_name + " wide [windows header wide string]"));
#endif
}
