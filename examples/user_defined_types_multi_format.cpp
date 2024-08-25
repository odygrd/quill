#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include "quill/TriviallyCopyableCodec.h"
#include "quill/bundled/fmt/format.h"

#include <string>

/**
 * The example shows how to define custom formatters to display a `Point` struct in different formats
 */

struct Point
{
  int x, y;
};

template <>
struct fmtquill::formatter<Point>
{
  char presentation = 'p';

  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
  {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'p' || *it == 'c'))
    {
      presentation = *it++;
    }
    if (it != end && *it != '}')
    {
      throw format_error("invalid format");
    }
    return it;
  }

  template <typename FormatContext>
  auto format(const Point& p, FormatContext& ctx) const -> decltype(ctx.out())
  {
    switch (presentation)
    {
    case 'p':
      return fmtquill::format_to(ctx.out(), "({}, {})", p.x, p.y);
    case 'c':
      return fmtquill::format_to(ctx.out(), "x: {}, y: {}", p.x, p.y);
    default:
      throw format_error("invalid format");
    }
  }
};

template <>
struct quill::Codec<Point> : quill::TriviallyCopyableTypeCodec<Point>
{
};

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  Point p{10, 20};
  LOG_INFO(logger, "Default format: {}", p);
  LOG_INFO(logger, "Parentheses format: {:p}", p);
  LOG_INFO(logger, "Coordinate format: {:c}", p);
}