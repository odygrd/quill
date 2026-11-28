#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/ostream.h"

#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/std/Chrono.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>

class Order
{
public:
  Order(std::string in_symbol, double in_price, int in_quantity)
    : timestamp(std::chrono::system_clock::now()), symbol(std::move(in_symbol)), price(in_price), quantity(in_quantity)
  {
  }

private:
  std::chrono::system_clock::time_point timestamp;
  std::string symbol;
  double price;
  int quantity;

  template <typename T, typename U>
  friend struct quill::Codec;

  Order() = default;

  friend std::ostream& operator<<(std::ostream& os, Order const& order)
  {
    os << "timestamp=" << order.timestamp.time_since_epoch().count() << " symbol=" << order.symbol
       << " price=" << order.price << " quantity=" << order.quantity;
    return os;
  }
};

template <>
struct fmtquill::formatter<Order> : fmtquill::ostream_formatter
{
};

template <>
struct quill::Codec<Order>
{
  static size_t compute_encoded_size(quill::SizeCacheVector& conditional_arg_size_cache, ::Order const& order) noexcept
  {
    return compute_total_encoded_size(conditional_arg_size_cache, order.timestamp, order.symbol,
                                      order.price, order.quantity);
  }

  static void encode(std::byte*& buffer, quill::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, ::Order const& order) noexcept
  {
    encode_members(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                   order.timestamp, order.symbol, order.price, order.quantity);
  }

  static ::Order decode_arg(std::byte*& buffer)
  {
    ::Order order;
    decode_members(buffer, order, order.timestamp, order.symbol, order.price, order.quantity);
    return order;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

int main()
{
  quill::Backend::start();

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  Order order{"AAPL", 220.10, 100};
  LOG_INFO(logger, "Order is {}", order);
  LOGV_INFO(logger, "Order", order);
}
