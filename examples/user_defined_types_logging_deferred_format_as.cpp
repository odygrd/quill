#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/bundled/fmt/format.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <string>
#include <utility>

/**
 * This example shows deferred formatting for a user-defined type while sharing
 * formatting customisation between Quill's bundled `fmtquill` namespace and
 * standalone `fmt` through a single free `format_as()` function.
 *
 * The important part is that `format_as()` must live in the same namespace as
 * the user-defined type so that ADL can find it.
 *
 * `DeferredFormatCodec<T>` is still used for the Quill side, while
 * `format_as()` avoids writing both:
 *   - `fmtquill::formatter<T>`
 *   - `fmt::formatter<T>`
 *
 * as long as `format_as()` returns a type that both libraries already know how
 * to format, such as `std::string`, `std::string_view` or an arithmetic type.
 */

namespace custom
{
class User
{
public:
  User(std::string name, std::string department, uint32_t age)
    : name(std::move(name)), department(std::move(department)), age(age)
  {
  }

  std::string name;
  std::string department;
  uint32_t age{};
};

inline std::string format_as(User const& user)
{
  return "User(name=" + user.name + ", department=" + user.department +
    ", age=" + std::to_string(user.age) + ")";
}
} // namespace custom

template <>
struct quill::Codec<custom::User> : quill::DeferredFormatCodec<custom::User>
{
};

int main()
{
  quill::Backend::start();

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  custom::User user_1{"Alice", "Platform", 42};
  custom::User user_2{"Bob", "Infra", 33};

  // `fmtquill::format()` also picks up `custom::format_as(custom::User const&)`.
  std::string preview = fmtquill::format("{}", user_1);
  LOG_INFO(logger, "Preview via fmtquill::format: {}", preview);

  // If your project also uses standalone fmt, `fmt::format("{}", user_1)` would
  // use the same `custom::format_as(custom::User const&)` function.
  LOG_INFO(logger, "User 1 is {}", user_1);
  LOG_INFO(logger, "User 2 is {}", user_2);
}
