#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <utility>

using namespace quill;

/***/
TEST_CASE("console_sink_no_console")
{
#if defined(_WIN32)
  static constexpr char const* child_variable = "QUILL_TEST_CONSOLE_SINK_NO_CONSOLE_CHILD";

  char child_variable_value[2];
  bool const is_child = ::GetEnvironmentVariableA(child_variable, child_variable_value,
                                                  static_cast<DWORD>(sizeof(child_variable_value))) != 0;

  if (is_child)
  {
    // This process has no console and no standard handles, as a GUI application or a service.
    // Its ConsoleSink has nothing to write to, which is not an error
    std::atomic<uint32_t> error_notifications{0};

    BackendOptions backend_options;
    backend_options.error_notifier = [&error_notifications](std::string const&)
    { error_notifications.fetch_add(1); };
    Backend::start(backend_options);

    auto console_sink = Frontend::create_or_get_sink<ConsoleSink>("console_sink_no_console_sink");
    Logger* logger =
      Frontend::create_or_get_logger("console_sink_no_console_logger", std::move(console_sink));

    LOG_INFO(logger, "there is no console to write this to");
    logger->flush_log();

    Frontend::remove_logger(logger);
    Backend::stop();

    REQUIRE_EQ(error_notifications.load(), 0u);
    return;
  }

  // Run this test again in a process without a console and without standard handles
  char executable_path[MAX_PATH];
  DWORD const executable_path_size = ::GetModuleFileNameA(nullptr, executable_path, MAX_PATH);
  REQUIRE_GT(executable_path_size, 0u);
  REQUIRE_LT(executable_path_size, static_cast<DWORD>(MAX_PATH));

  REQUIRE_NE(::SetEnvironmentVariableA(child_variable, "1"), 0);

  STARTUPINFOA startup_info{};
  startup_info.cb = sizeof(startup_info);
  startup_info.dwFlags = STARTF_USESTDHANDLES;

  PROCESS_INFORMATION process_information{};
  std::string command_line = std::string{"\""} + executable_path + "\"";

  BOOL const created = ::CreateProcessA(nullptr, command_line.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS,
                                        nullptr, nullptr, &startup_info, &process_information);

  ::SetEnvironmentVariableA(child_variable, nullptr);
  REQUIRE_NE(created, 0);

  // A process that the C runtime ends for an invalid parameter can wait on a dialog in debug builds
  DWORD exit_code{1};

  if (::WaitForSingleObject(process_information.hProcess, 60'000) == WAIT_OBJECT_0)
  {
    ::GetExitCodeProcess(process_information.hProcess, &exit_code);
  }
  else
  {
    ::TerminateProcess(process_information.hProcess, 1);
  }

  ::CloseHandle(process_information.hThread);
  ::CloseHandle(process_information.hProcess);

  REQUIRE_EQ(exit_code, 0u);
#else
  return;
#endif
}
