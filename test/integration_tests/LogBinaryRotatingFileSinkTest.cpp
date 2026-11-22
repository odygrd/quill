#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/RotatingFileSink.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("log_binary_rotating_file_sink")
{
  static constexpr char const* filename = "log_binary_rotating_file_sink.bin";
  static constexpr char const* filename_1 = "log_binary_rotating_file_sink.1.bin";
  static constexpr char const* filename_2 = "log_binary_rotating_file_sink.2.bin";
  static constexpr char const* filename_3 = "log_binary_rotating_file_sink.3.bin";
  static constexpr char const* filename_4 = "log_binary_rotating_file_sink.4.bin";
  static std::string const logger_name = "log_binary_rotating_file_sink_logger";

  static constexpr size_t number_of_records = 200;
  static constexpr size_t record_size = 64;

  // 63 records fit in a file of 4096 bytes after its header of 24 bytes
  static constexpr size_t records_per_file = 63;

  // The bytes that every file starts with, such as the global header of a pcap file
  std::string const file_header{
    "\xd4\xc3\xb2\xa1\x02\x00\x04\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\xff\xff\x00\x00\x01\x00\x00\x00",
    24};

  Backend::start();

  auto rotating_file_sink = Frontend::create_or_get_sink<RotatingFileSink>(
    filename,
    [&file_header]()
    {
      RotatingFileSinkConfig cfg;
      cfg.set_rotation_max_file_size(4096);
      cfg.set_open_mode('w');
      cfg.set_file_header(file_header);
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(rotating_file_sink));

  std::vector<std::string> records;

  for (size_t i = 0; i < number_of_records; ++i)
  {
    // A fixed-size record, e.g. a market data event, with its sequence number and every byte value
    std::string record(record_size, '\0');
    uint64_t const sequence = i;
    std::memcpy(record.data(), &sequence, sizeof(sequence));

    for (size_t j = sizeof(sequence); j < record.size(); ++j)
    {
      record[j] = static_cast<char>(i + j);
    }

    LOG_BINARY(logger, record.data(), record.size());
    records.push_back(std::move(record));
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // The oldest records are in the backup with the highest index
  std::vector<char const*> const filenames{filename_3, filename_2, filename_1, filename};
  size_t record_index{0};

  for (char const* file_to_check : filenames)
  {
    std::string expected{file_header};

    for (size_t i = 0; (i < records_per_file) && (record_index < records.size()); ++i)
    {
      expected += records[record_index];
      ++record_index;
    }

    std::string contents;

    {
      std::ifstream file{file_to_check, std::ios::binary};
      contents.assign(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
    }

    // Every file starts with the header, and a record is never split between two files
    REQUIRE_EQ(contents.size(), expected.size());
    REQUIRE(contents == expected);
  }

  REQUIRE_EQ(record_index, number_of_records);
  REQUIRE_FALSE(fs::exists(filename_4));

  for (char const* file_to_remove : filenames)
  {
    testing::remove_file(file_to_remove);
  }
}
