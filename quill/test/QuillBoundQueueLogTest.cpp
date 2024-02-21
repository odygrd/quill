#include <iostream>
#define QUILL_USE_BOUNDED_QUEUE

#include "doctest/doctest.h"

#include "quill/Quill.h"
#include "quill/detail/LogManager.h"

TEST_SUITE_BEGIN("QuillBoundQueueLog");

using namespace quill;
using namespace quill::detail;

TEST_CASE("quill_full_bound_queue_safe_flush")
{
  fs::path const filename{"test_quill_full_bound_queue_safe_flush"};

  LogManager lm;

  quill::Config cfg;

  cfg.default_handlers.emplace_back(lm.handler_collection().create_handler<FileHandler>(
    filename.string(),
    []()
    {
      quill::FileHandlerConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{}));

  lm.configure(cfg);

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  Logger* default_logger = lm.logger_collection().get_logger();

  for (int i = 0; i < 5000; ++i)
  {
    LOG_DEBUG(default_logger, "Log something to fulfill the bound queue.");
    LOG_INFO(default_logger, "Log something to fulfill the bound queue.");
    LOG_WARNING(default_logger, "Log something to fulfill the bound queue.");
    LOG_ERROR(default_logger, "Log something to fulfill the bound queue.");
  }

  // Test is passed if not core dump.
  lm.flush();

  lm.stop_backend_worker();
}

TEST_SUITE_END();
