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
    : name(std::move(name)), value(std::move(value)), count(count)
  {
  }

  MoveOnlyType(MoveOnlyType&&) = default;
  MoveOnlyType& operator=(MoveOnlyType&&) = default;

  // Delete copy operations to make it move-only
  MoveOnlyType(MoveOnlyType const&) = delete;
  MoveOnlyType& operator=(MoveOnlyType const&) = delete;

  bool operator==(MoveOnlyType const& other) const
  {
    return name == other.name && value == other.value && count == other.count;
  }

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

  bool operator==(CopyOnlyType const& other) const
  {
    return name == other.name && value == other.value && count == other.count;
  }

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
    : name(std::move(name)), value(std::move(value)), count(count)
  {
  }

  MoveAndCopyType(MoveAndCopyType&&) = default;
  MoveAndCopyType& operator=(MoveAndCopyType&&) = default;

  MoveAndCopyType(MoveAndCopyType const&) = default;
  MoveAndCopyType& operator=(MoveAndCopyType const&) = default;

  bool operator==(MoveAndCopyType const& other) const
  {
    return name == other.name && value == other.value && count == other.count;
  }

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

// Hash specializations for unordered containers
namespace std
{
template <>
struct hash<MoveOnlyType>
{
  size_t operator()(MoveOnlyType const& obj) const noexcept
  {
    return hash<std::string>{}(obj.name) ^ (hash<std::string>{}(obj.value) << 1) ^
      (hash<uint32_t>{}(obj.count) << 2);
  }
};

template <>
struct hash<CopyOnlyType>
{
  size_t operator()(CopyOnlyType const& obj) const noexcept
  {
    return hash<std::string>{}(obj.name) ^ (hash<std::string>{}(obj.value) << 1) ^
      (hash<uint32_t>{}(obj.count) << 2);
  }
};

template <>
struct hash<MoveAndCopyType>
{
  size_t operator()(MoveAndCopyType const& obj) const noexcept
  {
    return hash<std::string>{}(obj.name) ^ (hash<std::string>{}(obj.value) << 1) ^
      (hash<uint32_t>{}(obj.count) << 2);
  }
};
} // namespace std

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
    LOG_INFO(logger, "MoveOnly std::move with macro {}", std::move(move_obj));
  }

  // Test move-only type with temporary using LOG_INFO macro
  {
    LOG_INFO(logger, "MoveOnly temporary with macro {}", MoveOnlyType{"Bob", "TempValue", 99});
  }

  // Test move-only type with std::move() using quill::info function
  {
    MoveOnlyType move_obj_fn{"Charlie", "MovedValue2", 123};
    quill::info(logger, "MoveOnly std::move with function {}", std::move(move_obj_fn));
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
    LOG_INFO(logger, "MoveAndCopy std::move with macro {}", std::move(both_obj_moved));
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
    quill::info(logger, "MoveAndCopy std::move with function {}", std::move(both_obj_fn_moved));
  }

  // Test type with both move and copy - temporary with function (should move)
  {
    quill::info(logger, "MoveAndCopy temporary with function {}",
                MoveAndCopyType{"Nancy", "TempValue6", 666});
  }

  // Test STL containers with MoveOnlyType
  {
    std::vector<MoveOnlyType> move_only_vec;
    move_only_vec.emplace_back("Alice", "Value1", 1);
    move_only_vec.emplace_back("Bob", "Value2", 2);
    LOG_INFO(logger, "Vector<MoveOnlyType> {}", std::move(move_only_vec));
  }

  {
    std::list<MoveOnlyType> move_only_list;
    move_only_list.emplace_back("Charlie", "Value3", 3);
    move_only_list.emplace_back("Diana", "Value4", 4);
    LOG_INFO(logger, "List<MoveOnlyType> {}", std::move(move_only_list));
  }

  {
    std::list<CopyOnlyType> copy_only_list;
    copy_only_list.emplace_back("Grace", "Value7", 7);
    copy_only_list.emplace_back("Helen", "Value8", 8);
    LOG_INFO(logger, "List<CopyOnlyType> {}", copy_only_list);
  }

  // Test STL containers with MoveAndCopyType
  {
    std::vector<MoveAndCopyType> move_copy_vec;
    move_copy_vec.emplace_back("Ian", "Value9", 9);
    move_copy_vec.emplace_back("Jane", "Value10", 10);
    LOG_INFO(logger, "Vector<MoveAndCopyType> lvalue {}", move_copy_vec);
    // Verify vector still usable (was copied)
    REQUIRE_EQ(move_copy_vec.size(), 2);
    REQUIRE_EQ(move_copy_vec[0].name, "Ian");
    REQUIRE_EQ(move_copy_vec[0].value, "Value9");
    REQUIRE_EQ(move_copy_vec[0].count, 9);
  }

  {
    std::vector<MoveAndCopyType> move_copy_vec_moved;
    move_copy_vec_moved.emplace_back("Karl", "Value11", 11);
    move_copy_vec_moved.emplace_back("Laura", "Value12", 12);
    LOG_INFO(logger, "Vector<MoveAndCopyType> std::move {}", std::move(move_copy_vec_moved));
  }

  {
    std::list<MoveAndCopyType> move_copy_list;
    move_copy_list.emplace_back("Mike", "Value13", 13);
    move_copy_list.emplace_back("Nancy", "Value14", 14);
    LOG_INFO(logger, "List<MoveAndCopyType> {}", move_copy_list);
  }

  // Test std::deque with MoveOnlyType
  {
    std::deque<MoveOnlyType> move_only_deque;
    move_only_deque.emplace_back("Oscar", "Value15", 15);
    move_only_deque.emplace_back("Paul", "Value16", 16);
    LOG_INFO(logger, "Deque<MoveOnlyType> {}", std::move(move_only_deque));
  }

  // Test std::deque with CopyOnlyType
  {
    std::deque<CopyOnlyType> copy_only_deque;
    copy_only_deque.emplace_back("Quinn", "Value17", 17);
    copy_only_deque.emplace_back("Rachel", "Value18", 18);
    LOG_INFO(logger, "Deque<CopyOnlyType> {}", copy_only_deque);
  }

  // Test std::deque with MoveAndCopyType
  {
    std::deque<MoveAndCopyType> move_copy_deque;
    move_copy_deque.emplace_back("Sam", "Value19", 19);
    move_copy_deque.emplace_back("Tina", "Value20", 20);
    LOG_INFO(logger, "Deque<MoveAndCopyType> {}", move_copy_deque);
    // Verify object is still valid after logging without std::move
    REQUIRE_EQ(move_copy_deque.size(), 2);
    REQUIRE_EQ(move_copy_deque[0].name, "Sam");
    REQUIRE_EQ(move_copy_deque[0].value, "Value19");
  }

  // Test std::set with MoveOnlyType
  {
    std::set<int> move_only_set_keys;
    move_only_set_keys.insert(1);
    move_only_set_keys.insert(2);
    LOG_INFO(logger, "Set<int> {}", std::move(move_only_set_keys));
  }

  // Test std::map with MoveOnlyType values
  {
    std::map<int, MoveOnlyType> move_only_map;
    move_only_map.emplace(1, MoveOnlyType("Uma", "Value21", 21));
    move_only_map.emplace(2, MoveOnlyType("Victor", "Value22", 22));
    LOG_INFO(logger, "Map<int,MoveOnlyType> {}", std::move(move_only_map));
  }

  // Test std::map with CopyOnlyType values
  {
    std::map<int, CopyOnlyType> copy_only_map;
    copy_only_map.emplace(1, CopyOnlyType("Wendy", "Value23", 23));
    copy_only_map.emplace(2, CopyOnlyType("Xavier", "Value24", 24));
    LOG_INFO(logger, "Map<int,CopyOnlyType> {}", copy_only_map);
  }

  // Test std::map with MoveAndCopyType values
  {
    std::map<int, MoveAndCopyType> move_copy_map;
    move_copy_map.emplace(1, MoveAndCopyType("Yara", "Value25", 25));
    move_copy_map.emplace(2, MoveAndCopyType("Zack", "Value26", 26));
    LOG_INFO(logger, "Map<int,MoveAndCopyType> {}", move_copy_map);
    // Verify object is still valid after logging without std::move
    REQUIRE_EQ(move_copy_map.size(), 2);
    REQUIRE_EQ(move_copy_map[1].name, "Yara");
    REQUIRE_EQ(move_copy_map[1].value, "Value25");
  }

  // Test std::unordered_map with MoveOnlyType values
  {
    std::unordered_map<int, MoveOnlyType> move_only_unordered_map;
    move_only_unordered_map.emplace(1, MoveOnlyType("Alice", "Value101", 101));
    move_only_unordered_map.emplace(2, MoveOnlyType("Bob", "Value102", 102));
    LOG_INFO(logger, "UnorderedMap<int,MoveOnlyType> {}", std::move(move_only_unordered_map));
  }

  // Test std::unordered_map with CopyOnlyType values
  {
    std::unordered_map<int, CopyOnlyType> copy_only_unordered_map;
    copy_only_unordered_map.emplace(std::piecewise_construct, std::forward_as_tuple(1),
                                    std::forward_as_tuple("Charlie", "Value103", 103));
    copy_only_unordered_map.emplace(std::piecewise_construct, std::forward_as_tuple(2),
                                    std::forward_as_tuple("Diana", "Value104", 104));
    LOG_INFO(logger, "UnorderedMap<int,CopyOnlyType> {}", copy_only_unordered_map);
  }

  // Test std::unordered_map with MoveAndCopyType values
  {
    std::unordered_map<int, MoveAndCopyType> move_copy_unordered_map;
    move_copy_unordered_map.emplace(1, MoveAndCopyType("Eve", "Value105", 105));
    move_copy_unordered_map.emplace(2, MoveAndCopyType("Frank", "Value106", 106));
    LOG_INFO(logger, "UnorderedMap<int,MoveAndCopyType> {}", move_copy_unordered_map);
    // Verify object is still valid after logging without std::move
    REQUIRE_EQ(move_copy_unordered_map.size(), 2);
    REQUIRE_EQ(move_copy_unordered_map[1].name, "Eve");
    REQUIRE_EQ(move_copy_unordered_map[1].value, "Value105");
  }

  // Test std::unordered_set with CopyOnlyType
  {
    std::unordered_set<CopyOnlyType> copy_only_uset;
    copy_only_uset.emplace("Nancy", "Value115", 115);
    copy_only_uset.emplace("Oscar", "Value116", 116);
    LOG_INFO(logger, "UnorderedSet<CopyOnlyType> {}", copy_only_uset);
  }

  // Test std::unordered_set with MoveAndCopyType
  {
    std::unordered_set<MoveAndCopyType> move_copy_uset;
    move_copy_uset.emplace(MoveAndCopyType("Paula", "Value117", 117));
    move_copy_uset.emplace(MoveAndCopyType("Quinn", "Value118", 118));
    LOG_INFO(logger, "UnorderedSet<MoveAndCopyType> {}", move_copy_uset);
    // Verify object is still valid after logging without std::move
    REQUIRE_EQ(move_copy_uset.size(), 2);
    auto it = move_copy_uset.begin();
    REQUIRE_FALSE(it->name.empty());
  }

  // Test std::forward_list with MoveOnlyType
  {
    std::forward_list<MoveOnlyType> move_only_flist;
    move_only_flist.emplace_front(MoveOnlyType("Grace", "Value107", 107));
    move_only_flist.emplace_front(MoveOnlyType("Henry", "Value108", 108));
    LOG_INFO(logger, "ForwardList<MoveOnlyType> {}", std::move(move_only_flist));
  }

  // Test std::forward_list with CopyOnlyType
  {
    std::forward_list<CopyOnlyType> copy_only_flist;
    copy_only_flist.emplace_front("Iris", "Value109", 109);
    copy_only_flist.emplace_front("Jack", "Value110", 110);
    LOG_INFO(logger, "ForwardList<CopyOnlyType> {}", copy_only_flist);
  }

  // Test std::forward_list with MoveAndCopyType
  {
    std::forward_list<MoveAndCopyType> move_copy_flist;
    move_copy_flist.emplace_front(MoveAndCopyType("Kate", "Value111", 111));
    move_copy_flist.emplace_front(MoveAndCopyType("Leo", "Value112", 112));
    LOG_INFO(logger, "ForwardList<MoveAndCopyType> {}", move_copy_flist);
    // Verify object is still valid after logging without std::move
    REQUIRE_EQ(move_copy_flist.front().name, "Leo");
    REQUIRE_EQ(move_copy_flist.front().value, "Value112");
  }

  // Test std::array with MoveOnlyType
  {
    std::array<MoveOnlyType, 2> move_only_array;
    move_only_array[0] = MoveOnlyType("Amy", "Value27", 27);
    move_only_array[1] = MoveOnlyType("Ben", "Value28", 28);
    LOG_INFO(logger, "Array<MoveOnlyType> {}", std::move(move_only_array));
  }

  // Test std::array with CopyOnlyType
  {
    std::array<CopyOnlyType, 2> copy_only_array{CopyOnlyType("Carol", "Value29", 29),
                                                CopyOnlyType("Dan", "Value30", 30)};
    LOG_INFO(logger, "Array<CopyOnlyType> {}", copy_only_array);
  }

  // Test std::array with MoveAndCopyType
  {
    std::array<MoveAndCopyType, 2> move_copy_array;
    move_copy_array[0] = MoveAndCopyType("Emma", "Value31", 31);
    move_copy_array[1] = MoveAndCopyType("Frank", "Value32", 32);
    LOG_INFO(logger, "Array<MoveAndCopyType> {}", move_copy_array);
    // Verify object is still valid after logging without std::move
    REQUIRE_EQ(move_copy_array[0].name, "Emma");
    REQUIRE_EQ(move_copy_array[0].value, "Value31");
  }

  // Test std::pair with MoveOnlyType
  {
    std::pair<MoveOnlyType, MoveOnlyType> move_only_pair{MoveOnlyType("George", "Value33", 33),
                                                         MoveOnlyType("Hannah", "Value34", 34)};
    LOG_INFO(logger, "Pair<MoveOnlyType> {}", std::move(move_only_pair));
  }

  // Test std::pair with CopyOnlyType
  {
    std::pair<CopyOnlyType, CopyOnlyType> copy_only_pair{CopyOnlyType("Iris", "Value35", 35),
                                                         CopyOnlyType("Jack", "Value36", 36)};
    LOG_INFO(logger, "Pair<CopyOnlyType> {}", copy_only_pair);
  }

  // Test std::pair with MoveAndCopyType
  {
    std::pair<MoveAndCopyType, MoveAndCopyType> move_copy_pair{
      MoveAndCopyType("Kate", "Value37", 37), MoveAndCopyType("Leo", "Value38", 38)};
    LOG_INFO(logger, "Pair<MoveAndCopyType> {}", move_copy_pair);
    // Verify object is still valid after logging without std::move
    REQUIRE_EQ(move_copy_pair.first.name, "Kate");
    REQUIRE_EQ(move_copy_pair.first.value, "Value37");
  }

  // Test std::pair with MoveAndCopyType + MoveOnlyType (all elements move-constructible)
  {
    std::pair<MoveAndCopyType, MoveOnlyType> mixed_move_pair{
      MoveAndCopyType("Yolanda", "Value52", 52), MoveOnlyType("Zara", "Value53", 53)};
    LOG_INFO(logger, "Pair<MoveAndCopy+MoveOnly> {}", std::move(mixed_move_pair));
  }

#if !defined(__GNUC__) || __GNUC__ >= 10
  // Test std::pair with MoveAndCopyType + CopyOnlyType (all elements copy-constructible)
  {
    std::pair<MoveAndCopyType, CopyOnlyType> mixed_copy_pair{
      MoveAndCopyType("Albert", "Value54", 54), CopyOnlyType("Beth", "Value55", 55)};
    LOG_INFO(logger, "Pair<MoveAndCopy+CopyOnly> {}", mixed_copy_pair);
  }
#endif

  // Test std::tuple with MoveOnlyType
  {
    std::tuple<MoveOnlyType, MoveOnlyType> move_only_tuple{MoveOnlyType("Mia", "Value39", 39),
                                                           MoveOnlyType("Noah", "Value40", 40)};
    LOG_INFO(logger, "Tuple<MoveOnlyType> {}", std::move(move_only_tuple));
  }

  // Test std::tuple with CopyOnlyType
  {
    std::tuple<CopyOnlyType, CopyOnlyType> copy_only_tuple{CopyOnlyType("Olivia", "Value41", 41),
                                                           CopyOnlyType("Peter", "Value42", 42)};
    LOG_INFO(logger, "Tuple<CopyOnlyType> {}", copy_only_tuple);
  }

  // Test std::tuple with MoveAndCopyType
  {
    std::tuple<MoveAndCopyType, MoveAndCopyType> move_copy_tuple{
      MoveAndCopyType("Rose", "Value43", 43), MoveAndCopyType("Steve", "Value44", 44)};
    LOG_INFO(logger, "Tuple<MoveAndCopyType> {}", move_copy_tuple);
    // Verify object is still valid after logging without std::move
    REQUIRE_EQ(std::get<0>(move_copy_tuple).name, "Rose");
    REQUIRE_EQ(std::get<0>(move_copy_tuple).value, "Value43");
  }

  // Test std::tuple with MoveAndCopyType + MoveOnlyType (all elements move-constructible)
  {
    std::tuple<MoveAndCopyType, MoveOnlyType> mixed_move_tuple{
      MoveAndCopyType("Ulysses", "Value48", 48), MoveOnlyType("Vera", "Value49", 49)};
    LOG_INFO(logger, "Tuple<MoveAndCopy+MoveOnly> {}", std::move(mixed_move_tuple));
  }

#if !defined(__GNUC__) || __GNUC__ >= 10
  // Test std::tuple with MoveAndCopyType + CopyOnlyType (all elements copy-constructible)
  {
    std::tuple<MoveAndCopyType, CopyOnlyType> mixed_copy_tuple{
      MoveAndCopyType("Walter", "Value50", 50), CopyOnlyType("Xena", "Value51", 51)};
    LOG_INFO(logger, "Tuple<MoveAndCopy+CopyOnly> {}", mixed_copy_tuple);
  }
#endif

  // Test std::optional with MoveOnlyType
  {
    std::optional<MoveOnlyType> move_only_opt{MoveOnlyType("Tom", "Value45", 45)};
    LOG_INFO(logger, "Optional<MoveOnlyType> {}", std::move(move_only_opt));
  }

  // Test std::optional with CopyOnlyType
  {
    std::optional<CopyOnlyType> copy_only_opt{std::in_place, "Uma2", "Value46", 46};
    LOG_INFO(logger, "Optional<CopyOnlyType> {}", copy_only_opt);
  }

  // Test std::optional with MoveAndCopyType
  {
    std::optional<MoveAndCopyType> move_copy_opt{MoveAndCopyType("Vince", "Value47", 47)};
    LOG_INFO(logger, "Optional<MoveAndCopyType> {}", move_copy_opt);
    // Verify object is still valid after logging without std::move
    REQUIRE_EQ(move_copy_opt.value().name, "Vince");
    REQUIRE_EQ(move_copy_opt.value().value, "Value47");
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

#if !defined(__GNUC__) || __GNUC__ >= 10
  REQUIRE_EQ(file_contents.size(), 69);
#else
  REQUIRE_EQ(file_contents.size(), 67);
#endif

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

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Vector<MoveOnlyType> [MoveOnlyType(name: Alice, value: Value1, count: 1), MoveOnlyType(name: Bob, value: Value2, count: 2)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       List<MoveOnlyType> [MoveOnlyType(name: Charlie, value: Value3, count: 3), MoveOnlyType(name: Diana, value: Value4, count: 4)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       List<CopyOnlyType> [CopyOnlyType(name: Grace, value: Value7, count: 7), CopyOnlyType(name: Helen, value: Value8, count: 8)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Vector<MoveAndCopyType> lvalue [MoveAndCopyType(name: Ian, value: Value9, count: 9), MoveAndCopyType(name: Jane, value: Value10, count: 10)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Vector<MoveAndCopyType> std::move [MoveAndCopyType(name: Karl, value: Value11, count: 11), MoveAndCopyType(name: Laura, value: Value12, count: 12)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       List<MoveAndCopyType> [MoveAndCopyType(name: Mike, value: Value13, count: 13), MoveAndCopyType(name: Nancy, value: Value14, count: 14)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Deque<MoveOnlyType> [MoveOnlyType(name: Oscar, value: Value15, count: 15), MoveOnlyType(name: Paul, value: Value16, count: 16)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Deque<CopyOnlyType> [CopyOnlyType(name: Quinn, value: Value17, count: 17), CopyOnlyType(name: Rachel, value: Value18, count: 18)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Deque<MoveAndCopyType> [MoveAndCopyType(name: Sam, value: Value19, count: 19), MoveAndCopyType(name: Tina, value: Value20, count: 20)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Set<int> {1, 2}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Map<int,MoveOnlyType> {1: MoveOnlyType(name: Uma, value: Value21, count: 21), 2: MoveOnlyType(name: Victor, value: Value22, count: 22)}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Map<int,CopyOnlyType> {1: CopyOnlyType(name: Wendy, value: Value23, count: 23), 2: CopyOnlyType(name: Xavier, value: Value24, count: 24)}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Map<int,MoveAndCopyType> {1: MoveAndCopyType(name: Yara, value: Value25, count: 25), 2: MoveAndCopyType(name: Zack, value: Value26, count: 26)}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       UnorderedMap<int,MoveOnlyType> {"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"MoveOnlyType(name: Alice, value: Value101, count: 101)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"MoveOnlyType(name: Bob, value: Value102, count: 102)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       UnorderedMap<int,CopyOnlyType> {"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"CopyOnlyType(name: Charlie, value: Value103, count: 103)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"CopyOnlyType(name: Diana, value: Value104, count: 104)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       UnorderedMap<int,MoveAndCopyType> {"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"MoveAndCopyType(name: Eve, value: Value105, count: 105)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"MoveAndCopyType(name: Frank, value: Value106, count: 106)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       UnorderedSet<CopyOnlyType> {"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"CopyOnlyType(name: Nancy, value: Value115, count: 115)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"CopyOnlyType(name: Oscar, value: Value116, count: 116)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       UnorderedSet<MoveAndCopyType> {"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"MoveAndCopyType(name: Paula, value: Value117, count: 117)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"MoveAndCopyType(name: Quinn, value: Value118, count: 118)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ForwardList<MoveOnlyType> [MoveOnlyType(name: Henry, value: Value108, count: 108), MoveOnlyType(name: Grace, value: Value107, count: 107)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ForwardList<CopyOnlyType> [CopyOnlyType(name: Jack, value: Value110, count: 110), CopyOnlyType(name: Iris, value: Value109, count: 109)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ForwardList<MoveAndCopyType> [MoveAndCopyType(name: Leo, value: Value112, count: 112), MoveAndCopyType(name: Kate, value: Value111, count: 111)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Array<MoveOnlyType> [MoveOnlyType(name: Amy, value: Value27, count: 27), MoveOnlyType(name: Ben, value: Value28, count: 28)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Array<CopyOnlyType> [CopyOnlyType(name: Carol, value: Value29, count: 29), CopyOnlyType(name: Dan, value: Value30, count: 30)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Array<MoveAndCopyType> [MoveAndCopyType(name: Emma, value: Value31, count: 31), MoveAndCopyType(name: Frank, value: Value32, count: 32)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Pair<MoveOnlyType> (MoveOnlyType(name: George, value: Value33, count: 33), MoveOnlyType(name: Hannah, value: Value34, count: 34))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Pair<CopyOnlyType> (CopyOnlyType(name: Iris, value: Value35, count: 35), CopyOnlyType(name: Jack, value: Value36, count: 36))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Pair<MoveAndCopyType> (MoveAndCopyType(name: Kate, value: Value37, count: 37), MoveAndCopyType(name: Leo, value: Value38, count: 38))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Pair<MoveAndCopy+MoveOnly> (MoveAndCopyType(name: Yolanda, value: Value52, count: 52), MoveOnlyType(name: Zara, value: Value53, count: 53))"}));

#if !defined(__GNUC__) || __GNUC__ >= 10
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Pair<MoveAndCopy+CopyOnly> (MoveAndCopyType(name: Albert, value: Value54, count: 54), CopyOnlyType(name: Beth, value: Value55, count: 55))"}));
#endif

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Tuple<MoveOnlyType> (MoveOnlyType(name: Mia, value: Value39, count: 39), MoveOnlyType(name: Noah, value: Value40, count: 40))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Tuple<CopyOnlyType> (CopyOnlyType(name: Olivia, value: Value41, count: 41), CopyOnlyType(name: Peter, value: Value42, count: 42))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Tuple<MoveAndCopyType> (MoveAndCopyType(name: Rose, value: Value43, count: 43), MoveAndCopyType(name: Steve, value: Value44, count: 44))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Tuple<MoveAndCopy+MoveOnly> (MoveAndCopyType(name: Ulysses, value: Value48, count: 48), MoveOnlyType(name: Vera, value: Value49, count: 49))"}));

#if !defined(__GNUC__) || __GNUC__ >= 10
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Tuple<MoveAndCopy+CopyOnly> (MoveAndCopyType(name: Walter, value: Value50, count: 50), CopyOnlyType(name: Xena, value: Value51, count: 51))"}));
#endif

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Optional<MoveOnlyType> optional(MoveOnlyType(name: Tom, value: Value45, count: 45))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Optional<CopyOnlyType> optional(CopyOnlyType(name: Uma2, value: Value46, count: 46))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Optional<MoveAndCopyType> optional(MoveAndCopyType(name: Vince, value: Value47, count: 47))"}));

  testing::remove_file(filename);
}