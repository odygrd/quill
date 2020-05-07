#include "quill/detail/Config.h"
#include <gtest/gtest.h>

using namespace quill;
using namespace quill::detail;

/***/
TEST(Config, backend_thread_sleep_duration)
{
  Config cfg;
  cfg.set_backend_thread_sleep_duration(std::chrono::microseconds{10});
  EXPECT_EQ(cfg.backend_thread_sleep_duration(), std::chrono::nanoseconds{10000});
}

/***/
TEST(Config, backend_thread_cpu_affinity)
{
  Config cfg;
  cfg.set_backend_thread_cpu_affinity(2);
  EXPECT_EQ(cfg.backend_thread_cpu_affinity(), 2);
}

/***/
TEST(Config, backend_thread_name)
{
  Config cfg;
  cfg.set_backend_thread_name("backend_thread");
  EXPECT_EQ(cfg.backend_thread_name(), std::string{"backend_thread"});
}
