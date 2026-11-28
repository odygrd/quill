#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/WideString.h"

#include <string>
#include <string_view>
#include <utility>

int main()
{
  quill::BackendOptions backend_options;
  backend_options.check_printable_char = {}; // Preserve UTF-8 output for non-ASCII text.
  quill::Backend::start(backend_options);

  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("wide_strings.log");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));

  std::wstring ws = L"example";
  std::wstring_view wsv{L"string_view"};
  std::wstring unicode_ws = L"\u4E2D\u6587 \u03A9";
  LOG_INFO(logger, "This is a log info example {} {} {}", ws, wsv, unicode_ws);
}
