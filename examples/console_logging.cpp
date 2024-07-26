#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include "quill/bundled/fmt/ostream.h"
#include "quill/std/Array.h"
#include "quill/std/Chrono.h"

#include <iostream>
#include <string>
#include <utility>

struct TCStruct
{
  int a;
  double b;
  char c[12];

  friend std::ostream& operator<<(std::ostream& os, TCStruct const& arg)
  {
    os << "a: " << arg.a << ", b: " << arg.b << ", c: " << arg.c;
    return os;
  }
};

template <>
struct fmtquill::formatter<TCStruct> : fmtquill::ostream_formatter
{
};

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  LOG_INFO(logger, "time is {}", now);
}
