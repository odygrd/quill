#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/backend/ThreadUtilities.h"
#include "quill/sinks/ConsoleSink.h"

#include <thread>

extern "C" void log_message()
{
  auto t1 = std::thread(
    []()
    {
      quill::Logger* logger = quill::Frontend::create_or_get_logger(
        "root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

      for (int i = 0; i < 100; ++i)
      {
        LOG_INFO(logger, "{}", i);
      }
    });

  t1.join();
}