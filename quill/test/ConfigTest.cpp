#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/detail/Config.h"

TEST_SUITE_BEGIN("Config");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("backend_thread_sleep_duration")
{
  Config cfg;
  cfg.set_backend_thread_sleep_duration(std::chrono::microseconds{10});
  REQUIRE_EQ(cfg.backend_thread_sleep_duration(), std::chrono::nanoseconds{10000});
}

/***/
TEST_CASE("backend_thread_cpu_affinity")
{
  Config cfg;
  cfg.set_backend_thread_cpu_affinity(2);
  REQUIRE_EQ(cfg.backend_thread_cpu_affinity(), 2);
}

/***/
TEST_CASE("backend_thread_name")
{
  Config cfg;
  cfg.set_backend_thread_name("backend_thread");
  REQUIRE_STREQ(cfg.backend_thread_name().data(), "backend_thread");
}

TEST_SUITE_END();