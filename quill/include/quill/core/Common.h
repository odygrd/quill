/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

/**
 * Convert number to string
 */
#define QUILL_AS_STR(x) #x
#define QUILL_STRINGIFY(x) QUILL_AS_STR(x)

namespace quill
{
namespace detail
{
/**
 * Cache line size
 */
static constexpr size_t CACHE_LINE_SIZE{64u};
static constexpr size_t CACHE_LINE_ALIGNED{2 * CACHE_LINE_SIZE};
} // namespace detail

/**
 * Enum to select a queue type
 */
enum class QueueType
{
  UnboundedBlocking,
  UnboundedDropping,
  UnboundedUnlimited,
  BoundedBlocking,
  BoundedDropping
};

/**
 * Enum to select a timezone
 */
enum class Timezone : uint8_t
{
  LocalTime,
  GmtTime
};

/**
 * Enum for the used clock type
 */
enum class ClockSourceType : uint8_t
{
  Tsc,
  System,
  User
};

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#elif defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#elif defined(_WIN32)
  #pragma warning(push)
  #pragma warning(disable : 4265) // Disable warning about non-virtual destructor in MSVC
#endif

/**
 * Tags class that can be used for _WITH_TAGS log macros
 */
class Tags
{
public:
  constexpr Tags() = default;
  virtual void format(std::string&) const = 0;
};

#if defined(__clang__)
  #pragma clang diagnostic pop
#elif defined(__GNUC__)
  #pragma GCC diagnostic pop
#elif defined(_WIN32)
  #pragma warning(pop)
#endif

} // namespace quill
