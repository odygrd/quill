#include "quill_wrapper_static.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

void setup_quill()
{
  // Start the backend thread
  quill::Backend::start();

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  // Create and store the logger
  quill::Frontend::create_or_get_logger(
    "root", std::move(console_sink),
    quill::PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                                   "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                   "%H:%M:%S.%Qns", quill::Timezone::GmtTime});
}

quill::Logger* get_logger(std::string const& name)
{
  return quill::Frontend::get_logger(name);
}