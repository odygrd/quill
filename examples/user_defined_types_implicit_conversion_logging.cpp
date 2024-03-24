#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

// Header for formatting utilities, it must be always included when `LogMacrosFmt.h` is used
#include "quill/bundled/fmt/format.h"

// Header for formatting ranges such as std::array and std::vector
#include "quill/bundled/fmt/ranges.h"

// Header for formatting std types, e.g std::optional
#include "quill/bundled/fmt/std.h"

// Header for implicitly converting user defined and std types to strings
#include "quill/LogMacrosFmt.h"

#include <array>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * This example demonstrates logging user-defined and standard library types using the internal
 * libfmt and 'LogMacrosFmt.h`, eliminating the need for explicit string conversion as seen in
 * the `user_defined_types_logging` example.
 *
 * Starting from version 4.0.0, direct passing of user-defined and standard library types to the
 * 'LOG_' macros is no longer supported. Instead, these types must be converted to strings
 * before being passed to the logger.
 */

class User
{
public:
  User(std::string name, std::string surname, uint32_t age)
    : name(std::move(name)), surname(std::move(surname)), age(age){};

  friend std::ostream& operator<<(std::ostream& os, User const& obj)
  {
    os << "name: " << obj.name << ", surname: " << obj.surname << ", age: " << obj.age;
    return os;
  }

  friend struct fmtquill::formatter<User>;

private:
  std::string name;
  std::string surname;
  uint32_t age;
};

// fmt::formatter for User
template <>
struct fmtquill::formatter<User>
{
  template <typename FormatContext>
  auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(User const& user, FormatContext& ctx)
  {
    return fmtquill::format_to(ctx.out(), "User: {} {}, Age: {}", user.name, user.surname, user.age);
  }
};

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  User user_1{"Super", "User", 1};
  LOG_INFO(logger, "User is: {}", user_1);

  std::array<int, 4> a{1, 2, 3, 4};
  LOG_INFO(logger, "array is: {}", a);

  std::vector<int> v{4, 3, 2, 1};
  LOG_INFO(logger, "vector is: {}", v);

  std::unordered_map<std::string, std::string> u{{"one", "1"}, {"two", "2"}};
  LOG_INFO(logger, "unordered_map is: {}", u);

  std::optional<int> empty_opt;
  std::optional<int> opt{123};
  LOG_INFO(logger, "empty_opt is: {}, opt is: {}", empty_opt, opt);

  std::pair<int, double> p{1, 3.14};
  LOG_INFO(logger, "pair formatted to string on the frontend {}", p);

  LOG_INFO(
    logger,
    "or pass the primitives types, a copy of the types will be taken and they will be formatted by "
    "the backend thread for lower latency on the frontend. first: {}, second: {}",
    p.first, p.second);
}