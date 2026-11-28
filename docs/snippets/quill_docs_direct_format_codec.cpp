#include "quill/Backend.h"
#include "quill/DirectFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/bundled/fmt/ranges.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class User
{
public:
  User(std::string in_name, std::string in_surname, uint32_t in_age)
    : name(std::move(in_name)), surname(std::move(in_surname)), age(in_age)
  {
    favorite_colors.push_back("red");
    favorite_colors.push_back("blue");
    favorite_colors.push_back("green");
  }

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
struct quill::Codec<User> : quill::DirectFormatCodec<User>
{
};

int main()
{
  quill::Backend::start();

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  User user{"Super", "User", 1};
  LOG_INFO(logger, "User is [{}]", user);
}
