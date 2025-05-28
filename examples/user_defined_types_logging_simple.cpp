#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/HelperMacros.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/bundled/fmt/ostream.h"
#include "quill/bundled/fmt/ranges.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

/**
 * Example of an UNSAFE type for asynchronous logging
 *
 * This class contains a pointer member which makes it unsafe
 * for asynchronous formatting (the pointer may become invalid).
 *
 * Note: Quill handles standard types and strings, and
 * other STL containers safely without additional configuration.
 *
 * Helper macros are only needed to enable custom user-defined types
 * that contain unsafe members like raw pointers or references.
 */
class User
{
public:
  std::string name;
  uint64_t* value_ptr{nullptr};
};

std::ostream& operator<<(std::ostream& os, User const& user)
{
  os << "User(name: " << user.name << ", value: " << (user.value_ptr ? *user.value_ptr : 0) << ")";
  return os;
}

// Use direct formatting for unsafe types with pointer members
// This will format the object immediately when the log statement is called
QUILL_LOGGABLE_DIRECT_FORMAT(User)

/**
 * Example of a SAFE type for asynchronous logging
 * This class contains only value types, making it safe for
 * asynchronous formatting
 */
class Product
{
public:
  std::string name;
  double price{0.0};
  int quantity{0};
};

std::ostream& operator<<(std::ostream& os, Product const& product)
{
  os << "Product(name: " << product.name << ", price: $" << product.price
     << ", quantity: " << product.quantity << ")";
  return os;
}

// Safe types can use asynchronous formatting
// The object will be copied and formatted later by the backend thread
QUILL_LOGGABLE_DEFERRED_FORMAT(Product)

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  // Example with unsafe User type (contains a pointer)
  uint64_t value;
  for (size_t i = 0; i < 10; ++i)
  {
    value = i * 100;
    User user{"Alice_" + std::to_string(i), &value};
    LOG_INFO(logger, "User is [{}]", user);
    // The User object will be formatted immediately
    // because we used QUILL_LOGGABLE_DIRECT_FORMAT
  }

  // Example with safe Product type (no pointers)
  for (size_t i = 0; i < 10; ++i)
  {
    Product product{"Widget_" + std::to_string(i), static_cast<double>(9.99 + i), static_cast<int>(i + 1)};
    LOG_INFO(logger, "Product is [{}]", product);
    // The Product object can be safely copied and formatted later
    // because we used QUILL_LOGGABLE_ASYNC_FORMAT
  }

  return 0;
}