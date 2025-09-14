#include "quill/Backend.h"
#include "quill/BinaryDataDeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/Utility.h"
#include "quill/sinks/FileSink.h"

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

struct TemperatureReading
{
  uint32_t id;
  uint32_t celsius;
  uint32_t humidity;
};

std::ostream& operator<<(std::ostream& os, TemperatureReading const& temp)
{
  os << "Temperature {" << temp.celsius << "°C, " << temp.humidity << "% humidity}";
  return os;
}

struct WindData
{
  uint32_t id;
  int64_t timestamp;
  int64_t speed_kmh;
  bool storm_warning;
};

std::ostream& operator<<(std::ostream& os, WindData const& wind)
{
  os << "Wind {" << wind.timestamp << ", " << wind.speed_kmh << " km/h, storm: " << wind.storm_warning << "}";
  return os;
}

struct WeatherStation
{
  uint32_t id;
  char location[24];
};

std::ostream& operator<<(std::ostream& os, WeatherStation const& station)
{
  os << "Station {" << station.location << "}";
  return os;
}

struct WeatherProtocol
{
};

using WeatherBinaryData = quill::BinaryData<WeatherProtocol>;

template <>
struct fmtquill::formatter<WeatherBinaryData>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::WeatherBinaryData const& bin_data, format_context& ctx) const
  {
    auto out = ctx.out();
    auto size = bin_data.size();
    char const* size_ptr = reinterpret_cast<char const*>(&size);
    out = std::copy(size_ptr, size_ptr + sizeof(size), out);
    char const* data_ptr = reinterpret_cast<char const*>(bin_data.data());
    out = std::copy(data_ptr, data_ptr + size, out);
    return out;
  }
};

template <>
struct quill::Codec<WeatherBinaryData> : quill::BinaryDataDeferredFormatCodec<WeatherBinaryData>
{
};

int main()
{
  // Initialize Quill backend
  quill::BackendOptions backend_options;
  backend_options.check_printable_char = {};
  quill::Backend::start(backend_options);

  quill::PatternFormatterOptions binary_pfo;
  binary_pfo.format_pattern = "%(message)";
  binary_pfo.add_metadata_to_multi_line_logs = false;
  binary_pfo.pattern_suffix = quill::PatternFormatterOptions::NO_SUFFIX;

  // Create a file sink and logger
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("weather_data.bin",
                                                                        []()
                                                                        {
                                                                          quill::FileSinkConfig cfg;
                                                                          cfg.set_open_mode("wb");
                                                                          return cfg;
                                                                        }());

  quill::Logger* logger =
    quill::Frontend::create_or_get_logger("weather_monitor", std::move(file_sink), binary_pfo);

  // Buffer for our binary messages
  std::array<uint8_t, 128> buffer{};
  uint32_t message_size{0};

  // Log different types of weather data in rotation
  for (uint32_t i = 0; i < 15; ++i)
  {
    // Simulate encoding into the buffer different weather message types
    if ((i % 3) == 0)
    {
      // encode temperature reading data
      TemperatureReading temp;
      temp.id = 1;
      temp.celsius = 20 + i;
      temp.humidity = 50 + (i * 2);

      std::memcpy(buffer.data(), &temp, sizeof(TemperatureReading));
      message_size = sizeof(TemperatureReading);
    }
    else if ((i % 3) == 1)
    {
      // encode wind data
      WindData wind;
      wind.id = 2;
      wind.timestamp = i * 1000;
      wind.speed_kmh = i * 5;
      wind.storm_warning = (i % 4 == 0); // Storm warning every 4th reading

      std::memcpy(buffer.data(), &wind, sizeof(WindData));
      message_size = sizeof(WindData);
    }
    else if ((i % 3) == 2)
    {
      // encode weather station data
      WeatherStation station;
      station.id = 3;
      auto location = "Station" + std::to_string(i);
      strcpy(&station.location[0], location.c_str());

      std::memcpy(buffer.data(), &station, sizeof(WeatherStation));
      message_size = sizeof(WeatherStation);
    }

    // Pass the buffer for logging
    LOG_INFO(logger, "{}", WeatherBinaryData{buffer.data(), message_size});
  }

  quill::Backend::stop();

  // --- Decoding Phase for demonstration ---
  std::ifstream infile("weather_data.bin", std::ios::binary);
  if (!infile)
  {
    std::cerr << "Failed to open weather_data.bin\n";
    return 1;
  }

  while (true)
  {
    // Read size header
    uint32_t size;
    if (!infile.read(reinterpret_cast<char*>(&size), sizeof(size)))
      break; // EOF

    std::vector<uint8_t> data(size);
    if (!infile.read(reinterpret_cast<char*>(data.data()), size))
      break;

    // Interpret message
    if (size < sizeof(uint32_t))
    {
      std::cout << "Invalid binary data: too small\n";
      continue;
    }

    uint32_t id;
    std::memcpy(&id, data.data(), sizeof(uint32_t));
    std::stringstream oss;

    switch (id)
    {
    case 1:
      if (size >= sizeof(TemperatureReading))
      {
        TemperatureReading temp;
        std::memcpy(&temp, data.data(), sizeof(TemperatureReading));
        oss << temp;
      }
      else
      {
        oss << "Incomplete Temperature message";
      }
      break;
    case 2:
      if (size >= sizeof(WindData))
      {
        WindData wind;
        std::memcpy(&wind, data.data(), sizeof(WindData));
        oss << wind;
      }
      else
      {
        oss << "Incomplete Wind data message";
      }
      break;
    case 3:
      if (size >= sizeof(WeatherStation))
      {
        WeatherStation station;
        std::memcpy(&station, data.data(), sizeof(WeatherStation));
        oss << station;
      }
      else
      {
        oss << "Incomplete Weather Station message";
      }
      break;
    default:
      oss << "Unknown weather message type: " << id;
      break;
    }

    std::cout << oss.str() << "\n";
  }

  return 0;
}
