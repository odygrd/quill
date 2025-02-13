#include "quill/Backend.h"
#include "quill/DirectFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/bundled/fmt/ostream.h"
#include "quill/bundled/fmt/ranges.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <string>
#include <utility>

/**
 * This example illustrates logging user-defined types by implicitly converting it to a
 * string in the hot path
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
struct quill::Codec<User> : quill::DirectFormatCodec<User>
{
};

class UserOStream
{
public:
  UserOStream(std::string name, std::string surname, uint32_t age)
    : name(std::move(name)), surname(std::move(surname)), age(age) {};

  friend std::ostream& operator<<(std::ostream& os, UserOStream const& obj)
  {
    os << "Name: " << obj.name << ", Surname: " << obj.surname << ", Age: " << obj.age;
    return os;
  }

private:
  std::string name;
  std::string surname;
  uint32_t age{};
};

template <>
struct quill::Codec<UserOStream> : quill::DirectFormatCodec<UserOStream>
{
};

/***/
template <>
struct fmtquill::formatter<UserOStream> : fmtquill::ostream_formatter
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

  UserOStream user_2{"Another", "User", 12};

  LOG_INFO(logger, "User is [{}]", user_2);
}