#include "doctest/doctest.h"

#include "quill/core/LogLevel.h"
#include "quill/core/QuillError.h"
#include "quill/filters/Filter.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

TEST_SUITE_BEGIN("SinkAddFilter");

using namespace quill;

namespace
{
class AlwaysPassFilter : public Filter
{
public:
  explicit AlwaysPassFilter(std::string filter_name) : Filter(std::move(filter_name)) {}

  QUILL_NODISCARD bool filter(MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                              std::string_view, LogLevel, std::string_view, std::string_view) noexcept override
  {
    return true;
  }
};

class ReentrantNameFilter : public AlwaysPassFilter
{
public:
  explicit ReentrantNameFilter(Sink& sink)
    : AlwaysPassFilter("outer_filter"),
      _sink(sink),
      _nested_filter(std::make_unique<AlwaysPassFilter>("nested_filter"))
  {
  }

  QUILL_NODISCARD std::string const& get_filter_name() const noexcept override
  {
    if (_nested_filter)
    {
      _sink.add_filter(std::move(_nested_filter));
    }

    return AlwaysPassFilter::get_filter_name();
  }

private:
  Sink& _sink;
  mutable std::unique_ptr<Filter> _nested_filter;
};
} // namespace

TEST_CASE("add_filter_allows_reentrant_get_filter_name")
{
  // Regression: Sink::add_filter() used to call the virtual get_filter_name() while holding
  // the internal spinlock, so an override that re-entered add_filter() on the same sink would
  // deadlock. The name is now snapshotted before the lock is taken.
  ConsoleSinkConfig config;
  config.set_colour_mode(ConsoleSinkConfig::ColourMode::Never);
  ConsoleSink sink{config};

  sink.add_filter(std::make_unique<AlwaysPassFilter>("existing_filter"));
  sink.add_filter(std::make_unique<ReentrantNameFilter>(sink));

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(sink.add_filter(std::make_unique<AlwaysPassFilter>("nested_filter")), QuillError);
  REQUIRE_THROWS_AS(sink.add_filter(std::make_unique<AlwaysPassFilter>("outer_filter")), QuillError);
#endif
}

TEST_SUITE_END();
