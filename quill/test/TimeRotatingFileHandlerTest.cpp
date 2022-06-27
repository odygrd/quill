#include "doctest/doctest.h"

#include "quill/handlers/TimeRotatingFileHandler.h"

TEST_SUITE_BEGIN("TimeRotatingFileHandler");

using namespace quill::detail;
using namespace quill;

static fs::path const filename{"test_time_rotating_file_handler.log"};

#ifndef QUILL_NO_EXCEPTIONS
TEST_CASE("construct_invalid_when")
{
  std::string const when = "S";
  REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", when, 1, 1, Timezone::LocalTime, "12:00"});
}

TEST_CASE("construct_invalid_at_time")
{
  {
    std::string const at_time = "25:00";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }

  {
    std::string const at_time = "125:00";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }

  {
    std::string const at_time = "5:00";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }

  {
    std::string const at_time = "0:00";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }

  {
    std::string const at_time = ":00";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }

  {
    std::string const at_time = "01:000";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }

  {
    std::string const at_time = "01:0";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }

  {
    std::string const at_time = "01:";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }

  {
    std::string const at_time = "0";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }

  {
    std::string const at_time = "a";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }

  {
    std::string const at_time = "a:b";
    REQUIRE_THROWS(TimeRotatingFileHandler{filename, "a", "daily", 1, 1, Timezone::LocalTime, at_time});
  }
}
#endif

TEST_SUITE_END();