#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Utility.h"
#include "quill/sinks/FileSink.h"

#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/std/Array.h"
#include "quill/std/Vector.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/**
 * CustomType defined type
 */
struct CustomType
{
  std::string name;
  std::string surname;
  uint32_t age;
  std::array<std::string, 3> favorite_colors;
};

/***/
template <>
struct fmtquill::formatter<CustomType>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::CustomType const& custom_type, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}, Favorite Colors: {}",
                               custom_type.name, custom_type.surname, custom_type.age,
                               custom_type.favorite_colors);
  }
};

/***/
template <>
struct quill::detail::ArgSizeCalculator<CustomType>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, ::CustomType const& custom_type) noexcept
  {
    // pass as arguments the class members you want to serialize
    return calculate_total_size(conditional_arg_size_cache, custom_type.name, custom_type.surname,
                                custom_type.age, custom_type.favorite_colors);
  }
};

/***/
template <>
struct quill::detail::Encoder<CustomType>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, ::CustomType const& custom_type) noexcept
  {
    // You must encode the same members and in the same order as in the ArgSizeCalculator::calculate
    encode_members(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                   custom_type.name, custom_type.surname, custom_type.age, custom_type.favorite_colors);
  }
};

/***/
template <>
struct quill::detail::Decoder<CustomType>
{
  static ::CustomType decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    // You must decode the same members and in the same order as in the Encoder::encode
    ::CustomType custom_type;
    decode_and_assign_members(buffer, args_store, custom_type, custom_type.name,
                              custom_type.surname, custom_type.age, custom_type.favorite_colors);
    return custom_type;
  }
};

/***/
TEST_CASE("custom_type_defined_type_logging")
{
  static constexpr char const* filename = "custom_type_defined_type_logging.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  // Set writing logging to a file
  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(
    logger_name, std::move(file_sink),
    "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) "
    "%(logger:<12) %(message)");

  logger->set_log_level(quill::LogLevel::TraceL3);

  CustomType custom_type;
  custom_type.name = "Quill";
  custom_type.surname = "Library";
  custom_type.age = 4;
  custom_type.favorite_colors[0] = "red";
  custom_type.favorite_colors[1] = "green";
  custom_type.favorite_colors[2] = "blue";

  LOG_INFO(logger, "The answer is {}", custom_type);

  std::vector<CustomType> const custom_types = {{"Alice", "Doe", 25, {"red", "green"}},
                                                {"Bob", "Smith", 30, {"blue", "yellow"}},
                                                {"Charlie", "Johnson", 35, {"green", "orange"}},
                                                {"David", "Brown", 40, {"red", "blue", "yellow"}}};

  LOG_INFO(logger, "The answers are {}", custom_types);

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 2);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       The answer is Name: Quill, Surname: Library, Age: 4, Favorite Colors: [\"red\", \"green\", \"blue\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       The answers are [Name: Alice, Surname: Doe, Age: 25, Favorite Colors: [\"red\", \"green\", \"\"], Name: Bob, Surname: Smith, Age: 30, Favorite Colors: [\"blue\", \"yellow\", \"\"], Name: Charlie, Surname: Johnson, Age: 35, Favorite Colors: [\"green\", \"orange\", \"\"], Name: David, Surname: Brown, Age: 40, Favorite Colors: [\"red\", \"blue\", \"yellow\"]]"}));

  testing::remove_file(filename);
}