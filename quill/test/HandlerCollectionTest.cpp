#include "quill/detail/HandlerCollection.h"
#include <cstdio>
#include <gtest/gtest.h>

using namespace quill;
using namespace quill::detail;

/***/
TEST(HandlerCollection, stdout_stderr_handlers)
{
  HandlerCollection hc;

  // Get the default stdout stream handler
  StreamHandler* stdout_handler = hc.stdout_streamhandler();

#if defined(_WIN32)
  EXPECT_EQ(stdout_handler->filename(), std::wstring{L"stdout"});
#else
  EXPECT_EQ(stdout_handler->filename(), std::string{"stdout"});
#endif

  // Attempt to create a file handler with stdout as name and check that it is the same as the default
#if defined(_WIN32)
  StreamHandler* filehandler_1 = hc.file_handler(L"stdout");
#else
  StreamHandler* filehandler_1 = hc.file_handler("stdout");
#endif
  EXPECT_EQ(filehandler_1, stdout_handler);

  // Get the default stderr stream handler
  StreamHandler* stderr_handler = hc.stderr_streamhandler();
#if defined(_WIN32)
  EXPECT_EQ(stderr_handler->filename(), std::wstring{L"stderr"});
#else
  EXPECT_EQ(stderr_handler->filename(), std::string{"stderr"});
#endif

  // Attempt to create a file handler with stderr as name and check that it is the same as the default
#if defined(_WIN32)
  StreamHandler* filehandler_2 = hc.file_handler(L"stderr");
#else
  StreamHandler* filehandler_2 = hc.file_handler("stderr");
#endif
  EXPECT_EQ(filehandler_2, stderr_handler);
}

/***/
TEST(HandlerCollection, create_get)
{
  HandlerCollection hc;

#if defined(_WIN32)
  // Create a file handler
  StreamHandler* filehandler = hc.file_handler(L"create_get_file_handler", "w");

  // Request the same file handler
  StreamHandler* filehandler_2 = hc.file_handler(L"create_get_file_handler");
#else
  // Create a file handler
  StreamHandler* filehandler = hc.file_handler("create_get_file_handler", "w");

  // Request the same file handler
  StreamHandler* filehandler_2 = hc.file_handler("create_get_file_handler");
#endif

  // Compare the pointers
  EXPECT_EQ(filehandler, filehandler_2);
  std::remove("create_get_file_handler");
}

/***/
TEST(HandlerCollection, subscribe_get_active_same_handler)
{
  HandlerCollection hc;

#if defined(_WIN32)
  // Create a file handler
  StreamHandler* filehandler = hc.file_handler(L"create_get_file_handler", "w");

  // Request the same file handler
  StreamHandler* filehandler_2 = hc.file_handler(L"create_get_file_handler");
#else
  // Create a file handler
  StreamHandler* filehandler = hc.file_handler("create_get_file_handler", "w");

  // Request the same file handler
  StreamHandler* filehandler_2 = hc.file_handler("create_get_file_handler");
#endif

  // Compare the pointers
  EXPECT_EQ(filehandler, filehandler_2);

  // Check for active file handlers
  std::vector<Handler*> active_handlers = hc.active_handlers();
  EXPECT_EQ(active_handlers.size(), 0);

  // Subscribe the handler once
  hc.subscribe_handler(filehandler);
  active_handlers = hc.active_handlers();
  EXPECT_EQ(active_handlers.size(), 1);

  // Subscribe the new handler - no effect as it is the same as before
  hc.subscribe_handler(filehandler_2);
  active_handlers = hc.active_handlers();
  EXPECT_EQ(active_handlers.size(), 1);

  // Subscribe the same handler again - check no effect
  hc.subscribe_handler(filehandler);
  active_handlers = hc.active_handlers();
  EXPECT_EQ(active_handlers.size(), 1);
  std::remove("create_get_file_handler");
}

/***/
TEST(HandlerCollection, subscribe_get_active_different_handlers)
{
  HandlerCollection hc;

#if defined(_WIN32)
  // Create a file handler
  StreamHandler* filehandler = hc.file_handler(L"create_get_file_handler_1", "w");

  // Request the same file handler
  StreamHandler* filehandler_2 = hc.file_handler(L"create_get_file_handler_2");
#else
  // Create a file handler
  StreamHandler* filehandler = hc.file_handler("create_get_file_handler_1", "w");

  // Request the same file handler
  StreamHandler* filehandler_2 = hc.file_handler("create_get_file_handler_2");
#endif

  // Compare the pointers
  EXPECT_NE(filehandler, filehandler_2);

  // Check for active file handlers
  std::vector<Handler*> active_handlers = hc.active_handlers();
  EXPECT_EQ(active_handlers.size(), 0);

  // Subscribe the handler once
  hc.subscribe_handler(filehandler);
  active_handlers = hc.active_handlers();
  EXPECT_EQ(active_handlers.size(), 1);

  // Subscribe the new handler - no effect as it is the same as before
  hc.subscribe_handler(filehandler_2);
  active_handlers = hc.active_handlers();
  EXPECT_EQ(active_handlers.size(), 2);

  // Subscribe the same handler again - check no effect
  hc.subscribe_handler(filehandler);
  active_handlers = hc.active_handlers();
  EXPECT_EQ(active_handlers.size(), 2);

  std::remove("create_get_file_handler_1");
  std::remove("create_get_file_handler_2");
}