#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <array>
#include <string>
#include <vector>

QUILL_EXPORT void singletons_dll_instances(void const** instances);
QUILL_EXPORT void singletons_dll_start_backend();
QUILL_EXPORT quill::Logger* singletons_dll_create_logger(char const* logger_name,
                                                         char const* filename, quill::Sink** sink);
QUILL_EXPORT void singletons_dll_log(quill::Logger* logger, char const* message);

using namespace quill;

/***/
TEST_CASE("dll_shared_singletons")
{
  // An executable built with QUILL_DLL_IMPORT must use the singletons of the DLL built with
  // QUILL_DLL_EXPORT instead of copies of its own
  std::array<void const*, 7> dll_instances{};
  singletons_dll_instances(dll_instances.data());

  CHECK_EQ(dll_instances[0], static_cast<void const*>(&detail::LoggerManager::instance()));
  CHECK_EQ(dll_instances[1], static_cast<void const*>(&detail::SinkManager::instance()));
  CHECK_EQ(dll_instances[2], static_cast<void const*>(&detail::ThreadContextManager::instance()));
  CHECK_EQ(dll_instances[3], static_cast<void const*>(&detail::MetricManager::instance()));
  CHECK_EQ(dll_instances[4], static_cast<void const*>(&detail::BackendManager::instance()));
  CHECK_EQ(dll_instances[5], static_cast<void const*>(&detail::SignalHandlerContext::instance()));
  CHECK_EQ(dll_instances[6], static_cast<void const*>(&detail::RdtscClock::RdtscTicks::instance()));

  // The DLL runs the backend. A sink created by path in the executable is the sink the DLL gets
  // by name, and the logger the DLL creates is found from the executable
  static constexpr char const* filename = "dll_shared_singletons.log";
  singletons_dll_start_backend();

  FileSinkConfig config;
  config.set_open_mode('w');
  auto file_sink = Frontend::create_or_get_sink<FileSink>(fs::path{filename}, config);

  Sink* dll_sink{nullptr};
  Logger* logger = singletons_dll_create_logger("dll_shared_singletons", filename, &dll_sink);
  REQUIRE_EQ(dll_sink, static_cast<Sink*>(file_sink.get()));
  REQUIRE_EQ(Frontend::get_logger("dll_shared_singletons"), logger);

  LOG_INFO(logger, "Logged from the executable");
  singletons_dll_log(logger, "Logged from the DLL");
  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 2);
  REQUIRE(testing::file_contains(file_contents, "Logged from the executable"));
  REQUIRE(testing::file_contains(file_contents, "Logged from the DLL"));

  testing::remove_file(filename);
}
