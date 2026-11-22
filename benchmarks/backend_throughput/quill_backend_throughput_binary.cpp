#include <chrono>
#include <cstdint>
#include <iostream>

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

static constexpr size_t total_iterations = 4'000'000;

/** The records of a market data feed, 64 bytes each */
struct AddOrder
{
  uint64_t sequence;
  uint64_t order_id;
  uint64_t price;
  uint32_t quantity;
  char side;
  char padding[35];
};

struct UpdateOrder
{
  uint64_t sequence;
  uint64_t order_id;
  uint64_t price;
  uint32_t quantity;
  char padding[36];
};

struct DeleteOrder
{
  uint64_t sequence;
  uint64_t order_id;
  char padding[48];
};

static_assert(sizeof(AddOrder) == 64, "A record of the feed has 64 bytes");
static_assert(sizeof(UpdateOrder) == 64, "A record of the feed has 64 bytes");
static_assert(sizeof(DeleteOrder) == 64, "A record of the feed has 64 bytes");

/**
 * The backend worker just spins, so we just measure the total time elapsed for total_iterations
 */
int main()
{
  // main thread affinity - pinning can legitimately fail (e.g. Apple Silicon does not support
  // the affinity policy); warn and continue instead of terminating the benchmark
#if defined(QUILL_NO_EXCEPTIONS)
  quill::detail::set_cpu_affinity({1});
#else
  try
  {
    quill::detail::set_cpu_affinity({1});
  }
  catch (std::exception const& e)
  {
    std::cerr << "Failed to set cpu affinity: " << e.what() << std::endl;
  }
#endif

  quill::BackendOptions backend_options;
  backend_options.cpu_affinity = {5};
  backend_options.sleep_duration = std::chrono::nanoseconds{0};
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{1};

  // Start the logging backend thread and give it some tiem to init
  quill::Backend::start(backend_options);

  // Create a file sink to write to a file
  std::shared_ptr<quill::Sink> file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    "quill_backend_total_time.bin",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    quill::FileEventNotifier{});

  quill::Logger* logger = quill::Frontend::create_or_get_logger("bench_logger", std::move(file_sink));

  AddOrder add_order{};
  UpdateOrder update_order{};
  DeleteOrder delete_order{};

  LOG_BINARY(logger, &add_order, sizeof(add_order));
  logger->flush_log(0);

  // start counting the time until backend worker finishes
  auto const start_time = std::chrono::steady_clock::now();
  for (size_t iteration = 0; iteration < total_iterations; ++iteration)
  {
    // Cycle between three records: a feed does not repeat a single one
    switch (iteration % 3)
    {
    case 0:
      add_order.sequence = iteration;
      add_order.order_id = iteration / 3;
      add_order.price = iteration * 2;
      add_order.quantity = static_cast<uint32_t>(iteration);
      add_order.side = 'B';
      LOG_BINARY(logger, &add_order, sizeof(add_order));
      break;
    case 1:
      update_order.sequence = iteration;
      update_order.order_id = iteration / 3;
      update_order.price = iteration * 2;
      update_order.quantity = static_cast<uint32_t>(iteration);
      LOG_BINARY(logger, &update_order, sizeof(update_order));
      break;
    default:
      delete_order.sequence = iteration;
      delete_order.order_id = iteration / 3;
      LOG_BINARY(logger, &delete_order, sizeof(delete_order));
      break;
    }
  }

  // block until all records are flushed
  logger->flush_log(0);

  auto const end_time = std::chrono::steady_clock::now();
  auto const delta = end_time - start_time;
  auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();

  std::cout << fmtquill::format(
                 "Throughput is {:.2f} million records/sec average, total time elapsed: {} ms for "
                 "{} binary records of 64 bytes \n",
                 total_iterations / delta_d / 1e6,
                 std::chrono::duration_cast<std::chrono::milliseconds>(delta).count(), total_iterations)
            << std::endl;
}
