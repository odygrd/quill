#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/sinks/Sink.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

class PeriodicTaskSink final : public Sink
{
public:
  explicit PeriodicTaskSink(bool should_throw) : _should_throw(should_throw) {}

  void write_log(MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                 std::string const&, std::string_view, LogLevel, std::string_view, std::string_view,
                 std::vector<std::pair<std::string, std::string>> const*, std::string_view, std::string_view) override
  {
  }

  void flush_sink() override {}

  void run_periodic_tasks() override
  {
    ++_periodic_task_calls;

#if !defined(QUILL_NO_EXCEPTIONS)
    if (_should_throw)
    {
      throw std::runtime_error{"periodic task failure"};
    }
#endif
  }

  size_t periodic_task_calls() const noexcept { return _periodic_task_calls; }

private:
  bool _should_throw;
  size_t _periodic_task_calls{0};
};

TEST_CASE("periodic_sink_exception_is_notified_and_isolated")
{
#if defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  static std::string const throwing_sink_name = "periodic_exception_throwing_sink";
  static std::string const following_sink_name = "periodic_exception_following_sink";
  static std::string const logger_name = "periodic_exception_logger";

  auto throwing_sink = Frontend::create_or_get_sink<PeriodicTaskSink>(throwing_sink_name, true);
  auto following_sink = Frontend::create_or_get_sink<PeriodicTaskSink>(following_sink_name, false);
  Logger* logger = Frontend::create_or_get_logger(logger_name, {throwing_sink, following_sink});

  bool error_notifier_called{false};
  BackendOptions options;
  options.error_notifier = [&error_notifier_called](std::string const& error_message)
  {
    if (error_message == "periodic task failure")
    {
      error_notifier_called = true;
    }
  };

  ManualBackendWorker* worker = Backend::acquire_manual_backend_worker();
  worker->init(options);
  worker->poll_one();
  worker->shutdown();
  Frontend::remove_logger(logger);

  REQUIRE(error_notifier_called);
  REQUIRE_EQ(static_cast<PeriodicTaskSink*>(throwing_sink.get())->periodic_task_calls(), 1);
  REQUIRE_EQ(static_cast<PeriodicTaskSink*>(following_sink.get())->periodic_task_calls(), 1);
#endif
}
