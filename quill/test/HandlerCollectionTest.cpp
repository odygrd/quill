#include "doctest/doctest.h"

#include "quill/detail/HandlerCollection.h"
#include <cstdio>

TEST_SUITE_BEGIN("HandlerCollection");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("stdout_stderr_handlers")
{
  HandlerCollection hc;

  // Get the default stdout stream handler
  std::shared_ptr<Handler> stdout_handler = hc.stdout_console_handler();

  REQUIRE_EQ(reinterpret_cast<StreamHandler*>(stdout_handler.get())->filename(), std::string{"stdout"});

  // Attempt to create a file handler with stdout as name and check that it is the same as the default
  std::shared_ptr<Handler> filehandler_1 = hc.create_handler<FileHandler>(
    "stdout",
    []()
    {
      quill::FileHandlerConfig cfg;
      cfg.set_open_mode('a');
      return cfg;
    }(),
    FileEventNotifier{});
  REQUIRE_EQ(filehandler_1, stdout_handler);

  // Get the default stderr stream handler
  std::shared_ptr<Handler> stderr_handler = hc.stderr_console_handler();
  REQUIRE_EQ(reinterpret_cast<StreamHandler*>(stderr_handler.get())->filename(), std::string{"stderr"});

  // Attempt to create a file handler with stderr as name and check that it is the same as the default
  std::shared_ptr<Handler> filehandler_2 = hc.create_handler<FileHandler>(
    "stderr",
    []()
    {
      quill::FileHandlerConfig cfg;
      cfg.set_open_mode('a');
      return cfg;
    }(),
    FileEventNotifier{});
  REQUIRE_EQ(filehandler_2, stderr_handler);
}

/***/
TEST_CASE("create_get")
{
  HandlerCollection hc;

  // Create a file handler
  std::shared_ptr<Handler> filehandler = hc.create_handler<FileHandler>(
    "create_get_file_handler",
    []()
    {
      quill::FileHandlerConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  // Request the same file handler
  std::shared_ptr<Handler> filehandler_2 = hc.create_handler<FileHandler>(
    "create_get_file_handler",
    []()
    {
      quill::FileHandlerConfig cfg;
      cfg.set_open_mode('a');
      return cfg;
    }(),
    FileEventNotifier{});

  // Compare the pointers
  REQUIRE_EQ(filehandler, filehandler_2);
  std::remove("create_get_file_handler");
}

/***/
TEST_CASE("subscribe_get_active_same_handler")
{
  HandlerCollection hc;
  // Create a file handler
  std::shared_ptr<Handler> filehandler = hc.create_handler<FileHandler>(
    "create_get_file_handler",
    []()
    {
      quill::FileHandlerConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  // Request the same file handler
  std::shared_ptr<Handler> filehandler_2 = hc.create_handler<FileHandler>(
    "create_get_file_handler",
    []()
    {
      quill::FileHandlerConfig cfg;
      cfg.set_open_mode('a');
      return cfg;
    }(),
    FileEventNotifier{});

  // Compare the pointers
  REQUIRE_EQ(filehandler, filehandler_2);

  // Check for active file handlers
  auto active_handlers = hc.active_handlers();
  REQUIRE_EQ(active_handlers.size(), 0);

  // Subscribe the handler once
  hc.subscribe_handler(filehandler);
  active_handlers = hc.active_handlers();
  REQUIRE_EQ(active_handlers.size(), 1);

  // Subscribe the new handler - no effect as it is the same as before
  hc.subscribe_handler(filehandler_2);
  active_handlers = hc.active_handlers();
  REQUIRE_EQ(active_handlers.size(), 1);

  // Subscribe the same handler again - check no effect
  hc.subscribe_handler(filehandler);
  active_handlers = hc.active_handlers();
  REQUIRE_EQ(active_handlers.size(), 1);
  std::remove("create_get_file_handler");
}

/***/
TEST_CASE("subscribe_get_active_different_handlers")
{
  HandlerCollection hc;

  // Create a file handler
  std::shared_ptr<Handler> filehandler = hc.create_handler<FileHandler>(
    "create_get_file_handler_1",
    []()
    {
      quill::FileHandlerConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  // Request the same file handler
  std::shared_ptr<Handler> filehandler_2 = hc.create_handler<FileHandler>(
    "create_get_file_handler_2",
    []()
    {
      quill::FileHandlerConfig cfg;
      cfg.set_open_mode('a');
      return cfg;
    }(),
    FileEventNotifier{});

  // Compare the pointers
  REQUIRE_NE(filehandler, filehandler_2);

  // Check for active file handlers
  auto active_handlers = hc.active_handlers();
  REQUIRE_EQ(active_handlers.size(), 0);

  // Subscribe the handler once
  hc.subscribe_handler(filehandler);
  active_handlers = hc.active_handlers();
  REQUIRE_EQ(active_handlers.size(), 1);

  // Subscribe the new handler - no effect as it is the same as before
  hc.subscribe_handler(filehandler_2);
  active_handlers = hc.active_handlers();
  REQUIRE_EQ(active_handlers.size(), 2);

  // Subscribe the same handler again - check no effect
  hc.subscribe_handler(filehandler);
  active_handlers = hc.active_handlers();
  REQUIRE_EQ(active_handlers.size(), 2);

  std::remove("create_get_file_handler_1");
  std::remove("create_get_file_handler_2");
}

TEST_SUITE_END();