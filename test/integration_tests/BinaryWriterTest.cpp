#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/BinaryWriter.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/backend/SignalHandler.h"
#include "quill/core/FrontendOptions.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

using namespace quill;

struct BinaryWriterCustomFrontendOptions : FrontendOptions
{
  static constexpr size_t initial_queue_capacity = 4096;
};

// The writer also compiles for an application with custom frontend options
template class quill::BinaryWriterImpl<BinaryWriterCustomFrontendOptions>;

/** Keeps the records that a writer passes to a sink of the user */
struct BinaryWriterCapturingSink final : public Sink
{
  void write_log(LogRecord const& /* record */) override { ++log_writes; }

  void write_binary(BinaryRecord const& record) override
  {
    records.emplace_back(reinterpret_cast<char const*>(record.data), record.size);
    all_have_binary_metadata &= (record.macro_metadata != nullptr) &&
      (record.macro_metadata->event() == MacroMetadata::Event::Binary);
  }

  void flush_sink(SinkFlushReason) override {}

  std::vector<std::string> records;
  size_t log_writes{0};
  bool all_have_binary_metadata{true};
};

/***/
TEST_CASE("binary_writer")
{
  static constexpr char const* filename = "binary_writer.bin";
  static constexpr char const* rotating_filename = "binary_writer_rotating.bin";
  static constexpr char const* rotating_filename_1 = "binary_writer_rotating.1.bin";
  static constexpr char const* rotating_filename_2 = "binary_writer_rotating.2.bin";
  static constexpr char const* framed_filename = "binary_writer_framed.bin";
  static constexpr char const* shared_filename = "binary_writer_shared.bin";

  auto read_file_bytes = [](char const* file_to_read)
  {
    std::ifstream file{file_to_read, std::ios::binary};
    return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
  };

  testing::remove_file(filename);
  testing::remove_file(rotating_filename);
  testing::remove_file(rotating_filename_1);
  testing::remove_file(rotating_filename_2);
  testing::remove_file(framed_filename);
  testing::remove_file(shared_filename);

  // Bytes that a text file would not keep
  std::string const file_header{"\xd4\xc3\xb2\xa1\x00\n\r\n", 8};
  std::string const first_record{"\x01\x00\n\xff", 4};
  std::string const second_record(200, '\n');
  std::string const third_record(200, '\0');

  Backend::start();

  {
    // A file that starts with a header
    FileSinkConfig sink_config;
    sink_config.set_open_mode('w');
    sink_config.set_file_header(file_header);

    BinaryWriter writer{filename, sink_config};
    REQUIRE(writer.write(first_record.data(), first_record.size()));
    REQUIRE(writer.write(second_record.data(), second_record.size()));

    writer.flush();
    REQUIRE(read_file_bytes(filename) == file_header + first_record + second_record);

    writer.close();

#if !defined(QUILL_NO_EXCEPTIONS)
    REQUIRE_THROWS_AS(writer.write(first_record.data(), first_record.size()), QuillError);
    REQUIRE_THROWS_AS(writer.flush(), QuillError);
#endif
  }

  {
    // The default configuration appends to the file
    BinaryWriter writer{filename};
    REQUIRE(writer.write(third_record.data(), third_record.size()));
    writer.close();
  }

  {
    // Two records fit in a rotating file after its header, and the third one rotates it
    RotatingFileSinkConfig sink_config;
    sink_config.set_open_mode('w');
    sink_config.set_rotation_max_file_size(512);
    sink_config.set_file_header(file_header);

    BinaryWriter writer{rotating_filename, sink_config};
    REQUIRE(writer.write(second_record.data(), second_record.size()));
    REQUIRE(writer.write(third_record.data(), third_record.size()));
    REQUIRE(writer.write(second_record.data(), second_record.size()));
    writer.close();
  }

  {
    // Records of different sizes, each after its timestamp and size
    FileSinkConfig sink_config;
    sink_config.set_open_mode('w');
    sink_config.set_file_header(file_header);
    sink_config.set_binary_record_framing(BinaryRecordFraming::TimestampAndSize);

    BinaryWriter writer{framed_filename, sink_config};
    REQUIRE(writer.write(first_record.data(), first_record.size()));
    REQUIRE(writer.write(second_record.data(), second_record.size()));

    // A header and data that are given apart are one record
    REQUIRE(writer.write(first_record.data(), first_record.size(), third_record.data(), third_record.size()));
    writer.close();
  }

  {
    // A second writer that is given the name of the file shares its sink, so it does not
    // truncate the file, and its records are written as the records of the first one. A writer
    // that would frame its records is refused
    BinaryWriter writer{shared_filename};
    REQUIRE(writer.write(first_record.data(), first_record.size()));
    writer.flush();

    FileSinkConfig truncating_sink_config;
    truncating_sink_config.set_open_mode('w');

    BinaryWriter second_writer{shared_filename, truncating_sink_config};
    REQUIRE(second_writer.write(second_record.data(), second_record.size()));

#if !defined(QUILL_NO_EXCEPTIONS)
    FileSinkConfig framed_sink_config;
    framed_sink_config.set_binary_record_framing(BinaryRecordFraming::Size);
    REQUIRE_THROWS_AS(BinaryWriter(shared_filename, framed_sink_config), QuillError);
#endif

    writer.close();
    second_writer.close();
  }

  {
    // The signal handler does not choose the logger of a writer to write its messages, which
    // would be text in the middle of the records
    BinaryWriter writer{filename};

    std::vector<std::string> const excluded_logger_substrings = SignalHandlerOptions{}.excluded_logger_substrings;
    REQUIRE_EQ(detail::LoggerManager::instance().get_valid_logger(excluded_logger_substrings), nullptr);

    // Nor does the logger of a writer write the log statements of an application that logs to
    // every logger it finds: the file has the records and nothing else
    for (Logger* logger : Frontend::get_all_loggers())
    {
      CHECK_EQ(logger->get_log_level(), LogLevel::None);
      LOG_INFO(logger, "text that is not a record");
      LOG_CRITICAL(logger, "text that is not a record");
      logger->flush_log();
    }

    writer.close();
  }

  auto capturing_sink =
    Frontend::create_or_get_sink<BinaryWriterCapturingSink>("binary_writer_capturing_sink");

  {
    // A sink of the user
    BinaryWriter writer{"binary_writer_capturing", capturing_sink};
    REQUIRE(writer.write(first_record.data(), first_record.size()));
    REQUIRE(writer.write(nullptr, 0));
    writer.close();
  }

  auto first_of_two_sinks =
    Frontend::create_or_get_sink<BinaryWriterCapturingSink>("binary_writer_first_of_two");
  auto second_of_two_sinks =
    Frontend::create_or_get_sink<BinaryWriterCapturingSink>("binary_writer_second_of_two");

  {
    // Several sinks of the user, each of which gets every record
    BinaryWriter writer{"binary_writer_two_sinks",
                        std::vector<std::shared_ptr<Sink>>{first_of_two_sinks, second_of_two_sinks}};
    REQUIRE(writer.write(first_record.data(), first_record.size()));
    REQUIRE(writer.write(second_record.data(), second_record.size()));
    writer.close();
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  REQUIRE(read_file_bytes(filename) == file_header + first_record + second_record + third_record);
  REQUIRE(read_file_bytes(shared_filename) == first_record + second_record);

  REQUIRE(read_file_bytes(rotating_filename_1) == file_header + second_record + third_record);
  REQUIRE(read_file_bytes(rotating_filename) == file_header + second_record);
  REQUIRE_FALSE(fs::exists(rotating_filename_2));

  // Read the framed file as a reader does: the timestamp and the size tell where a record ends
  std::string const framed_contents = read_file_bytes(framed_filename);
  REQUIRE(framed_contents.substr(0, file_header.size()) == file_header);

  std::vector<std::string> framed_records;
  uint64_t previous_timestamp{0};
  size_t position = file_header.size();

  while (position < framed_contents.size())
  {
    REQUIRE_GE(framed_contents.size() - position, sizeof(uint64_t) + sizeof(uint32_t));

    uint64_t timestamp;
    std::memcpy(&timestamp, framed_contents.data() + position, sizeof(timestamp));
    position += sizeof(timestamp);

    uint32_t size;
    std::memcpy(&size, framed_contents.data() + position, sizeof(size));
    position += sizeof(size);

    // Nanoseconds since epoch, after 2020 and not before the record that precedes it
    REQUIRE_GT(timestamp, 1'577'836'800'000'000'000ull);
    REQUIRE_GE(timestamp, previous_timestamp);
    previous_timestamp = timestamp;

    REQUIRE_GE(framed_contents.size() - position, size);
    framed_records.push_back(framed_contents.substr(position, size));
    position += size;
  }

  REQUIRE_EQ(framed_records.size(), 3);
  REQUIRE(framed_records[0] == first_record);
  REQUIRE(framed_records[1] == second_record);
  REQUIRE(framed_records[2] == first_record + third_record);

  auto const* capturing = static_cast<BinaryWriterCapturingSink const*>(capturing_sink.get());
  REQUIRE_EQ(capturing->records.size(), 2);
  REQUIRE(capturing->records[0] == first_record);
  REQUIRE(capturing->records[1].empty());
  REQUIRE(capturing->all_have_binary_metadata);
  REQUIRE_EQ(capturing->log_writes, 0);

  for (auto const& sink : {first_of_two_sinks, second_of_two_sinks})
  {
    REQUIRE_EQ(sink->records.size(), 2);
    REQUIRE(sink->records[0] == first_record);
    REQUIRE(sink->records[1] == second_record);
  }

  testing::remove_file(filename);
  testing::remove_file(rotating_filename);
  testing::remove_file(rotating_filename_1);
  testing::remove_file(framed_filename);
  testing::remove_file(shared_filename);
}
