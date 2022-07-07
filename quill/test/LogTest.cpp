﻿#include "doctest/doctest.h"

#define QUILL_ACTIVE_LOG_LEVEL QUILL_LOG_LEVEL_TRACE_L3

#include "misc/TestUtilities.h"
#include "quill/detail/LogMacros.h"
#include "quill/detail/LogManager.h"
#include "quill/detail/misc/FileUtilities.h"
#include "quill/filters/FilterBase.h"
#include "quill/handlers/Handler.h"
#include <chrono>
#include <cstdio>
#include <thread>

TEST_SUITE_BEGIN("Log");

/**
 * Contains tests that include frontend and backend thread testing
 * In real logger usage LogManager would be a singleton
 */
using namespace quill;
using namespace quill::detail;

// Important - Because LogManager each time creates a new ThreadContextCollection the following can happen
// 1st Test - Main thread creates static thread local inside ThreadContext
// 2nd Test - Main thread will not recreate the ThreadContext but the new LogManager instance
// is using a new ThreadContextCollection instance resulting in not being able to see the context

// Real logger is using a singleton so that is not an issue
// For the tests we will use threads so that the thread local is also destructed with the LogManager

/***/
TEST_CASE("default_logger_with_filehandler_1")
{
  LogManager lm;
  fs::path const filename{"test_default_logger_with_filehandler_1"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "w", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      Logger* default_logger = lm.logger_collection().get_logger();

      // log using the raw selialization queue
      std::string s = "adipiscing";
      LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur {} {} {} {}", s, "elit", 1, 3.14);
      LOG_ERROR(default_logger,
                "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante {} {}", 2, true);

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 2);
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      root         - Lorem ipsum dolor sit amet, consectetur adipiscing elit 1 3.14"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR     root         - Nulla tempus, libero at dignissim viverra, lectus libero finibus ante 2 true"}));

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("default_logger_with_filehandler_2")
{
  LogManager lm;

  fs::path const filename{"test_default_logger_with_filehandler_2"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "w", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      Logger* default_logger = lm.logger_collection().get_logger();

      // log an array so the log message is pushed to the queue
      LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit {}",
               std::array<int, 2>{1, 2});
      LOG_ERROR(default_logger,
                "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante {}",
                std::array<int, 2>{3, 4});

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 2);
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      root         - Lorem ipsum dolor sit amet, consectetur adipiscing elit [1, 2]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR     root         - Nulla tempus, libero at dignissim viverra, lectus libero finibus ante [3, 4]"}));

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("default_logger_ints_and_large_string")
{
  LogManager lm;

  fs::path const filename{"test_default_logger_ints_and_large_string"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "w", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      Logger* default_logger = lm.logger_collection().get_logger();

      // log an array so the log message is pushed to the queue
      for (int i = 0; i < 2000; ++i)
      {
        std::string v{"Lorem ipsum dolor sit amet, consectetur "};
        v += std::to_string(i);

        LOG_INFO(default_logger, "Logging int: {}, int: {}, string: {}, char: {}", i, i * 10, v, v.c_str());
      }

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 2000);
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      root         - Logging int: 0, int: 0, string: Lorem ipsum dolor sit amet, consectetur 0, char: Lorem ipsum dolor sit amet, consectetur 0"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      root         - Logging int: 1999, int: 19990, string: Lorem ipsum dolor sit amet, consectetur 1999, char: Lorem ipsum dolor sit amet, consectetur 1999"}));

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("default_logger_ints_and_very_large_string")
{
  LogManager lm;

  fs::path const filename{"test_default_logger_ints_and_very_large_string"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "w", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      Logger* default_logger = lm.logger_collection().get_logger();

      // log an array so the log message is pushed to the queue
      for (int i = 0; i < 100; ++i)
      {
        std::string v{
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
          "incididunt ut labore et dolore magna aliqua. Dui accumsan sit amet nulla facilisi morbi "
          "tempus. Diam ut venenatis tellus in metus vulputate eu scelerisque felis. Lorem mollis "
          "aliquam ut porttitor leo a. Posuere urna nec tincidunt praesent semper feugiat nibh "
          "sed. Auctor urna nunc id cursus metus aliquam eleifend mi. Et ultrices neque ornare "
          "aenean euismod elementum nisi quis. Phasellus vestibulum lorem sed risus ultricies "
          "tristique nulla. Porta nibh venenatis cras sed felis eget velit aliquet sagittis. Eget "
          "arcu dictum varius duis at consectetur lorem. Diam quam nulla porttitor massa id neque "
          "aliquam vestibulum morbi. Sed euismod nisi porta lorem mollis aliquam. Arcu felis "
          "bibendum ut tristique. Lorem ipsum dolor sit amet consectetur adipiscing elit "
          "pellentesque habitant. Mauris augue neque gravida in. Dictum fusce ut placerat orci "
          "nulla pellentesque dignissim "};
        v += std::to_string(i);

        LOG_INFO(default_logger, "Logging int: {}, int: {}, string: {}, string: {}", i, i * 10, v, v);
      }

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 100);

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

#if defined(_WIN32)
/***/
TEST_CASE("default_logger_ints_and_wide_string")
{
  LogManager lm;

  fs::path const filename{"default_logger_ints_and_wide_string"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "w", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      Logger* default_logger = lm.logger_collection().get_logger();

      std::wstring s1{L"μιλάμε ελληνικά"};
      LOG_INFO(default_logger, L"καλημέρα, {}", s1);

      wchar_t const* s2{L"Λορεμ ιπσθμ δολορ σιτ αμετ, αδμοδθμ δελενιτι ηενδρεριτ"};
      LOG_INFO(default_logger, L"{}", s2);

      lm.flush();
    });

  frontend.join();

  std::vector<std::wstring> const file_contents = quill::testing::wfile_contents(filename);

  REQUIRE_EQ(file_contents.size(), 2);

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("default_logger_ints_and_very_large_wide_string")
{
  LogManager lm;

  fs::path const filename{"default_logger_ints_and_very_large_wide_string"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "w", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      Logger* default_logger = lm.logger_collection().get_logger();

      // log an array so the log message is pushed to the queue
      for (int i = 0; i < 100; ++i)
      {
        std::wstring v{
          L"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
          "incididunt ut labore et dolore magna aliqua. Dui accumsan sit amet nulla facilisi morbi "
          "tempus. Diam ut venenatis tellus in metus vulputate eu scelerisque felis. Lorem mollis "
          "aliquam ut porttitor leo a. Posuere urna nec tincidunt praesent semper feugiat nibh "
          "sed. Auctor urna nunc id cursus metus aliquam eleifend mi. Et ultrices neque ornare "
          "aenean euismod elementum nisi quis. Phasellus vestibulum lorem sed risus ultricies "
          "tristique nulla. Porta nibh venenatis cras sed felis eget velit aliquet sagittis. Eget "
          "arcu dictum varius duis at consectetur lorem. Diam quam nulla porttitor massa id neque "
          "aliquam vestibulum morbi. Sed euismod nisi porta lorem mollis aliquam. Arcu felis "
          "bibendum ut tristique. Lorem ipsum dolor sit amet consectetur adipiscing elit "
          "pellentesque habitant. Mauris augue neque gravida in. Dictum fusce ut placerat orci "
          "nulla pellentesque dignissim "};
        v += std::to_wstring(i);

        LOG_INFO(default_logger, L"Logging int: {}, int: {}, string: {}, string: {}", i, i * 10, v, v);
      }

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 100);

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}
#endif

void custom_default_logger_same_handler(int test_case = 0)
{
  LogManager lm;
  fs::path const filename{"test_custom_default_logger_same_handler"};

  // Set a file handler the custom logger handler and log to it
  Handler* file_handler =
    lm.handler_collection().create_handler<FileHandler>(filename.string(), "w", FilenameAppend::None);
  file_handler->set_pattern("%(ascii_time) %(logger_name) - %(message) [%(level_id)]");
  lm.set_default_logger_handler(file_handler);

  // Start logging
  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  if (test_case == 0)
  {
    // Add a second logger using the same file handler
    Handler* file_handler_2 =
      lm.handler_collection().create_handler<FileHandler>(filename.string(), "a", FilenameAppend::None);
    QUILL_MAYBE_UNUSED Logger* logger_2 =
      lm.create_logger("custom_logger", file_handler_2, std::nullopt, std::nullopt);
  }
  else if (test_case == 1)
  {
    // Add the other logger by using the default logger params - which is the same as obtaining the file handler above
    QUILL_MAYBE_UNUSED Logger* logger_2 = lm.create_logger("custom_logger", std::nullopt, std::nullopt);
  }
  // Thread for default pattern
  std::thread frontend_default(
    [&lm]()
    {
      Logger* default_logger = lm.logger_collection().get_logger();

      LOG_INFO(default_logger, "Default Lorem ipsum dolor sit amet, consectetur adipiscing elit");
      LOG_ERROR(default_logger,
                "Default Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

      lm.flush();
    });

  // Thread for custom pattern
  std::thread frontend_custom(
    [&lm]()
    {
      Logger* logger_2 = lm.logger_collection().get_logger("custom_logger");

      LOG_INFO(logger_2, "Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit");
      LOG_ERROR(logger_2,
                "Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

      lm.flush();
    });

  frontend_custom.join();
  frontend_default.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 4);

  std::string const first_log_line_default =
    "root - Default Lorem ipsum dolor sit amet, consectetur adipiscing elit";
  std::string const second_log_line_default =
    "root - Default Nulla tempus, libero at dignissim viverra, lectus libero finibus "
    "ante";

  std::string const first_log_line_custom =
    "custom_logger - Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit "
    "[I]";
  std::string const second_log_line_custom =
    "custom_logger - Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus ante "
    "[E]";

  REQUIRE(quill::testing::file_contains(file_contents, first_log_line_default));
  REQUIRE(quill::testing::file_contains(file_contents, second_log_line_default));
  REQUIRE(quill::testing::file_contains(file_contents, first_log_line_custom));
  REQUIRE(quill::testing::file_contains(file_contents, second_log_line_custom));

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("custom_default_logger_same_file") { custom_default_logger_same_handler(0); }

/***/
TEST_CASE("custom_default_logger_same_file_from_default_logger")
{
  custom_default_logger_same_handler(1);
}

void test_custom_default_logger_multiple_handlers(int test_case)
{
  LogManager lm;
  fs::path const filename_1{"test_custom_default_logger_multiple_handlers_1"};
  fs::path const filename_2{"test_custom_default_logger_multiple_handlers_2"};

  // Set a file handler the custom logger handler and log to it

  // First handler
  Handler* file_handler_1 =
    lm.handler_collection().create_handler<FileHandler>(filename_1.string(), "w", FilenameAppend::None);
  file_handler_1->set_pattern("%(ascii_time) %(logger_name) - %(message) [%(level_id)]");

  // Second handler with different pattern
  Handler* file_handler_2 =
    lm.handler_collection().create_handler<FileHandler>(filename_2.string(), "w", FilenameAppend::None);
  file_handler_2->set_pattern("%(ascii_time) %(logger_name) - %(message)", "%D %H:%M:%S.%Qms");

  lm.set_default_logger_handler({file_handler_1, file_handler_2});

  // Start logging
  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  if (test_case == 0)
  {
    // Add a second logger using the same file handler
    Handler* file_handler_a =
      lm.handler_collection().create_handler<FileHandler>(filename_1.string(), "", FilenameAppend::None);
    Handler* file_handler_b =
      lm.handler_collection().create_handler<FileHandler>(filename_2.string(), "", FilenameAppend::None);
    QUILL_MAYBE_UNUSED Logger* logger_2 =
      lm.create_logger("custom_logger", {file_handler_a, file_handler_b}, std::nullopt, std::nullopt);
  }
  else if (test_case == 1)
  {
    // Add the second logger constructing it from the params of the default logger
    QUILL_MAYBE_UNUSED Logger* logger_2 = lm.create_logger("custom_logger", std::nullopt, std::nullopt);
  }

  // Thread for default pattern
  std::thread frontend_default(
    [&lm]()
    {
      Logger* default_logger = lm.logger_collection().get_logger();

      LOG_INFO(default_logger, "Default Lorem ipsum dolor sit amet, consectetur adipiscing elit");
      LOG_ERROR(default_logger,
                "Default Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

      lm.flush();
    });

  // Thread for custom pattern
  std::thread frontend_custom(
    [&lm]()
    {
      Logger* logger_2 = lm.logger_collection().get_logger("custom_logger");

      LOG_INFO(logger_2, "Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit");
      LOG_ERROR(logger_2,
                "Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

      lm.flush();
    });

  frontend_custom.join();
  frontend_default.join();

  {
    // Validate handler 1
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_1);

    REQUIRE_EQ(file_contents.size(), 4);

    std::string const first_log_line_default =
      "root - Default Lorem ipsum dolor sit amet, consectetur adipiscing elit [I]";
    std::string const second_log_line_default =
      "root - Default Nulla tempus, libero at dignissim viverra, lectus libero finibus ante "
      "[E]";

    std::string const first_log_line_custom =
      "custom_logger - Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit "
      "[I]";
    std::string const second_log_line_custom =
      "custom_logger - Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus "
      "ante [E]";

    REQUIRE(quill::testing::file_contains(file_contents, first_log_line_default));
    REQUIRE(quill::testing::file_contains(file_contents, second_log_line_default));
    REQUIRE(quill::testing::file_contains(file_contents, first_log_line_custom));
    REQUIRE(quill::testing::file_contains(file_contents, second_log_line_custom));
  }

  {
    // Validate handler 2
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_2);

    REQUIRE_EQ(file_contents.size(), 4);

    std::string const first_log_line_default =
      "root - Default Lorem ipsum dolor sit amet, consectetur adipiscing elit";
    std::string const second_log_line_default =
      "root - Default Nulla tempus, libero at dignissim viverra, lectus libero finibus ante";

    std::string const first_log_line_custom =
      "custom_logger - Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit";
    std::string const second_log_line_custom =
      "custom_logger - Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus "
      "ante";

    REQUIRE(quill::testing::file_contains(file_contents, first_log_line_default));
    REQUIRE(quill::testing::file_contains(file_contents, second_log_line_default));
    REQUIRE(quill::testing::file_contains(file_contents, first_log_line_custom));
    REQUIRE(quill::testing::file_contains(file_contents, second_log_line_custom));
  }

  lm.stop_backend_worker();
  quill::detail::remove_file(filename_1);
  quill::detail::remove_file(filename_2);
}

/***/
TEST_CASE("custom_default_logger_multiple_handlers")
{
  test_custom_default_logger_multiple_handlers(0);
}

/***/
TEST_CASE("custom_default_logger_multiple_handlers_from_default_logger")
{
  test_custom_default_logger_multiple_handlers(1);
}

/***/
TEST_CASE("many_loggers_multiple_threads")
{
  LogManager lm;
  fs::path const filename{"test_many_loggers_multiple_threads"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "a", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  // Spawn many threads
  std::vector<std::thread> threads;
  static constexpr size_t thread_count = 100;
  static constexpr size_t message_count = 120;

  for (size_t i = 0; i < thread_count; ++i)
  {
    threads.emplace_back(
      [&lm, i]()
      {
        // Create a logger in this thread
        std::string logger_name = "logger_" + std::to_string(i);
        Logger* logger = lm.create_logger(logger_name.data(), std::nullopt, std::nullopt);

        for (size_t j = 0; j < message_count; ++j)
        {
          LOG_INFO(logger, "Hello from thread {} this is message {}", i, j);
        }

        // Let all log get flushed to the file
        lm.flush();
      });
  }

  for (auto& elem : threads)
  {
    elem.join();
  }

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), thread_count * message_count);

  for (size_t i = 0; i < thread_count; ++i)
  {
    // for each thread
    std::string expected_logger_name = "logger_" + std::to_string(i);

    for (size_t j = 0; j < message_count; ++j)
    {
      size_t num = 13 - expected_logger_name.length();
      std::string const white_spaces(num, ' ');
      std::string expected_string = expected_logger_name + white_spaces + "- " +
        "Hello from thread " + std::to_string(i) + " this is message " + std::to_string(j);

      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

#if !defined(QUILL_NO_EXCEPTIONS)
/***/
TEST_CASE("backend_error_handler")
{
  LogManager lm;
  fs::path const filename{"test_backend_error_handler"};

  // counter to check our error handler was invoked
  // atomic because we check this value on this thread, but the backend worker thread updates it
  std::atomic<size_t> error_handler_invoked{0};

  std::thread frontend(
    [&lm, &filename, &error_handler_invoked]()
    {
      // Setting to an invalid CPU. When we call quill::start() our error handler will be invoked and an error will be logged
      lm.config().set_backend_thread_cpu_affinity(static_cast<uint16_t>(321312));

      // Set invalid thread name
      lm.config().set_backend_thread_name(
        "Lorem_ipsum_dolor_sit_amet_consectetur_adipiscing_elit_sed_do_eiusmod_tempor_incididunt_"
        "ut_"
        "labore_et_dolore_magna_aliqua");

      // Set a custom error handler to handler exceptions
      lm.set_backend_worker_error_handler([&error_handler_invoked](std::string const& s)
                                          { ++error_handler_invoked; });

      // Set a file handler as the custom logger handler and log to it
      lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
        filename.string(), "a", FilenameAppend::None));

      // Start backend worker
      lm.start_backend_worker(false, std::initializer_list<int32_t>{});

      Logger* default_logger = lm.logger_collection().get_logger();

      LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
      LOG_ERROR(default_logger,
                "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

      // Let all log get flushed to the file - this waits for the backend to actually start and
      // thy to do something
      lm.flush();
    });

  frontend.join();

  // Check our handler was invoked since either set_backend_thread_name or set_backend_thread_cpu_affinity should have failed
  REQUIRE(error_handler_invoked.load() != 0);

  lm.stop_backend_worker();

  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("backend_error_handler_log_from_backend_thread")
{
  LogManager lm;
  fs::path const filename{"test_backend_error_handler_log_from_backend_thread"};

  std::thread frontend(
    [&lm, &filename]()
    {
      // Setting to an invalid CPU. When we call quill::start() our error handler will be invoked and an error will be logged
      lm.config().set_backend_thread_cpu_affinity(static_cast<uint16_t>(321312));

      // Set invalid thread name
      lm.config().set_backend_thread_name(
        "Lorem_ipsum_dolor_sit_amet_consectetur_adipiscing_elit_sed_do_eiusmod_tempor_incididunt_"
        "ut_"
        "labore_et_dolore_magna_aliqua");

      // Set a file handler as the custom logger handler and log to it
      lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
        filename.string(), "a", FilenameAppend::None));

      Logger* default_logger = lm.logger_collection().get_logger();

      // Set a custom error handler to handler exceptions
      lm.set_backend_worker_error_handler(
        [default_logger, &lm](std::string const& s)
        {
          LOG_WARNING(default_logger, "error handler invoked");
          lm.flush(); // this will be called by the backend but do nothing
        });

      // Start backend worker
      lm.start_backend_worker(false, std::initializer_list<int32_t>{});

      LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
      LOG_ERROR(default_logger,
                "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

      // Let all log get flushed to the file - this waits for the backend to actually start and
      // thy to do something
      lm.flush();
    });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE(quill::testing::file_contains(file_contents,
                                        "LOG_WARNING   root         - error handler invoked"));
  REQUIRE_EQ(file_contents.size(), 3);

  lm.stop_backend_worker();

  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("backend_error_handler_error_throw_while_in_backend_process")
{
  LogManager lm;
  fs::path const filename{"test_backend_error_handler_error_throw_while_in_backend_process"};

  // counter to check our error handler was invoked
  // atomic because we check this value on this thread, but the backend worker thread updates it
  std::atomic<size_t> error_handler_invoked{0};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "a", FilenameAppend::None));

  // Set a custom error handler to handler exceptions
  lm.set_backend_worker_error_handler([&error_handler_invoked](std::string const& s)
                                      { ++error_handler_invoked; });

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      Logger* logger = lm.logger_collection().get_logger();

      // Here we will call LOG_BACKTRACE(...) without calling init_backtrace(...) first
      // We expect an error to be thrown and reported to our error handler backend_worker_error_handler

      LOG_INFO(logger, "Before backtrace.");
      for (uint32_t i = 0; i < 4; ++i)
      {
  #if defined(__aarch64__) || ((__ARM_ARCH >= 6) || defined(_M_ARM64))
        // On ARM we add a small delay because log messages can get the same timestamp from rdtsc
        // when in this loop and make the test unstable
        std::this_thread::sleep_for(std::chrono::microseconds{200});
  #endif
        LOG_BACKTRACE(logger, "Backtrace message {}.", i);
      }
      LOG_ERROR(logger, "After Error.");

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  // Check that the backend worker thread called our error handler 4 times - the number of LOG_BACKTRACE calls
  REQUIRE_EQ(error_handler_invoked.load(), 4);

  quill::detail::remove_file(filename);
}
#endif

/***/
TEST_CASE("log_backtrace_and_flush_on_error_1")
{
  LogManager lm;
  fs::path filename{"test_log_backtrace_and_flush_on_error_1"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "a", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      // Get a logger and enable backtrace
      Logger* logger = lm.logger_collection().get_logger();

      // Enable backtrace for 2 messages
      logger->init_backtrace(2, LogLevel::Error);

      LOG_INFO(logger, "Before backtrace.");
      for (uint32_t i = 0; i < 12; ++i)
      {
#if defined(__aarch64__) || ((__ARM_ARCH >= 6) || defined(_M_ARM64))
        // On ARM we add a small delay because log messages can get the same timestamp from rdtsc
        // when in this loop and make the test unstable
        std::this_thread::sleep_for(std::chrono::microseconds{200});
#endif
        LOG_BACKTRACE(logger, "Backtrace message {}.", i);
      }
      LOG_ERROR(logger, "After Error.");

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 4);
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      root         - Before backtrace."}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"LOG_ERROR     root         - After Error."}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_BACKTRACE root         - Backtrace message 10."}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_BACKTRACE root         - Backtrace message 11."}));

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("log_backtrace_and_flush_on_error_2")
{
  LogManager lm;
  fs::path const filename{"test_log_backtrace_and_flush_on_error_2"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "a", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      // Get a logger and enable backtrace
      Logger* logger = lm.logger_collection().get_logger();

      // Enable backtrace for 2 messages
      logger->init_backtrace(2, LogLevel::Error);

      // here we log error with an array so that the error message is pushed into the queue
      LOG_INFO(logger, "Before backtrace {}", std::array<int, 2>{0, 1});

      for (uint32_t i = 0; i < 12; ++i)
      {
#if defined(__aarch64__) || ((__ARM_ARCH >= 6) || defined(_M_ARM64))
        // On ARM we add a small delay because log messages can get the same timestamp from rdtsc
        // when in this loop and make the test unstable
        std::this_thread::sleep_for(std::chrono::microseconds{200});
#endif
        LOG_BACKTRACE(logger, "Backtrace message {}.", i);
      }

      // here we log error with an array so that the error message is pushed into the queue
      LOG_ERROR(logger, "After Error {}", std::array<int, 2>{0, 1});

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 4);
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      root         - Before backtrace"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"LOG_ERROR     root         - After Error"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_BACKTRACE root         - Backtrace message 10."}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_BACKTRACE root         - Backtrace message 11."}));

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("log_backtrace_terminate_thread_then_and_flush_on_error")
{
  // In this test we store in backtrace from one thread that terminates
  // and then we log that backtrace from a different thread
  LogManager lm;
  fs::path const filename{"test_log_backtrace_terminate_thread_then_and_flush_on_error"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "a", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      // Get a logger and enable backtrace
      Logger* logger = lm.logger_collection().get_logger();

      // Enable backtrace for 2 messages
      logger->init_backtrace(2, LogLevel::Error);

      for (uint32_t i = 0; i < 12; ++i)
      {
#if defined(__aarch64__) || ((__ARM_ARCH >= 6) || defined(_M_ARM64))
        // On ARM we add a small delay because log messages can get the same timestamp from rdtsc
        // when in this loop and make the test unstable
        std::this_thread::sleep_for(std::chrono::microseconds{200});
#endif
        LOG_BACKTRACE(logger, "Backtrace message {}.", i);
      }
    });

  frontend.join();

  // The first thread logged something in backtrace and finished.
  // Now we spawn a different thread and LOG_ERROR and we expect to see the backtrace from the previous thread
  std::thread frontend_1(
    [&lm]()
    {
      // Get the same logger
      Logger* logger = lm.logger_collection().get_logger();
      LOG_ERROR(logger, "After Error.");

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend_1.join();

  // Now check file
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 3);
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"LOG_ERROR     root         - After Error."}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_BACKTRACE root         - Backtrace message 10."}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_BACKTRACE root         - Backtrace message 11."}));

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("log_backtrace_manual_flush")
{
  LogManager lm;
  fs::path const filename{"test_log_backtrace_manual_flush"};

  // Set a file handler as the custom logger handler and log to it
  lm.set_default_logger_handler(lm.handler_collection().create_handler<FileHandler>(
    filename.string(), "a", FilenameAppend::None));

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  std::thread frontend(
    [&lm]()
    {
      // Get a logger and enable backtrace
      Logger* logger = lm.logger_collection().get_logger();

      // Enable backtrace for 2 messages but without flushing
      logger->init_backtrace(2);

      LOG_INFO(logger, "Before backtrace.");
      for (uint32_t i = 0; i < 12; ++i)
      {
#if defined(__aarch64__) || ((__ARM_ARCH >= 6) || defined(_M_ARM64))
        // On ARM we add a small delay because log messages can get the same timestamp from rdtsc
        // when in this loop and make the test unstable
        std::this_thread::sleep_for(std::chrono::microseconds{200});
#endif
        LOG_BACKTRACE(logger, "Backtrace message {}.", i);
      }
      LOG_ERROR(logger, "After Error.");

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 2);
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      root         - Before backtrace."}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"LOG_ERROR     root         - After Error."}));

  // The backtrace didn't flush we will force flush it now from a new thread
  std::thread frontend_1(
    [&lm]()
    {
      // Get a logger and enable backtrace
      Logger* logger = lm.logger_collection().get_logger();

      logger->flush_backtrace();

      lm.flush();
    });

  frontend_1.join();

  std::vector<std::string> const file_contents_2 = quill::testing::file_contents(filename);

  // Now we also have the backtrace
  REQUIRE_EQ(file_contents_2.size(), 4);
  REQUIRE(quill::testing::file_contains(
    file_contents_2, std::string{"LOG_INFO      root         - Before backtrace."}));
  REQUIRE(quill::testing::file_contains(file_contents_2, std::string{"LOG_ERROR     root         - After Error."}));
  REQUIRE(quill::testing::file_contains(
    file_contents_2, std::string{"LOG_BACKTRACE root         - Backtrace message 10."}));
  REQUIRE(quill::testing::file_contains(
    file_contents_2, std::string{"LOG_BACKTRACE root         - Backtrace message 11."}));

  lm.stop_backend_worker();
  quill::detail::remove_file(filename);
}

/**
 * Filter class for our file handler
 */
class FileFilter1 : public quill::FilterBase
{
public:
  FileFilter1() : quill::FilterBase("FileFilter1"){};

  QUILL_NODISCARD bool filter(char const* thread_id, std::chrono::nanoseconds log_message_timestamp,
                              quill::MacroMetadata const& metadata,
                              fmt::memory_buffer const& formatted_record) noexcept override
  {
    if (metadata.level() < quill::LogLevel::Warning)
    {
      return true;
    }
    return false;
  }
};

/**
 * Filter for the stdout handler
 */
class FileFilter2 : public quill::FilterBase
{
public:
  FileFilter2() : quill::FilterBase("FileFilter2"){};

  QUILL_NODISCARD bool filter(char const* thread_id, std::chrono::nanoseconds log_message_timestamp,
                              quill::MacroMetadata const& metadata,
                              fmt::memory_buffer const& formatted_record) noexcept override
  {
    if (metadata.level() >= quill::LogLevel::Warning)
    {
      return true;
    }
    return false;
  }
};

/***/
TEST_CASE("logger_with_two_files_filters")
{
  LogManager lm;
  fs::path const filename1{"logger_with_two_files_filters1"};
  fs::path const filename2{"logger_with_two_files_filters2"};

  // Set file 1
  quill::Handler* file_handler1 =
    lm.handler_collection().create_handler<FileHandler>(filename1.string(), "w", FilenameAppend::None);

  // Create and add the filter to our handler
  file_handler1->add_filter(std::make_unique<FileFilter1>());

  // Set file 2
  quill::Handler* file_handler2 =
    lm.handler_collection().create_handler<FileHandler>(filename2.string(), "w", FilenameAppend::None);

  // Create and add the filter to our handler
  file_handler2->add_filter(std::make_unique<FileFilter2>());

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  Logger* default_logger =
    lm.create_logger("logger", {file_handler1, file_handler2}, std::nullopt, std::nullopt);

  std::thread frontend(
    [&lm, default_logger]()
    {
      LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
      LOG_ERROR(default_logger,
                "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  // File 1 only log info
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename1);
  REQUIRE_EQ(file_contents.size(), 1);
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      logger       - Lorem ipsum dolor sit amet, consectetur adipiscing elit"}));

  // file 2 only log error
  std::vector<std::string> const file_contents2 = quill::testing::file_contents(filename2);
  REQUIRE_EQ(file_contents2.size(), 1);
  REQUIRE(quill::testing::file_contains(
    file_contents2, std::string{"LOG_ERROR     logger       - Nulla tempus, libero at dignissim viverra, lectus libero finibus ante"}));

  lm.stop_backend_worker();

  quill::detail::remove_file(filename1);
  quill::detail::remove_file(filename2);
}

/***/
TEST_CASE("logger_with_two_files_set_log_level_on_handler")
{
  LogManager lm;
  fs::path const filename1{"logger_with_two_files_set_log_level_on_handler1"};
  fs::path const filename2{"logger_with_two_files_set_log_level_on_handler2"};

  // Set file 1
  quill::Handler* file_handler1 =
    lm.handler_collection().create_handler<FileHandler>(filename1.string(), "w", FilenameAppend::None);

  // Set file 2
  quill::Handler* file_handler2 =
    lm.handler_collection().create_handler<FileHandler>(filename2.string(), "w", FilenameAppend::None);

  lm.start_backend_worker(false, std::initializer_list<int32_t>{});

  Logger* default_logger =
    lm.create_logger("logger", {file_handler1, file_handler2}, std::nullopt, std::nullopt);

  // Check log levels on the handlers
  REQUIRE_EQ(file_handler1->get_log_level(), LogLevel::TraceL3);
  REQUIRE_EQ(file_handler2->get_log_level(), LogLevel::TraceL3);

  // Set filters to our handlers
  file_handler1->set_log_level(LogLevel::Info);
  file_handler2->set_log_level(LogLevel::Error);

  std::thread frontend(
    [&lm, default_logger]()
    {
      LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
      LOG_ERROR(default_logger,
                "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

      // Let all log get flushed to the file
      lm.flush();
    });

  frontend.join();

  // Check log levels on the handlers
  REQUIRE_EQ(file_handler1->get_log_level(), LogLevel::Info);
  REQUIRE_EQ(file_handler2->get_log_level(), LogLevel::Error);

  // File 1 has everything
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename1);
  REQUIRE_EQ(file_contents.size(), 2);
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      logger       - Lorem ipsum dolor sit amet, consectetur adipiscing elit"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR     logger       - Nulla tempus, libero at dignissim viverra, lectus libero finibus ante"}));

  // file 2 only log error
  std::vector<std::string> const file_contents2 = quill::testing::file_contents(filename2);
  REQUIRE_EQ(file_contents2.size(), 1);
  REQUIRE(quill::testing::file_contains(
    file_contents2, std::string{"LOG_ERROR     logger       - Nulla tempus, libero at dignissim viverra, lectus libero finibus ante"}));

  lm.stop_backend_worker();

  quill::detail::remove_file(filename1);
  quill::detail::remove_file(filename2);
}
TEST_SUITE_END();