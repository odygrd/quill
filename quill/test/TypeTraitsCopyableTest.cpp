#include "doctest/doctest.h"

#include "quill/detail/misc/TypeTraitsCopyable.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

TEST_SUITE_BEGIN("TypeTraitsCopyable");

using namespace quill::detail;

struct TriviallyCopyableButNotTrivial
{
  explicit TriviallyCopyableButNotTrivial(TriviallyCopyableButNotTrivial const&) = default;
  explicit TriviallyCopyableButNotTrivial(int x) : m(x + 1) {}
  int m;
};

struct TaggedNonTrivial
{
public:
  using copy_loggable = std::true_type;

  explicit TaggedNonTrivial(std::string x) : x(std::move(x)){};

private:
  std::string x;
};

struct NonTrivial
{
public:
  explicit NonTrivial(std::string x) : x(std::move(x)){};

private:
  std::string x;
};

enum Enum
{
  One,
  Two
};

enum class EnumClass
{
  Three,
  Four
};

TEST_CASE("is_string")
{
  static_assert(is_string_v<std::string>, "_");
  static_assert(!is_string_v<int>, "_");
  static_assert(!is_string_v<std::vector<int>>, "_");
}

TEST_CASE("is_tagged_copyable")
{
  static_assert(!is_tagged_copyable_v<std::string>, "_");
  static_assert(!is_tagged_copyable_v<int>, "_");
  static_assert(!is_tagged_copyable_v<std::vector<int>>, "_");
  static_assert(is_tagged_copyable_v<TaggedNonTrivial>, "_");
}

TEST_CASE("is_copyable_pair")
{
  static_assert(is_copyable_pair_v<std::pair<std::string, std::string>>, "_");
  static_assert(is_copyable_pair_v<std::pair<int, int>>, "_");
  static_assert(is_copyable_pair_v<std::pair<std::string, const int>>, "_");
  static_assert(is_copyable_pair_v<std::pair<std::string, std::string>>, "_");
  static_assert(
    is_copyable_pair_v<std::pair<std::pair<std::string, std::string>, std::pair<int, int>>>, "_");
}

TEST_CASE("is_copyable_optional")
{
  static_assert(is_copyable_optional_v<std::optional<int>>, "_");
  static_assert(is_copyable_optional_v<std::optional<bool>>, "_");
  static_assert(is_copyable_optional_v<std::optional<const int>>, "_");
  static_assert(is_copyable_optional_v<std::optional<const std::string>>, "_");
  static_assert(is_copyable_optional_v<std::optional<std::string>>, "_");
  static_assert(is_copyable_optional_v<std::optional<std::pair<std::string, int>>>, "_");
  static_assert(is_copyable_optional_v<std::optional<std::optional<std::pair<std::string, int>>>>,
                "_");
}

TEST_CASE("is_container")
{
  static_assert(is_container_v<std::vector<int>>, "_");
  static_assert(is_container_v<std::vector<std::string>>, "_");
  static_assert(is_container_v<std::map<std::string, int>>, "_");
}

TEST_CASE("is_copyable_container")
{
  static_assert(is_copyable_container_v<std::vector<int>>, "_");
  static_assert(is_copyable_container_v<std::vector<std::string>>, "_");
  static_assert(is_copyable_container_v<std::vector<std::vector<std::string>>>, "_");
  static_assert(is_copyable_container_v<std::vector<std::map<std::string, int>>>, "_");
  static_assert(is_copyable_container_v<std::vector<std::pair<std::string, int>>>, "_");
  static_assert(!is_copyable_container_v<std::vector<NonTrivial>>, "_");
}

TEST_CASE("is_copyable")
{
  // built in - copyable
  static_assert(is_copyable_v<int>, "_");
  static_assert(is_copyable_v<int>, "_");
  static_assert(is_copyable_v<Enum>, "_");
  static_assert(is_copyable_v<EnumClass>, "_");
  static_assert(is_copyable_v<std::string>, "_");
  static_assert(is_copyable_v<void*>, "_");
  static_assert(is_copyable_v<TaggedNonTrivial>, "_");
  static_assert(is_copyable_v<TriviallyCopyableButNotTrivial>, "_");

  // built in - not copyable
  static_assert(!is_copyable_v<NonTrivial>, "_");

  // std chrono duration is copyable
  static_assert(is_copyable_v<std::chrono::nanoseconds>, "_");
  static_assert(is_copyable_v<std::chrono::minutes>, "_");
  static_assert(is_copyable_v<std::chrono::hours>, "_");
  static_assert(is_copyable_v<std::time_t>, "_");

  // pairs - copyable
  static_assert(is_copyable_v<std::pair<std::string, std::string>>, "_");
  static_assert(is_copyable_v<std::pair<std::string, std::string>>, "_");
  static_assert(is_copyable_v<std::pair<float, std::string>>, "_");
  static_assert(is_copyable_v<std::pair<TaggedNonTrivial, std::string>>, "_");
  static_assert(is_copyable_v<std::pair<TaggedNonTrivial, TaggedNonTrivial>>, "_");
  static_assert(is_copyable_v<std::pair<TaggedNonTrivial, TriviallyCopyableButNotTrivial>>, "_");
  
  // pairs - not copyable
  static_assert(!is_copyable_v<std::pair<NonTrivial, std::string>>, "_");

  // arrays
  static_assert(is_copyable_v<std::array<int, 10>>, "_");
  static_assert(is_copyable_v<std::array<TaggedNonTrivial, 10>>, "_");

  // arrays - non copyable
  static_assert(!is_copyable_v<std::array<NonTrivial, 10>>, "_");

  // vectors - copyable
  static_assert(is_copyable_v<std::vector<int>>, "_");
  static_assert(is_copyable_v<std::vector<std::string>>, "_");
  static_assert(is_copyable_v<std::vector<std::vector<std::string>>>, "_");
  static_assert(is_copyable_v<std::vector<std::vector<std::vector<int>>>>, "_");
  static_assert(is_copyable_v<std::vector<std::map<std::string, int>>>, "_");
  static_assert(is_copyable_v<std::vector<std::pair<std::string, int>>>, "_");

  // vectors - not copyable
  static_assert(!is_copyable_v<std::vector<NonTrivial>>, "_");
  static_assert(!is_copyable_v<std::vector<std::vector<std::vector<NonTrivial>>>>, "_");
  static_assert(!is_copyable_v<std::vector<std::map<std::string, NonTrivial>>>, "_");
  static_assert(!is_copyable_v<std::vector<std::map<NonTrivial, std::string>>>, "_");

  // Non copyables - not copyable
  static_assert(!is_copyable_v<std::map<std::string, NonTrivial>>, "_");

  // tuples
  static_assert(is_copyable_v<std::tuple<int, bool, std::string>>, "-");
  static_assert(is_copyable_v<std::tuple<int, std::chrono::hours, std::string>>, "-");
  static_assert(is_copyable_v<std::tuple<int, TaggedNonTrivial, std::string>>, "-");
  static_assert(is_copyable_v<std::tuple<std::pair<std::string, std::string>, bool, std::string>>,
                "-");

  // tuples - not copyable
  static_assert(!is_copyable_v<std::tuple<std::pair<NonTrivial, std::string>, bool, std::string>>,
                "-");
  static_assert(!is_copyable_v<std::tuple<int, NonTrivial, std::string>>, "-");

  // reference wrapper
  static_assert(!is_copyable_v<std::reference_wrapper<int>>, "-");
  static_assert(!is_copyable_v<std::vector<std::reference_wrapper<int>>>, "-");

  // optional
  static_assert(is_copyable_v<std::optional<bool>>, "_");
  static_assert(is_copyable_v<std::optional<const int>>, "_");
  static_assert(is_copyable_v<std::optional<const std::string>>, "_");
  static_assert(is_copyable_v<std::optional<std::string>>, "_");
  static_assert(is_copyable_v<std::optional<std::pair<std::string, int>>>, "_");

  // tuples - not copyable
  static_assert(!is_copyable_v<std::optional<NonTrivial>>, "_");
}

TEST_CASE("are_copyable")
{
  // built in - copyable
  static_assert(are_copyable_v<int, Enum, std::string, TaggedNonTrivial>, "_");
  static_assert(!are_copyable_v<int, Enum, std::string, NonTrivial>, "_");
}

TEST_SUITE_END();