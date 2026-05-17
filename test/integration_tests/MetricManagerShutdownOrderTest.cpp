#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/NullSink.h"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

#if defined(_WIN32)
  #include <windows.h>
#elif defined(__APPLE__)
  #include <mach-o/dyld.h>
#else
  #include <unistd.h>
#endif

using namespace quill;

namespace
{
// Name of the env var used to put the re-executed test binary into "child" mode. The parent
// sets this before spawning itself; the child returns early from the non-matching test case.
char constexpr child_env_var[] = "QUILL_METRIC_MANAGER_SHUTDOWN_ORDER_CHILD";

bool is_child_mode() noexcept
{
  char const* env = std::getenv(child_env_var);
  return env != nullptr && std::string{env} == "1";
}

std::string current_executable_path()
{
#if defined(_WIN32)
  std::string buffer(MAX_PATH, '\0');
  DWORD const copied = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  buffer.resize(copied);
  return buffer;
#elif defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::string buffer(size, '\0');
  _NSGetExecutablePath(buffer.data(), &size);
  if (!buffer.empty() && buffer.back() == '\0')
  {
    buffer.pop_back();
  }
  return buffer;
#else
  std::string buffer(4096, '\0');
  ssize_t const copied = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
  buffer.resize(copied > 0 ? static_cast<size_t>(copied) : 0u);
  return buffer;
#endif
}
} // namespace

TEST_CASE("metric_manager_shutdown_order_child")
{
  if (!is_child_mode())
  {
    return;
  }

  BackendOptions backend_options;
  backend_options.sleep_duration = std::chrono::hours{24};

  Backend::start(backend_options);

  // Give the backend enough time to go idle and enter the long sleep path so the queued
  // metric is drained during the atexit stop handler instead of being processed immediately.
  std::this_thread::sleep_for(std::chrono::milliseconds{10});

  auto sink = Frontend::create_or_get_sink<NullSink>("metric_manager_shutdown_order_child_sink");
  Logger* logger = Frontend::create_or_get_logger("metric_manager_shutdown_order_child_logger", sink);

  MetricMetadata const* metric_metadata = Frontend::create_metric(
    "metric_manager_shutdown_order_child_metric_key", "metric_manager_shutdown_order_child_metric");

  logger->publish_metric(metric_metadata, 1.0);

  // Return from the child test case with the metric still queued. The atexit-installed Backend
  // stop handler must drain the queue; if MetricManager were destroyed before the drain runs,
  // the backend worker would dereference a dangling MetricMetadata pointer and crash.
}

TEST_CASE("metric_manager_shutdown_order")
{
  // Re-exec this test binary with the child env var so the child process drives the scenario
  // above to its atexit drain. The parent process only checks that the child exits cleanly.
  std::string const executable = current_executable_path();
  REQUIRE_FALSE(executable.empty());

#if defined(_WIN32)
  REQUIRE_NE(SetEnvironmentVariableA(child_env_var, "1"), 0);
#else
  REQUIRE_EQ(setenv(child_env_var, "1", 1), 0);
#endif

  std::string const command =
    "\"" + executable + "\" --test-case=metric_manager_shutdown_order_child --no-breaks --success=0";

  int const exit_code = std::system(command.c_str());

#if defined(_WIN32)
  SetEnvironmentVariableA(child_env_var, nullptr);
#else
  unsetenv(child_env_var);
#endif

  REQUIRE_EQ(exit_code, 0);
}
