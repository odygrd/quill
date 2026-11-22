#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstddef>
#include <fstream>
#include <iterator>
#include <string>

using namespace quill;

/***/
TEST_CASE("log_binary")
{
  static constexpr char const* filename = "log_binary.bin";
  static std::string const logger_name = "log_binary_logger";
  static constexpr size_t number_of_records = 5000;

  // The default options: nothing has to be disabled to write the bytes of a record unchanged
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

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  std::string expected;

  for (size_t i = 0; i < number_of_records; ++i)
  {
    // Records of 1 to 300 bytes that go through every byte value, including the line breaks,
    // null and non-printable characters that a formatted message does not keep
    std::string record(1 + (i % 300), '\0');

    for (size_t j = 0; j < record.size(); ++j)
    {
      record[j] = static_cast<char>(i + j);
    }

    if ((i % 3) == 0)
    {
      LOG_BINARY(logger, record.data(), record.size());
    }
    else if ((i % 3) == 1)
    {
      QUILL_LOG_BINARY(logger, record.data(), record.size());
    }
    else
    {
      // Without the macro, the caller provides the metadata that the sinks get with the bytes
      static constexpr MacroMetadata macro_metadata{
        "", "", "", nullptr, LogLevel::None, MacroMetadata::Event::Binary};

      REQUIRE(logger->log_binary(&macro_metadata, record.data(), record.size()));
    }

    expected += record;
  }

  // Larger than the initial capacity of the queue and than a block of the backend's messages
  std::string large_record(512u * 1024u, '\0');

  for (size_t j = 0; j < large_record.size(); ++j)
  {
    large_record[j] = static_cast<char>(j * 7u);
  }

  LOG_BINARY(logger, large_record.data(), large_record.size());
  expected += large_record;

  // A record without bytes writes nothing
  LOG_BINARY(logger, nullptr, 0);

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  std::string contents;

  {
    std::ifstream file{filename, std::ios::binary};
    contents.assign(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
  }

  REQUIRE_EQ(contents.size(), expected.size());
  REQUIRE(contents == expected);

  testing::remove_file(filename);
}
