#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/bundled/fmt/ostream.h"
#include "quill/sinks/ConsoleSink.h"

#include <chrono>
#include <cstdint>
#include <ostream>
#include <utility>

class Order
{
public:
  Order(double in_price, int in_quantity)
    : timestamp(static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count())),
      price(in_price),
      quantity(in_quantity)
  {
  }

private:
  uint64_t timestamp;
  double price;
  int quantity;

  friend struct quill::DeferredFormatCodec<Order>;

  Order() = default;

  friend std::ostream& operator<<(std::ostream& os, Order const& order)
  {
    os << "timestamp=" << order.timestamp << " price=" << order.price << " quantity=" << order.quantity;
    return os;
  }
};

template <>
struct fmtquill::formatter<Order> : fmtquill::ostream_formatter
{
};

template <>
struct quill::Codec<Order> : quill::DeferredFormatCodec<Order>
{
};

int main()
{
  quill::Backend::start();

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  Order order{220.10, 100};
  LOG_INFO(logger, "Order is {}", order);
}
