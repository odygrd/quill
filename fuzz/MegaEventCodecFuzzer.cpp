#define FUZZER_LOG_FILENAME "mega_event_codec_fuzz.log"
#define FUZZER_IMMEDIATE_FLUSH_LIMIT 128
#include "FuzzerHelper.h"

#include "FuzzDataExtractor.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/DirectFormatCodec.h"
#include "quill/LogMacros.h"
#include "quill/core/Filesystem.h"

#include "quill/std/Array.h"
#include "quill/std/FilesystemPath.h"
#include "quill/std/Optional.h"
#include "quill/std/Pair.h"
#include "quill/std/Tuple.h"
#include "quill/std/Vector.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

struct DeferredPayload
{
  uint64_t order_id{};
  double risk{};
};

struct DirectPayload
{
  int64_t sequence{};
  std::string name;
};

enum class EventState : uint8_t
{
  Open = 0,
  Partial = 1,
  Filled = 2,
  Cancelled = 3
};

namespace fmtquill
{
template <>
struct formatter<DeferredPayload>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(DeferredPayload const& value, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "DeferredPayload{{order_id:{}, risk:{}}}", value.order_id,
                               value.risk);
  }
};

template <>
struct formatter<DirectPayload>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(DirectPayload const& value, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "DirectPayload{{sequence:{}, name:{}}}", value.sequence,
                               value.name);
  }
};

template <>
struct formatter<EventState> : formatter<std::string_view>
{
  auto format(EventState value, format_context& ctx) const
  {
    std::string_view text = "open";
    switch (value)
    {
    case EventState::Open:
      text = "open";
      break;
    case EventState::Partial:
      text = "partial";
      break;
    case EventState::Filled:
      text = "filled";
      break;
    case EventState::Cancelled:
      text = "cancelled";
      break;
    }

    return formatter<std::string_view>::format(text, ctx);
  }
};
} // namespace fmtquill

namespace quill
{
template <>
struct Codec<DeferredPayload> : DeferredFormatCodec<DeferredPayload>
{
};

template <>
struct Codec<DirectPayload> : DirectFormatCodec<DirectPayload>
{
};
} // namespace quill

namespace
{
std::vector<int32_t> make_ints(FuzzDataExtractor& extractor)
{
  std::vector<int32_t> values;
  values.reserve(extractor.get_byte() % 8u);
  while (values.size() < values.capacity())
  {
    values.push_back(extractor.get_int32());
  }
  return values;
}

std::vector<std::string> make_tags(FuzzDataExtractor& extractor)
{
  std::vector<std::string> tags;
  tags.reserve(extractor.get_byte() % 5u);
  while (tags.size() < tags.capacity())
  {
    tags.emplace_back(extractor.get_string(32));
  }
  return tags;
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 8 || !g_logger)
  {
    return 0;
  }

  FuzzDataExtractor extractor{data, size};

  std::string symbol = extractor.get_string(48);
  std::string venue = extractor.get_string(24);
  std::string note = extractor.get_bytes(static_cast<size_t>(extractor.get_byte() % 160u));
  std::string_view comment = extractor.get_string_view();
  std::string c_text_storage = extractor.get_string(80);
  char const* c_text = c_text_storage.c_str();

  std::array<uint16_t, 4> levels{extractor.get_uint16(), extractor.get_uint16(),
                                 extractor.get_uint16(), extractor.get_uint16()};

  std::vector<int32_t> legs = make_ints(extractor);
  std::vector<std::string> tags = make_tags(extractor);
  std::optional<std::string> optional_note =
    extractor.get_bool() ? std::optional<std::string>{extractor.get_string(40)} : std::nullopt;
  std::pair<uint32_t, std::string> route{extractor.get_uint32(), extractor.get_string(24)};
  std::tuple<int32_t, double, std::string> stats{extractor.get_int32(), extractor.get_double(),
                                                 extractor.get_string(24)};
  quill::fs::path path = quill::fs::path{"fuzz"} / extractor.get_string(20);
  DeferredPayload deferred{extractor.get_uint64(), extractor.get_double()};
  DirectPayload direct{extractor.get_int64(), extractor.get_string(24)};
  EventState state = static_cast<EventState>(extractor.get_byte() % 4u);
  uint64_t order_id = extractor.get_uint64();
  int64_t sequence = extractor.get_int64();
  double price = extractor.get_double();
  float ratio = extractor.get_float();
  bool active = extractor.get_bool();

  switch (extractor.get_byte() % 5u)
  {
  case 0:
    LOG_INFO(g_logger,
             "mega order_id={} sequence={} symbol={} venue={} note={} comment={} c_text={} "
             "levels={} legs={} tags={} optional={} route={} stats={} path={} deferred={} "
             "direct={} state={} price={} ratio={} active={}",
             order_id, sequence, symbol, venue, note, comment, c_text, levels, legs, tags,
             optional_note, route, stats, path, deferred, direct, state, price, ratio, active);
    break;
  case 1:
    LOG_INFO(g_logger,
             "mega_named {order_id} {sequence} {symbol} {venue} {note} {price} {ratio} {active} "
             "{route} {state} {path}",
             order_id, sequence, symbol, venue, note, price, ratio, active, route, state, path);
    break;
  case 2:
    LOGV_INFO(g_logger, "mega_logv", order_id, sequence, symbol, note, levels, legs, tags, direct,
              deferred, price, ratio);
    break;
  case 3:
    LOG_DYNAMIC(g_logger, static_cast<quill::LogLevel>(extractor.get_byte() % 9u),
                "mega_dynamic {} {} {} {} {} {}", symbol, venue, direct, deferred, optional_note, tags);
    break;
  case 4:
    LOG_INFO(g_logger, "mega_text {} {} {}", c_text, comment, note);
    LOG_INFO(g_logger, "mega_containers {} {} {}", legs, levels, stats);
    LOG_INFO(g_logger, "mega_objects {} {} {} {}", route, path, state, optional_note);
    break;
  }

  return 0;
}
