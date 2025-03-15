/**
 * This example showcases passing user-defined types as arguments to the logger, with their
 * formatting deferred asynchronously to the backend. It's particularly useful in scenarios where
 * string formatting latency is unacceptable and the code operates on the critical path.
 */

#include "quill/Backend.h"
#include "quill/sinks/ConsoleSink.h"

// Header required for quill::Frontend::get_logger
#include "quill/Frontend.h"

// We need only these two headers in order to log
#include "quill/LogMacros.h"
#include "quill/Logger.h"

// Required only when passing to logger std::vector<User> for offloading the formatting to the backend
#include "quill/std/Vector.h"

// required includes for custom codec
#include "quill/bundled/fmt/format.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/std/Array.h" // To serialise the std::array member of User you need Array.h otherwise you don't need to include this
#include <utility> // for declval only required if you do the decoding manualy and use declval

/**
 * User defined type
 */
struct User
{
  std::string name;
  std::string surname;
  uint32_t age;
  std::array<std::string, 3> favorite_colors;
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
struct quill::Codec<User>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache, ::User const& user) noexcept
  {
    // pass as arguments the class members you want to serialize
    return compute_total_encoded_size(conditional_arg_size_cache, user.name, user.surname, user.age,
                                      user.favorite_colors);
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, ::User const& user) noexcept
  {
    // You must encode the same members and in the same order as in compute_total_encoded_size
    encode_members(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, user.name,
                   user.surname, user.age, user.favorite_colors);
  }

  static ::User decode_arg(std::byte*& buffer)
  {
    // You must decode the same members and in the same order as in encode
    ::User user;
    decode_members(buffer, user, user.name, user.surname, user.age, user.favorite_colors);
    return user;

    // note:
    // If the object is not default constructible you can also do it manually without
    // decode_members helper

    // auto name = Codec<decltype(std::declval<::User>().name)>::decode_arg(buffer);
    // auto surname = Codec<decltype(std::declval<::User>().surname)>::decode_arg(buffer);
    // auto age = Codec<decltype(std::declval<::User>().age)>::decode_arg(buffer);
    // auto favorite_colors = Codec<decltype(std::declval<::User>().favorite_colors)>::decode_arg(buffer);

    // ::User user{name, surname, age, favorite_colors};

    // return user;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

int main()
{
  // Start the backend thread
  quill::Backend::start();

  // Setup sink and logger
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console_sink");

  // Create and store the logger
  quill::Frontend::create_or_get_logger(
    "root", std::move(console_sink),
    quill::PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                                   "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                   "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

  quill::Logger* logger = quill::Frontend::get_logger("root");

  User user;
  user.name = "Quill";
  user.surname = "Library";
  user.age = 4;
  user.favorite_colors[0] = "red";
  user.favorite_colors[1] = "green";
  user.favorite_colors[2] = "blue";

  LOG_INFO(logger, "The user is {}", user);

  std::vector<User> const users = {{"Alice", "Doe", 25, {"red", "green"}},
                                   {"Bob", "Smith", 30, {"blue", "yellow"}},
                                   {"Charlie", "Johnson", 35, {"green", "orange"}},
                                   {"David", "Brown", 40, {"red", "blue", "yellow"}}};

  LOG_INFO(logger, "The users are {}", users);
}