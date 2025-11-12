#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/LogMacros.h"
#include "quill/Utility.h"
#include "quill/sinks/FileSink.h"

#include "quill/DeferredFormatCodec.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"

#include "quill/std/Array.h"
#include "quill/std/Deque.h"
#include "quill/std/ForwardList.h"
#include "quill/std/List.h"
#include "quill/std/Vector.h"

#include "quill/std/Optional.h"
#include "quill/std/Pair.h"
#include "quill/std/Tuple.h"

#include "quill/std/Map.h"
#include "quill/std/Set.h"
#include "quill/std/UnorderedMap.h"
#include "quill/std/UnorderedSet.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
struct CustomTypeTC
{
  CustomTypeTC(int n, double s, uint32_t a) : name(n), surname(s), age(a) {};
  CustomTypeTC() = default;

private:
  template <typename T, typename Char, typename Enable>
  friend struct fmtquill::formatter;

  //  template <typename T>
  //  friend struct quill::DeferredFormatCodec;

  // CustomTypeTC() = default;

  int name{};
  double surname{};
  uint32_t age{};
};

/***/
template <>
struct fmtquill::formatter<CustomTypeTC>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::CustomTypeTC const& custom_type, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}", custom_type.name,
                               custom_type.surname, custom_type.age);
  }
};

template <>
struct quill::Codec<CustomTypeTC> : quill::DeferredFormatCodec<CustomTypeTC>
{
};

static_assert(quill::DeferredFormatCodec<CustomTypeTC>::use_memcpy,
              "CustomTypeTC must be trivially copyable");

/***/
struct CustomTypeTCCC
{
  CustomTypeTCCC(int n, double s, uint32_t a) : name(n), surname(s), age(a) {}

  // Trivially copyable, No default ctor, but copy and move ctor exist

private:
  template <typename T, typename Char, typename Enable>
  friend struct fmtquill::formatter;

  int name{};
  double surname{};
  uint32_t age{};
};

/***/
template <>
struct fmtquill::formatter<CustomTypeTCCC>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::CustomTypeTCCC const& custom_type, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}", custom_type.name,
                               custom_type.surname, custom_type.age);
  }
};

template <>
struct quill::Codec<CustomTypeTCCC> : quill::DeferredFormatCodec<CustomTypeTCCC>
{
};

static_assert(std::is_trivially_copyable_v<CustomTypeTCCC>,
              "CustomTypeTCCC should be trivially copyable");
static_assert(!quill::DeferredFormatCodec<CustomTypeTCCC>::use_memcpy,
              "CustomTypeTCCC should not use_memcpy because no default ctor");

/***/
class CustomTypeCC
{
public:
  CustomTypeCC() = default;

  CustomTypeCC(std::string name, std::string surname, uint32_t age)
    : name(std::move(name)), surname(std::move(surname)), age(age)
  {
    favorite_colors.push_back("red");
    favorite_colors.push_back("blue");
    favorite_colors.push_back("green");
  };

  bool operator<(CustomTypeCC const& other) const { return this->age < other.age; }

  bool operator==(CustomTypeCC const& other) const
  {
    return this->age == other.age && this->name == other.name && this->surname == other.surname;
  }

  std::string name;
  std::string surname;
  uint32_t age{};
  std::vector<std::string> favorite_colors;
};

/***/
struct CustomTypeCCHash
{
  std::size_t operator()(CustomTypeCC const& obj) const
  {
    return std::hash<uint32_t>()(obj.age); // Hash only the `age`
  }
};

/***/
template <>
struct fmtquill::formatter<CustomTypeCC>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::CustomTypeCC const& user, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}, Favorite Colors: {}",
                               user.name, user.surname, user.age, user.favorite_colors);
  }
};

/***/
template <>
struct quill::Codec<CustomTypeCC> : quill::DeferredFormatCodec<CustomTypeCC>
{
};

/***/
class CustomTypeCCThrows
{
public:
  CustomTypeCCThrows() = default;

  CustomTypeCCThrows(CustomTypeCCThrows const&) { throw std::runtime_error("error"); }

  explicit CustomTypeCCThrows(std::string name) : name(std::move(name)) {};

  std::string name;
};

/***/
template <>
struct fmtquill::formatter<CustomTypeCCThrows>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::CustomTypeCCThrows const& user, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Name: {}", user.name);
  }
};

/***/
template <>
struct quill::Codec<CustomTypeCCThrows> : quill::DeferredFormatCodec<CustomTypeCCThrows>
{
};

/***/
class MoveOnlyType
{
public:
  MoveOnlyType() = default;

  explicit MoveOnlyType(std::string name, std::string value, uint32_t count)
    : name(static_cast<std::string&&>(name)), value(static_cast<std::string&&>(value)), count(count)
  {
  }

  MoveOnlyType(MoveOnlyType&&) = default;
  MoveOnlyType& operator=(MoveOnlyType&&) = default;

  // Delete copy operations to make it move-only
  MoveOnlyType(MoveOnlyType const&) = delete;
  MoveOnlyType& operator=(MoveOnlyType const&) = delete;

  std::string name;
  std::string value;
  uint32_t count{};
};

/***/
template <>
struct fmtquill::formatter<MoveOnlyType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::MoveOnlyType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "MoveOnlyType(name: {}, value: {}, count: {})", obj.name,
                               obj.value, obj.count);
  }
};

/***/
template <>
struct quill::Codec<MoveOnlyType> : quill::DeferredFormatCodec<MoveOnlyType>
{
};

/***/
class CopyOnlyType
{
public:
  CopyOnlyType() = default;

  explicit CopyOnlyType(std::string name, std::string value, uint32_t count)
    : name(name), value(value), count(count)
  {
  }

  CopyOnlyType(CopyOnlyType const&) = default;
  CopyOnlyType& operator=(CopyOnlyType const&) = default;

  // Delete move operations to make it copy-only
  CopyOnlyType(CopyOnlyType&&) = delete;
  CopyOnlyType& operator=(CopyOnlyType&&) = delete;

  std::string name;
  std::string value;
  uint32_t count{};
};

/***/
template <>
struct fmtquill::formatter<CopyOnlyType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::CopyOnlyType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "CopyOnlyType(name: {}, value: {}, count: {})", obj.name,
                               obj.value, obj.count);
  }
};

/***/
template <>
struct quill::Codec<CopyOnlyType> : quill::DeferredFormatCodec<CopyOnlyType>
{
};

/***/
class MoveAndCopyType
{
public:
  MoveAndCopyType() = default;

  explicit MoveAndCopyType(std::string name, std::string value, uint32_t count)
    : name(static_cast<std::string&&>(name)), value(static_cast<std::string&&>(value)), count(count)
  {
  }

  MoveAndCopyType(MoveAndCopyType&&) = default;
  MoveAndCopyType& operator=(MoveAndCopyType&&) = default;

  MoveAndCopyType(MoveAndCopyType const&) = default;
  MoveAndCopyType& operator=(MoveAndCopyType const&) = default;

  std::string name;
  std::string value;
  uint32_t count{};
};

/***/
template <>
struct fmtquill::formatter<MoveAndCopyType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::MoveAndCopyType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "MoveAndCopyType(name: {}, value: {}, count: {})",
                               obj.name, obj.value, obj.count);
  }
};

/***/
template <>
struct quill::Codec<MoveAndCopyType> : quill::DeferredFormatCodec<MoveAndCopyType>
{
};

/***/
TEST_CASE("custom_type_defined_type_deferred_format_logging")
{
  static constexpr char const* filename = "custom_type_defined_type_deferred_format_logging.log";
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
    PatternFormatterOptions{
      "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) "
      "%(logger:<12) %(message)"});

  logger->set_log_level(quill::LogLevel::TraceL3);

  {
    CustomTypeCCThrows custom_type_cct{"tt"};
    bool throws{false};

    try
    {
      LOG_INFO(logger, "CustomTypeCCThrows {}", custom_type_cct);
    }
    catch (std::exception const&)
    {
      throws = true;
    }

    REQUIRE(throws);
  }

  {
    CustomTypeTC custom_type_tc{1222, 13.12, 12};
    LOG_INFO(logger, "CustomTypeTC {}", custom_type_tc);
  }

  {
    CustomTypeTCCC custom_type_tccc{1233, 13.12, 12};
    LOG_INFO(logger, "CustomTypeTCCC {}", custom_type_tccc);
  }

  {
    CustomTypeTC custom_type_tcar[2] = {CustomTypeTC{1222, 13.12, 12}, CustomTypeTC{1222, 13.12, 12}};
    LOG_INFO(logger, "CustomTypeTCAr {}", custom_type_tcar);
  }

  {
    CustomTypeCC custom_type_ccar[2] = {CustomTypeCC{"Super", "User", 1},
                                        CustomTypeCC{"Super", "User", 1}};
    LOG_INFO(logger, "CustomTypeCCAr {}", custom_type_ccar);
  }

  {
    std::vector<CustomTypeTC> custom_type_tc_vec;
    custom_type_tc_vec.emplace_back(CustomTypeTC{111, 1.1, 13});
    custom_type_tc_vec.emplace_back(CustomTypeTC{123, 12.1, 23});
    custom_type_tc_vec.emplace_back(CustomTypeTC{456, 13.1, 33});
    LOG_INFO(logger, "CustomTypeTC Vec {}", custom_type_tc_vec);
  }

  {
    CustomTypeCC custom_type_cc{"Super", "User", 1};
    LOG_INFO(logger, "CustomTypeCC {}", custom_type_cc);
  }

  {
    std::vector<CustomTypeCC> custom_type_cc_vec;
    custom_type_cc_vec.emplace_back(CustomTypeCC{"Super", "User", 1});
    custom_type_cc_vec.emplace_back(CustomTypeCC{"Super-Duper", "User", 12});
    custom_type_cc_vec.emplace_back(CustomTypeCC{"Another", "Name", 13});
    LOG_INFO(logger, "CustomTypeCC Vec {}", custom_type_cc_vec);
  }

  {
    std::array<CustomTypeCC, 2> custom_type_arr;
    custom_type_arr[0] = CustomTypeCC{"Super", "User", 1};
    custom_type_arr[1] = CustomTypeCC{"Super", "User", 2};
    LOG_INFO(logger, "CustomTypeTC Array {}", custom_type_arr);
  }

  {
    std::deque<CustomTypeCC> custom_type_cc_deq;
    custom_type_cc_deq.emplace_back(CustomTypeCC{"Super", "User", 1});
    custom_type_cc_deq.emplace_back(CustomTypeCC{"Super-Duper", "User", 12});
    custom_type_cc_deq.emplace_back(CustomTypeCC{"Another", "Name", 13});
    LOG_INFO(logger, "CustomTypeCC Deq {}", custom_type_cc_deq);
  }

  {
    std::forward_list<CustomTypeCC> custom_type_cc_flist;
    custom_type_cc_flist.push_front(CustomTypeCC{"Another", "Name", 13});
    custom_type_cc_flist.push_front(CustomTypeCC{"Super-Duper", "User", 12});
    custom_type_cc_flist.push_front(CustomTypeCC{"Super", "User", 1});
    LOG_INFO(logger, "CustomTypeCC FList {}", custom_type_cc_flist);
  }

  {
    std::list<CustomTypeCC> custom_type_cc_list;
    custom_type_cc_list.push_back(CustomTypeCC{"Super", "User", 1});
    custom_type_cc_list.push_back(CustomTypeCC{"Super-Duper", "User", 12});
    custom_type_cc_list.push_back(CustomTypeCC{"Another", "Name", 13});
    LOG_INFO(logger, "CustomTypeCC List {}", custom_type_cc_list);
  }

  {
    std::optional<CustomTypeCC> custom_type_opt;
    custom_type_opt = CustomTypeCC{"Super", "User", 1};
    LOG_INFO(logger, "CustomTypeCC Opt {}", custom_type_opt);
  }

  {
    auto custom_type_pair =
      std::make_pair(CustomTypeCC{"Super", "User", 1}, CustomTypeCC{"Super", "User", 2});
    LOG_INFO(logger, "CustomTypeCC Pair {}", custom_type_pair);
  }

  {
    auto custom_type_tuple =
      std::make_tuple(CustomTypeCC{"Super", "User", 1}, CustomTypeCC{"Super", "User", 2},
                      CustomTypeCC{"Super", "User", 3});
    LOG_INFO(logger, "CustomTypeCC Tuple {}", custom_type_tuple);
  }

  {
    std::map<int, CustomTypeCC> custom_type_cc_map;
    custom_type_cc_map.emplace(1, CustomTypeCC{"Another", "Name", 13});
    custom_type_cc_map.emplace(2, CustomTypeCC{"Super-Duper", "User", 12});
    LOG_INFO(logger, "CustomTypeCC Map {}", custom_type_cc_map);
  }

  {
    std::set<CustomTypeCC> custom_type_cc_set;
    custom_type_cc_set.emplace("Another", "Name", 1);
    custom_type_cc_set.emplace("Super-Duper", "User", 2);
    LOG_INFO(logger, "CustomTypeCC Set {}", custom_type_cc_set);
  }

  {
    std::unordered_map<int, CustomTypeCC> custom_type_cc_unmap;
    custom_type_cc_unmap.emplace(1, CustomTypeCC{"Another", "Name", 13});
    LOG_INFO(logger, "CustomTypeCC UnMap {}", custom_type_cc_unmap);
  }

  {
    std::unordered_set<CustomTypeCC, CustomTypeCCHash> custom_type_cc_unset;
    custom_type_cc_unset.emplace(CustomTypeCC{"Another", "Name", 13});
    LOG_INFO(logger, "CustomTypeCC UnSet {}", custom_type_cc_unset);
  }

  // Test move-only type with std::move() using LOG_INFO macro
  {
    MoveOnlyType move_obj{"Alice", "MovedValue1", 42};
    LOG_INFO(logger, "MoveOnly std::move with macro {}", static_cast<MoveOnlyType&&>(move_obj));
  }

  // Test move-only type with temporary using LOG_INFO macro
  {
    LOG_INFO(logger, "MoveOnly temporary with macro {}", MoveOnlyType{"Bob", "TempValue", 99});
  }

  // Test move-only type with std::move() using quill::info function
  {
    MoveOnlyType move_obj_fn{"Charlie", "MovedValue2", 123};
    quill::info(logger, "MoveOnly std::move with function {}", static_cast<MoveOnlyType&&>(move_obj_fn));
  }

  // Test move-only type with temporary using quill::info function
  {
    quill::info(logger, "MoveOnly temporary with function {}", MoveOnlyType{"Diana", "TempValue2", 456});
  }

  // Test copy-only type with LOG_INFO macro
  {
    CopyOnlyType copy_obj{"Eve", "CopiedValue1", 50};
    LOG_INFO(logger, "CopyOnly with macro {}", copy_obj);
  }

  // Test copy-only type with temporary using LOG_INFO macro
  {
    LOG_INFO(logger, "CopyOnly temporary with macro {}", CopyOnlyType{"Frank", "TempValue3", 77});
  }

  // Test copy-only type using quill::info function
  {
    CopyOnlyType copy_obj_fn{"Grace", "CopiedValue2", 88};
    quill::info(logger, "CopyOnly with function {}", copy_obj_fn);
  }

  // Test copy-only type with temporary using quill::info function
  {
    quill::info(logger, "CopyOnly temporary with function {}", CopyOnlyType{"Helen", "TempValue4", 200});
  }

  // Test type with both move and copy - lvalue should use copy to preserve object
  {
    MoveAndCopyType both_obj{"Ian", "BothValue1", 111};
    LOG_INFO(logger, "MoveAndCopy lvalue with macro {}", both_obj);
    // Verify object is still usable after logging (it was copied, not moved)
    REQUIRE_EQ(both_obj.name, "Ian");
  }

  // Test type with both move and copy - explicit move
  {
    MoveAndCopyType both_obj_moved{"Jane", "BothValue2", 222};
    LOG_INFO(logger, "MoveAndCopy std::move with macro {}", static_cast<MoveAndCopyType&&>(both_obj_moved));
  }

  // Test type with both move and copy - temporary (should move)
  {
    LOG_INFO(logger, "MoveAndCopy temporary with macro {}", MoveAndCopyType{"Karl", "TempValue3", 333});
  }

  // Test type with both move and copy - lvalue with function should use copy
  {
    MoveAndCopyType both_obj_fn{"Laura", "BothValue4", 444};
    quill::info(logger, "MoveAndCopy lvalue with function {}", both_obj_fn);
    // Verify object is still usable after logging (it was copied, not moved)
    REQUIRE_EQ(both_obj_fn.name, "Laura");
  }

  // Test type with both move and copy - explicit move with function
  {
    MoveAndCopyType both_obj_fn_moved{"Mike", "BothValue5", 555};
    quill::info(logger, "MoveAndCopy std::move with function {}",
                static_cast<MoveAndCopyType&&>(both_obj_fn_moved));
  }

  // Test type with both move and copy - temporary with function (should move)
  {
    quill::info(logger, "MoveAndCopy temporary with function {}",
                MoveAndCopyType{"Nancy", "TempValue6", 666});
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 32);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeTC Name: 1222, Surname: 13.12, Age: 12"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeTCCC Name: 1233, Surname: 13.12, Age: 12"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeTCAr [Name: 1222, Surname: 13.12, Age: 12, Name: 1222, Surname: 13.12, Age: 12]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCCAr [Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeTC Vec [Name: 111, Surname: 1.1, Age: 13, Name: 123, Surname: 12.1, Age: 23, Name: 456, Surname: 13.1, Age: 33]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC Vec [Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Super-Duper, Surname: User, Age: 12, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Another, Surname: Name, Age: 13, Favorite Colors: [\"red\", \"blue\", \"green\"]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeTC Array [Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Super, Surname: User, Age: 2, Favorite Colors: [\"red\", \"blue\", \"green\"]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC Deq [Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Super-Duper, Surname: User, Age: 12, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Another, Surname: Name, Age: 13, Favorite Colors: [\"red\", \"blue\", \"green\"]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC FList [Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Super-Duper, Surname: User, Age: 12, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Another, Surname: Name, Age: 13, Favorite Colors: [\"red\", \"blue\", \"green\"]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC List [Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Super-Duper, Surname: User, Age: 12, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Another, Surname: Name, Age: 13, Favorite Colors: [\"red\", \"blue\", \"green\"]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC Opt optional(Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"])"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC Pair (Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Super, Surname: User, Age: 2, Favorite Colors: [\"red\", \"blue\", \"green\"])"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC Tuple (Name: Super, Surname: User, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Super, Surname: User, Age: 2, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Super, Surname: User, Age: 3, Favorite Colors: [\"red\", \"blue\", \"green\"])"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC Map {1: Name: Another, Surname: Name, Age: 13, Favorite Colors: [\"red\", \"blue\", \"green\"], 2: Name: Super-Duper, Surname: User, Age: 12, Favorite Colors: [\"red\", \"blue\", \"green\"]}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC Set {Name: Another, Surname: Name, Age: 1, Favorite Colors: [\"red\", \"blue\", \"green\"], Name: Super-Duper, Surname: User, Age: 2, Favorite Colors: [\"red\", \"blue\", \"green\"]}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC UnMap {1: Name: Another, Surname: Name, Age: 13, Favorite Colors: [\"red\", \"blue\", \"green\"]}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CustomTypeCC UnSet {Name: Another, Surname: Name, Age: 13, Favorite Colors: [\"red\", \"blue\", \"green\"]}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       MoveOnly std::move with macro MoveOnlyType(name: Alice, value: MovedValue1, count: 42)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       MoveOnly temporary with macro MoveOnlyType(name: Bob, value: TempValue, count: 99)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       MoveOnly std::move with function MoveOnlyType(name: Charlie, value: MovedValue2, count: 123)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       MoveOnly temporary with function MoveOnlyType(name: Diana, value: TempValue2, count: 456)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CopyOnly with macro CopyOnlyType(name: Eve, value: CopiedValue1, count: 50)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CopyOnly temporary with macro CopyOnlyType(name: Frank, value: TempValue3, count: 77)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CopyOnly with function CopyOnlyType(name: Grace, value: CopiedValue2, count: 88)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       CopyOnly temporary with function CopyOnlyType(name: Helen, value: TempValue4, count: 200)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       MoveAndCopy lvalue with macro MoveAndCopyType(name: Ian, value: BothValue1, count: 111)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       MoveAndCopy std::move with macro MoveAndCopyType(name: Jane, value: BothValue2, count: 222)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       MoveAndCopy temporary with macro MoveAndCopyType(name: Karl, value: TempValue3, count: 333)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       MoveAndCopy lvalue with function MoveAndCopyType(name: Laura, value: BothValue4, count: 444)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       MoveAndCopy std::move with function MoveAndCopyType(name: Mike, value: BothValue5, count: 555)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       MoveAndCopy temporary with function MoveAndCopyType(name: Nancy, value: TempValue6, count: 666)"}));

  testing::remove_file(filename);
}