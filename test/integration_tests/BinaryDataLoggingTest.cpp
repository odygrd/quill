
#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/BinaryDataDeferredFormatCodec.h"
#include "quill/bundled/fmt/format.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

using namespace quill;

// Sample structs with descriptive names
struct Position
{
  uint32_t id;
  uint32_t width;
  uint32_t height;
};

std::ostream& operator<<(std::ostream& os, Position const& position)
{
  os << "Position {" << position.width << ", " << position.height << "}";
  return os;
}

struct StateInfo
{
  uint32_t id;
  int64_t timestamp;
  int64_t magnitude;
  bool active;
};

std::ostream& operator<<(std::ostream& os, StateInfo const& state)
{
  os << "StateInfo {" << state.timestamp << ", " << state.magnitude << ", " << state.active << "}";
  return os;
}

struct Entity
{
  uint32_t id;
  char name[24];
};

std::ostream& operator<<(std::ostream& os, Entity const& entity)
{
  os << "Entity {" << entity.name << "}";
  return os;
}

// Type provider for binary data codec
struct TypeProvider
{
};

// Codec support for sample structs
using BinaryTypeData = quill::BinaryData<TypeProvider>;

template <>
struct quill::Codec<BinaryTypeData> : quill::BinaryDataDeferredFormatCodec<BinaryTypeData>
{
};

template <>
struct fmtquill::formatter<BinaryTypeData>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::BinaryTypeData const& bin_data, format_context& ctx) const
  {
    uint32_t id;
    std::memcpy(&id, bin_data.data(), sizeof(uint32_t));

    std::stringstream oss;

    if (id == 1)
    {
      // Process Position data
      Position position;
      std::memcpy(&position, bin_data.data(), sizeof(Position));
      oss << position;
    }
    else if (id == 2)
    {
      // Process StateInfo data
      StateInfo state;
      std::memcpy(&state, bin_data.data(), sizeof(StateInfo));
      oss << state;
    }
    else if (id == 3)
    {
      // Process Entity data
      Entity entity;
      std::memcpy(&entity, bin_data.data(), sizeof(Entity));
      oss << entity;
    }

    return fmtquill::format_to(ctx.out(), "{}", oss.str());
  }
};

/**
 * Test to verify binary data logging functionality
 */
TEST_CASE("binary_data_logging")
{
  static constexpr size_t message_count = 10000;
  static constexpr char const* log_filename = "binary_data_logging.log";
  static std::string const logger_name = "binary_logger";

  // Start the logging backend thread
  Backend::start();

  Frontend::preallocate();

  // Set up file sink for logging
  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    log_filename,
    []()
    {
      FileSinkConfig config;
      config.set_open_mode('w');

      // For this test only we use the default buffer size, it should not make any difference
      // it is just for testing the default behavior and code coverage
      config.set_write_buffer_size(0);

      return config;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink),
                                                  PatternFormatterOptions{"%(message)"});

  std::array<std::byte, 64> buffer{};
  uint32_t encoded_size{0};

  std::byte* data_null = nullptr;
  LOG_INFO(logger, "null data [{}]", BinaryTypeData{data_null, encoded_size});

  // Log different types of data in rotation
  for (uint32_t i = 0; i < message_count; ++i)
  {
    if ((i % 3) == 0)
    {
      // encode position data
      Position position;
      position.id = 1;
      position.width = i;
      position.height = i * 10;

      std::memcpy(buffer.data(), &position, sizeof(Position));
      encoded_size = sizeof(Position);
    }
    else if ((i % 3) == 1)
    {
      // encode state info data
      StateInfo state;
      state.id = 2;
      state.timestamp = i * 1000;
      state.magnitude = i * 10000;
      state.active = (i % 2 == 0); // Alternate between true and false

      std::memcpy(buffer.data(), &state, sizeof(StateInfo));
      encoded_size = sizeof(StateInfo);
    }
    else if ((i % 3) == 2)
    {
      // encode entity data
      Entity entity;
      entity.id = 3;
      auto name = std::to_string(i);
      strcpy(&entity.name[0], name.c_str());

      std::memcpy(buffer.data(), &entity, sizeof(Entity));
      encoded_size = sizeof(Entity);
    }

    LOG_INFO(logger, "{}", BinaryTypeData{buffer.data(), encoded_size});
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and verify log contents
  std::vector<std::string> const file_contents = quill::testing::file_contents(log_filename);
  REQUIRE_EQ(file_contents.size(), message_count + 1);

  // Verify each logged message matches expected format
  std::string expected_string;
  expected_string = "null data []";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  for (size_t i = 0; i < message_count; ++i)
  {
    if ((i % 3) == 0)
    {
      expected_string = "Position {" + std::to_string(i) + ", " + std::to_string(i * 10) + "}";
    }
    else if ((i % 3) == 1)
    {
      bool is_active = (i % 2 == 0);
      expected_string = "StateInfo {" + std::to_string(i * 1000) + ", " +
        std::to_string(i * 10000) + ", " + (is_active ? "1" : "0") + "}";
    }
    else if ((i % 3) == 2)
    {
      expected_string = "Entity {" + std::to_string(i) + "}";
    }

    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  testing::remove_file(log_filename);
}

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning(pop)
#endif