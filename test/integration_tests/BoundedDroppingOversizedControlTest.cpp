#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/sinks/NullSink.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

namespace
{
struct BoundedControlOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;
  static constexpr size_t initial_queue_capacity = 4096;
};
} // namespace

TEST_CASE("bounded_dropping_oversized_control")
{
#if defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  using Frontend = quill::FrontendImpl<BoundedControlOptions>;
  auto sink = Frontend::create_sink<quill::NullSink>("bounded_dropping_control_sink");
  auto* logger = Frontend::create_logger("bounded_dropping_control_logger", sink,
                                         quill::PatternFormatterOptions{}, quill::ClockSourceType::System);
  std::string const oversized(8192, 'x');
  CHECK_THROWS_AS(logger->set_mdc("request", oversized), quill::QuillError);
  CHECK_THROWS_AS(logger->erase_mdc(oversized), quill::QuillError);

  static constexpr quill::MacroMetadata metadata{
    "", "", "message", nullptr, quill::LogLevel::Info, quill::MacroMetadata::Event::Log};
  std::atomic<bool> full{false};
  std::atomic<bool> completed{false};
  std::thread producer{[&]
                       {
                         while (logger->template log_statement<false>(&metadata))
                         {
                         }
                         full.store(true, std::memory_order_release);
                         logger->set_mdc("request", "small");
                         completed.store(true, std::memory_order_release);
                       }};

  while (!full.load(std::memory_order_acquire))
  {
    std::this_thread::yield();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds{10});
  CHECK_FALSE(completed.load(std::memory_order_acquire));

  quill::BackendOptions options;
  options.error_notifier = {};
  quill::Backend::start(options);
  producer.join();
  CHECK(completed.load(std::memory_order_acquire));
  quill::Backend::stop();
#endif
}
