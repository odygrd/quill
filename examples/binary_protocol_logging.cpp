#include "quill/Backend.h"
#include "quill/BinaryDataDeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/Utility.h"
#include "quill/sinks/ConsoleSink.h"

#include <array>
#include <iostream>
#include <sstream>
#include <utility>

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

/**
 * @brief Efficient binary data logging with deferred formatting
 *
 * This example demonstrates how to efficiently log binary data (such as network packets,
 * protocol buffers, or market data) using Quill's deferred formatting capabilities.
 *
 * Key benefits:
 * 1. Minimal overhead on the critical path - a memory copy is performed
 * 2. All formatting is deferred to the background logger thread
 * 3. Human-readable output for binary data in log files
 * 4. Support for different message types within the same binary protocol
 */

//------------------------------------------------------------------------------
// Sample message types for demonstration purposes
// (These would be your actual protocol messages in a real application)
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// Step 1: Create a tag struct to identify your binary protocol
//------------------------------------------------------------------------------

struct TypeProvider
{
};

//------------------------------------------------------------------------------
// Step 2: Define your binary data type using the tag
//------------------------------------------------------------------------------

using BinaryTypeData = quill::BinaryData<TypeProvider>;

//------------------------------------------------------------------------------
// Step 4: Implement a formatter for your binary data type
// This formatter runs in the background logger thread
//------------------------------------------------------------------------------

template <>
struct fmtquill::formatter<BinaryTypeData>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::BinaryTypeData const& bin_data, format_context& ctx) const
  {
    // Check if we have enough data to read the message ID
    if (bin_data.size() < sizeof(uint32_t))
    {
      return fmtquill::format_to(ctx.out(), "Invalid binary data: too small");
    }

    // Extract the message ID (first 4 bytes)
    uint32_t id;
    std::memcpy(&id, bin_data.data(), sizeof(uint32_t));

    std::stringstream oss;

    // Format based on message type
    switch (id)
    {
    case 1:
      if (bin_data.size() >= sizeof(Position))
      {
        Position position;
        std::memcpy(&position, bin_data.data(), sizeof(Position));
        oss << position;
      }
      else
      {
        oss << "Incomplete Position message";
      }
      break;

    case 2:
      if (bin_data.size() >= sizeof(StateInfo))
      {
        StateInfo state;
        std::memcpy(&state, bin_data.data(), sizeof(StateInfo));
        oss << state;
      }
      else
      {
        oss << "Incomplete StateInfo message";
      }
      break;

    case 3:
      if (bin_data.size() >= sizeof(Entity))
      {
        Entity entity;
        std::memcpy(&entity, bin_data.data(), sizeof(Entity));
        oss << entity;
      }
      else
      {
        oss << "Incomplete Entity message";
      }
      break;

    default:
      oss << "Unknown message type: " << id;
      break;
    }

    // Add a hex dump of the raw data
    static constexpr size_t upper_case_hex = false;
    oss << " <" << quill::utility::to_hex(bin_data.data(), bin_data.size(), upper_case_hex) << ">";

    return fmtquill::format_to(ctx.out(), "{}", oss.str());
  }
};

//------------------------------------------------------------------------------
// Step 4: Tell Quill to use deferred formatting for your binary data type
//------------------------------------------------------------------------------

template <>
struct quill::Codec<BinaryTypeData> : quill::BinaryDataDeferredFormatCodec<BinaryTypeData>
{
};

int main()
{
  // Initialize Quill backend
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Create a console sink and logger
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "trading", std::move(console_sink),
    quill::PatternFormatterOptions{"[%(time)] %(message)", "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

  // Buffer for our binary messages
  std::array<uint8_t, 128> buffer{};
  uint32_t message_size{0};

  // Log different types of data in rotation
  for (uint32_t i = 0; i < 15; ++i)
  {
    // Simulate encoding into the buffer different message types
    if ((i % 3) == 0)
    {
      // encode position data
      Position position;
      position.id = 1;
      position.width = i;
      position.height = i * 10;

      std::memcpy(buffer.data(), &position, sizeof(Position));
      message_size = sizeof(Position);
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
      message_size = sizeof(StateInfo);
    }
    else if ((i % 3) == 2)
    {
      // encode entity data
      Entity entity;
      entity.id = 3;
      auto name = std::to_string(i);
      strcpy(&entity.name[0], name.c_str());

      std::memcpy(buffer.data(), &entity, sizeof(Entity));
      message_size = sizeof(Entity);
    }

    // Pass the buffer for logging
    LOG_INFO(logger, "[IN] {}", BinaryTypeData{buffer.data(), message_size});
  }

  return 0;
}

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning(pop)
#endif