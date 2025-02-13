#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/bundled/fmt/ranges.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <string>
#include <utility>

/**
 * This example illustrates logging user-defined types
 */

class User
{
public:
  User(std::string name, std::string surname, uint32_t age)
    : name(std::move(name)), surname(std::move(surname)), age(age)
  {
    favorite_colors.push_back("red");
    favorite_colors.push_back("blue");
    favorite_colors.push_back("green");
  };

  std::string name;
  std::string surname;
  uint32_t age{};
  std::vector<std::string> favorite_colors;
};

/***/
template <>
struct fmtquill::formatter<User>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::User const& user, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}, Favorite Colors: {}",
                               user.name, user.surname, user.age, user.favorite_colors);
  }
};

/***/
template <>
struct quill::Codec<User> : quill::DeferredFormatCodec<User>
{
};

/***/
int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  User user_1{"Super", "User", 1};

  LOG_INFO(logger, "User is [{}]", user_1);

  User user_2{"Another", "User", 12};

  LOG_INFO(logger, "User is [{}]", user_2);
}