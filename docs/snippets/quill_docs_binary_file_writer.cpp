#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

/** A fixed-size record, e.g. an event received from a market data feed */
struct OrderEvent
{
  uint64_t order_id;
  uint64_t price;
  uint32_t quantity;
  char type; // 'A' add, 'U' update, 'D' delete
  char side; // 'B' buy, 'S' sell
  char padding[2];
};

int main()
{
  quill::Backend::start();

  // Every file of the sink starts with these bytes, e.g. a magic number and a version
  static constexpr std::string_view file_header{"ORDERS01"};

  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("orders.bin",
                                                                        []()
                                                                        {
                                                                          quill::FileSinkConfig cfg;
                                                                          cfg.set_open_mode('w');
                                                                          cfg.set_file_header(file_header);
                                                                          return cfg;
                                                                        }());

  // A logger of its own, so that the file has the records and nothing else
  quill::Logger* logger = quill::Frontend::create_or_get_logger("orders", std::move(file_sink));

  for (uint32_t i = 0; i < 6; ++i)
  {
    OrderEvent event{};
    event.order_id = 1000 + (i / 3);
    event.price = 21032 + i;
    event.quantity = 100 * (i + 1);
    event.type = "AUD"[i % 3];
    event.side = (i < 3) ? 'B' : 'S';

    // The bytes are copied to the queue here. The backend thread writes them to the file as
    // they are, in timestamp order
    LOG_BINARY(logger, &event, sizeof(event));
  }

  // Writes the queued records to the file
  quill::Backend::stop();

  // --- Reading the file back, for demonstration ---
  std::ifstream file{"orders.bin", std::ios::binary};

  std::string header(file_header.size(), '\0');
  if (!file.read(header.data(), static_cast<std::streamsize>(header.size())) || (header != file_header))
  {
    std::cerr << "Failed to read the header of orders.bin\n";
    return 1;
  }

  OrderEvent event;
  while (file.read(reinterpret_cast<char*>(&event), sizeof(event)))
  {
    std::cout << event.type << " order " << event.order_id << " " << event.side << " "
              << event.quantity << " @ " << event.price << "\n";
  }

  return 0;
}
