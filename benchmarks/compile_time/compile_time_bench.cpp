#include "qwrapper/qwrapper.h"

#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"

int main()
{
  setup_quill("recommended_usage.log");
  auto logger = quill::Frontend::get_logger("root");

  LOG_INFO(logger, "example lazy brown {} {} {}", "example2", 4.0f, "example3");
  LOG_INFO(logger, "jumps brown example lazy {} {} {} {} {}", std::string("str1"),
           static_cast<short>(9), 6LL, "example1", static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown test jumps fox {} {} {} {} {} {} {} {} {}", false, std::string("str1"),
           3.0, "example3", 6LL, "example2", static_cast<unsigned short>(10), 1, true);
  LOG_INFO(logger, "dog test example lazy {} {} {} {} {} {} {} {} {} {}", std::string_view("view1"), 7UL,
           static_cast<short>(9), false, 6LL, 5L, true, "example3", "example2", std::string("str1"));
  LOG_INFO(logger, "example fox lazy {} {} {} {} {} {} {} {} {} {}", "example1", 4.0f, 7UL, 5L,
           true, 2, 3.0, 6LL, false, "example3");
  LOG_INFO(logger, "quick test over fox {} {} {} {}", true, static_cast<short>(9), "example3", 6LL);
  LOG_INFO(logger, "lazy logging test {} {} {} {} {}", 8ULL, 4.0f, std::string_view("view2"),
           "example1", "example3");
  LOG_INFO(logger, "lazy over fox {} {}", std::string_view("view1"), 8ULL);
  LOG_INFO(logger, "lazy fox brown {} {} {}", 5L, static_cast<short>(9), static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging brown over dog {} {} {} {} {} {} {} {}", std::string_view("view1"),
           "example2", std::string("str2"), std::string("str1"), 8ULL, std::string_view("view2"), false, 2);
  LOG_INFO(logger, "brown fox test example {} {} {} {}", static_cast<unsigned short>(10), 6LL,
           static_cast<short>(9), 7UL);
  LOG_INFO(logger, "fox logging brown {} {} {} {} {} {} {} {}", 2, std::string("str2"),
           static_cast<short>(9), "example2", false, 7UL, "example3", 4.0f);
  LOG_INFO(logger, "over quick logging lazy {} {} {} {}", std::string("str2"), "example3", 7UL, 5L);
  LOG_INFO(logger, "dog over test jumps {} {} {} {} {}", 1, std::string_view("view2"), 8ULL,
           "example3", 4.0f);
  LOG_INFO(logger, "fox example dog {}", 5L);
  LOG_INFO(logger, "example test fox dog {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10), 6LL,
           3.0, static_cast<short>(9), 2, false, std::string_view("view2"), std::string_view("view1"));
  LOG_INFO(logger, "test lazy jumps fox {} {} {} {} {} {}", 4.0f, std::string_view("view1"),
           static_cast<unsigned short>(10), std::string("str2"), 3.0, true);
  LOG_INFO(logger, "over jumps brown {}", 4.0f);
  LOG_INFO(logger, "dog brown over {} {} {} {} {} {} {} {}", "example1",
           static_cast<unsigned short>(10), false, 1, 6LL, std::string_view("view1"), 2, 8ULL);
  LOG_INFO(logger, "over test jumps {} {} {} {} {} {} {} {}", true, std::string("str1"), 6LL,
           "example3", 1, 7UL, 2, "example1");
  LOG_INFO(logger, "over dog example fox {} {} {} {} {} {} {} {} {} {}", 8ULL, 2, 5L, std::string("str1"),
           std::string_view("view1"), std::string_view("view2"), 1, "example1", 7UL, std::string("str2"));
  LOG_INFO(logger, "logging lazy fox {} {} {} {} {} {} {} {} {} {}", 8ULL, std::string("str2"), 7UL,
           false, std::string("str1"), "example3", 3.0, std::string_view("view1"), "example1", 6LL);
  LOG_INFO(logger, "logging example quick dog {} {} {} {} {} {} {} {} {}", 6LL, 1, 5L, 3.0,
           "example3", 4.0f, 8ULL, std::string("str1"), false);
  LOG_INFO(logger, "example jumps over {} {} {} {} {} {}", 6LL, static_cast<short>(9),
           static_cast<unsigned short>(10), "example3", true, 7UL);
  LOG_INFO(logger, "dog fox brown quick {} {} {}", 1, 5L, 6LL);
  LOG_INFO(logger, "brown example dog {} {} {} {}", std::string("str2"), 6LL, true, std::string_view("view1"));
  LOG_INFO(logger, "jumps example fox test {} {}", 6LL, std::string("str1"));
  LOG_INFO(logger, "brown over dog fox {} {} {} {} {} {} {}", std::string("str1"),
           std::string("str2"), 4.0f, false, 8ULL, "example2", true);
  LOG_INFO(logger, "example over fox dog {} {} {} {} {} {} {} {} {} {}", "example1", 1,
           static_cast<short>(9), static_cast<unsigned short>(10), "example2", std::string("str1"),
           false, std::string_view("view1"), 4.0f, 7UL);
  LOG_INFO(logger, "brown example test quick {} {} {} {} {} {}", std::string("str2"), 3.0,
           static_cast<short>(9), "example2", std::string("str1"), 8ULL);
  LOG_INFO(logger, "fox brown jumps {} {} {} {} {} {} {} {} {} {}", "example3", std::string("str2"),
           "example2", 7UL, false, 6LL, 5L, 4.0f, 2, std::string("str1"));
  LOG_INFO(logger, "brown quick dog over {} {} {} {} {} {} {} {}", 1, 4.0f, std::string_view("view1"),
           false, true, "example2", static_cast<unsigned short>(10), std::string("str2"));
  LOG_INFO(logger, "lazy jumps brown {} {} {}", 8ULL, 5L, "example1");
  LOG_INFO(logger, "example dog over {} {} {} {} {} {}", static_cast<short>(9), "example2", 3.0,
           std::string("str1"), 5L, std::string_view("view2"));
  LOG_INFO(logger, "fox example dog {} {} {} {} {} {}", true, "example2", 3.0, 5L, 6LL, 1);
  LOG_INFO(logger, "example lazy dog {} {} {} {} {} {} {}", 6LL, std::string("str2"), 4.0f,
           "example2", std::string("str1"), static_cast<unsigned short>(10), 8ULL);
  LOG_INFO(logger, "fox logging brown {} {} {} {} {} {}", 2, true, "example1", 6LL,
           std::string_view("view1"), 7UL);
  LOG_INFO(logger, "example brown over test {} {}", 5L, std::string_view("view1"));
  LOG_INFO(logger, "over test dog {} {} {} {} {}", std::string("str1"), std::string_view("view1"),
           "example2", "example1", 6LL);
  LOG_INFO(logger, "logging test example {} {} {} {} {} {} {} {} {} {}", 6LL, "example3",
           static_cast<short>(9), true, 7UL, 5L, std::string_view("view2"), 2, 1, std::string_view("view1"));
  LOG_INFO(logger, "example test lazy quick {} {} {} {} {} {} {} {}", std::string("str1"), 6LL, 7UL,
           std::string_view("view2"), static_cast<short>(9), 5L, std::string("str2"), "example1");
  LOG_INFO(logger, "over lazy logging {} {} {} {} {}", "example3", std::string_view("view1"), 6LL,
           false, "example2");
  LOG_INFO(logger, "lazy example jumps over {} {} {} {} {} {} {} {}", 3.0, "example3", 2,
           "example2", 4.0f, 6LL, true, static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox brown example {}", 5L);
  LOG_INFO(logger, "jumps lazy test over {} {}", false, std::string_view("view2"));
  LOG_INFO(logger, "lazy over quick {} {} {} {} {} {} {}", true, 7UL, std::string("str1"),
           "example1", 5L, 4.0f, static_cast<short>(9));
  LOG_INFO(logger, "fox brown over jumps {} {} {} {} {} {} {}", 1, 3.0, true, std::string("str2"),
           4.0f, 6LL, "example3");
  LOG_INFO(logger, "example logging test quick {} {} {} {} {} {} {}", 1, "example2",
           static_cast<short>(9), 6LL, static_cast<unsigned short>(10), 5L, std::string_view("view1"));
  LOG_INFO(logger, "brown logging quick test {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           5L, true, 8ULL, "example1", 2, 3.0, "example3", std::string("str2"));
  LOG_INFO(logger, "logging dog over fox {} {} {}", false, std::string_view("view2"), 7UL);
  LOG_INFO(logger, "quick fox test {} {} {} {} {} {} {} {}", std::string("str2"), 4.0f, 8ULL,
           "example3", 5L, false, 7UL, std::string_view("view1"));
  LOG_INFO(logger, "over logging lazy {} {} {} {} {} {} {} {}", 7UL, "example1", 3.0, "example2",
           std::string_view("view2"), static_cast<short>(9), std::string_view("view1"), 1);
  LOG_INFO(logger, "brown dog example fox {} {} {} {} {} {} {} {} {}", std::string("str2"),
           std::string_view("view1"), 1, 4.0f, 2, "example1", "example2", 7UL, 5L);
  LOG_INFO(logger, "jumps brown dog test {} {} {} {} {} {} {} {} {}", "example2",
           std::string("str2"), 7UL, 2, static_cast<short>(9), false, 6LL, 5L, 3.0);
  LOG_INFO(logger, "quick lazy logging {} {} {}", 4.0f, "example1", static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps test quick fox {} {} {} {} {} {} {} {}", false, 4.0f, 1,
           static_cast<short>(9), "example1", 2, 6LL, "example3");
  LOG_INFO(logger, "over example dog {} {} {} {} {}", 6LL, 5L, "example1", 1, 4.0f);
  LOG_INFO(logger, "brown fox logging {} {} {} {}", 5L, 3.0, 7UL, static_cast<short>(9));
  LOG_INFO(logger, "over example brown {} {} {} {} {} {} {} {} {}", 2, std::string("str1"), 7UL,
           true, false, 1, 5L, std::string("str2"), "example3");
  LOG_INFO(logger, "lazy test over jumps {} {} {} {} {} {} {} {} {} {}", false, "example3",
           std::string_view("view2"), static_cast<short>(9), 2, 4.0f, std::string("str2"),
           std::string_view("view1"), std::string("str1"), "example2");
  LOG_INFO(logger, "test quick brown {} {}", std::string_view("view2"), std::string("str2"));
  LOG_INFO(logger, "dog example over lazy {} {} {}", "example3", static_cast<unsigned short>(10), true);
  LOG_INFO(logger, "over test brown lazy {} {}", 3.0, 4.0f);
  LOG_INFO(logger, "lazy jumps example {} {} {} {} {} {} {}", std::string_view("view1"), false, 6LL,
           "example2", true, "example1", static_cast<short>(9));
  LOG_INFO(logger, "fox example jumps over {} {} {} {} {} {} {} {} {}", 4.0f, 7UL, "example2", 8ULL,
           std::string("str2"), 6LL, false, "example3", static_cast<short>(9));
  LOG_INFO(logger, "dog example lazy jumps {} {} {} {} {} {} {} {}", 3.0, std::string_view("view2"),
           2, 6LL, std::string_view("view1"), std::string("str1"), 4.0f, std::string("str2"));
  LOG_INFO(logger, "quick lazy test dog {} {}", "example1", std::string("str1"));
  LOG_INFO(logger, "fox example quick dog {} {} {} {} {} {} {} {}", 2, 6LL,
           static_cast<unsigned short>(10), 7UL, std::string_view("view1"), 1, "example2", false);
  LOG_INFO(logger, "jumps test quick fox {} {} {} {} {} {} {} {} {}", 8ULL, 2, std::string_view("view2"),
           false, 6LL, 3.0, static_cast<short>(9), "example1", "example2");
  LOG_INFO(logger, "dog jumps quick lazy {} {} {}", 2, 4.0f, "example2");
  LOG_INFO(logger, "dog over brown logging {} {} {} {}", std::string("str1"), 1, "example2", false);
  LOG_INFO(logger, "dog lazy over example {} {} {} {}", 2, std::string("str2"), std::string("str1"),
           "example1");
  LOG_INFO(logger, "logging jumps brown fox {} {} {} {} {} {} {} {} {} {}", std::string("str2"),
           std::string_view("view2"), 7UL, std::string("str1"), 3.0, true, false, 6LL, 2,
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "example over jumps brown {} {} {} {} {}", std::string_view("view2"),
           std::string("str2"), 3.0, 2, 4.0f);
  LOG_INFO(logger, "example dog jumps lazy {} {} {} {} {} {}", 8ULL, "example3", "example2", 6LL, true, false);
  LOG_INFO(logger, "example dog fox {} {} {} {} {} {} {} {} {}", "example2", 1, 3.0,
           std::string("str1"), "example1", 8ULL, std::string_view("view2"), false, 2);
  LOG_INFO(logger, "example fox over {} {} {} {} {} {} {}", 7UL, 6LL,
           static_cast<unsigned short>(10), true, 4.0f, "example2", 5L);
  LOG_INFO(logger, "lazy example jumps logging {} {} {} {} {} {} {}", false, "example3", 4.0f,
           "example1", 8ULL, 3.0, std::string_view("view2"));
  LOG_INFO(logger, "test dog lazy {} {} {}", std::string_view("view2"), 3.0, 5L);
  LOG_INFO(logger, "jumps example over logging {} {} {}", 7UL, 4.0f, static_cast<short>(9));
  LOG_INFO(logger, "dog example jumps quick {} {} {} {}", 2, 5L, 1, static_cast<short>(9));
  LOG_INFO(logger, "test quick fox over {} {} {} {} {}", "example3", 1, 7UL, "example2",
           std::string_view("view1"));
  LOG_INFO(logger, "brown jumps over {} {} {} {} {} {} {} {} {}", "example2", false, std::string_view("view1"),
           std::string("str2"), true, "example3", 3.0, 5L, static_cast<short>(9));
  LOG_INFO(logger, "dog over fox {} {} {} {}", std::string("str2"), std::string("str1"), 1,
           "example2");
  LOG_INFO(logger, "brown lazy dog over {} {} {} {} {}", std::string_view("view2"), 3.0, 4.0f, true,
           std::string_view("view1"));
  LOG_INFO(logger, "jumps lazy example {} {} {} {} {} {} {} {} {}", static_cast<short>(9),
           "example3", std::string_view("view2"), std::string("str1"), true, 7UL,
           static_cast<unsigned short>(10), 6LL, 5L);
  LOG_INFO(logger, "dog quick jumps {} {} {} {} {} {} {} {} {} {}", "example3", 5L, true,
           std::string("str1"), 8ULL, "example2", std::string_view("view1"), 1, 7UL, "example1");
  LOG_INFO(logger, "jumps quick example {} {} {} {} {} {} {}", 7UL, 5L, 2, static_cast<short>(9),
           std::string_view("view2"), 6LL, std::string("str1"));
  LOG_INFO(logger, "quick logging brown {} {} {} {} {} {} {}", 3.0, "example1",
           static_cast<unsigned short>(10), 6LL, static_cast<short>(9), 2, "example2");
  LOG_INFO(logger, "example test dog quick {}", static_cast<short>(9));
  LOG_INFO(logger, "lazy jumps dog {}", 7UL);
  LOG_INFO(logger, "example fox quick {} {} {} {} {} {} {}", std::string_view("view2"), 7UL, 2,
           8ULL, std::string("str1"), 6LL, 4.0f);
  LOG_INFO(logger, "brown over dog test {} {}", 7UL, "example3");
  LOG_INFO(logger, "logging dog fox test {} {} {} {} {} {}", 4.0f, static_cast<short>(9), true,
           std::string_view("view2"), static_cast<unsigned short>(10), std::string("str1"));
  LOG_INFO(logger, "quick dog test {} {} {} {} {} {} {}", 6LL, 8ULL, "example2",
           std::string("str1"), "example1", true, false);
  LOG_INFO(logger, "jumps dog logging over {} {} {} {}", "example1", static_cast<unsigned short>(10), 4.0f, false);
  LOG_INFO(logger, "lazy dog example {} {} {} {} {} {} {} {} {}", true, 1, 2, 5L,
           static_cast<unsigned short>(10), "example1", 3.0, 8ULL, "example3");
  LOG_INFO(logger, "fox dog lazy jumps {} {} {}", std::string_view("view1"), 3.0, 8ULL);
  LOG_INFO(logger, "lazy logging test quick {} {} {}", 5L, std::string_view("view2"), std::string("str1"));
  LOG_INFO(logger, "jumps logging quick {}", false);
  LOG_INFO(logger, "quick logging over {} {} {} {} {} {} {} {}", 5L, std::string_view("view1"), 1,
           std::string_view("view2"), true, "example3", static_cast<unsigned short>(10), false);
  LOG_INFO(logger, "fox lazy test {} {} {} {} {} {} {}", std::string("str2"), "example3", 6LL, 1,
           true, false, 4.0f);
  LOG_INFO(logger, "fox over quick dog {} {} {} {}", "example2", true,
           static_cast<unsigned short>(10), std::string("str1"));
  LOG_INFO(logger, "brown fox dog logging {} {} {} {} {} {} {} {}", 3.0, static_cast<unsigned short>(10),
           std::string_view("view1"), 7UL, 4.0f, false, 1, "example1");
  LOG_INFO(logger, "fox logging quick lazy {} {} {} {} {} {} {}", 5L, true, "example1", 6LL,
           std::string_view("view2"), "example2", static_cast<unsigned short>(10));
  LOG_INFO(logger, "quick dog jumps example {} {} {} {}", 6LL, std::string_view("view1"),
           static_cast<unsigned short>(10), std::string("str1"));
  LOG_INFO(logger, "quick brown fox {} {} {} {} {} {} {} {} {} {}", std::string("str1"), 2,
           std::string_view("view1"), 4.0f, false, "example1", 5L, static_cast<unsigned short>(10),
           static_cast<short>(9), std::string("str2"));
  LOG_INFO(logger, "dog quick brown {} {} {} {} {} {} {} {}", true, static_cast<short>(9), 8ULL,
           std::string("str2"), static_cast<unsigned short>(10), 6LL, "example2", 2);
  LOG_INFO(logger, "jumps dog logging over {} {} {} {} {} {} {} {}", 8ULL, true,
           std::string("str2"), "example2", 4.0f, 5L, 6LL, static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog lazy example {}", static_cast<short>(9));
  LOG_INFO(logger, "brown logging lazy {} {} {} {} {} {}", std::string_view("view2"), 5L, 2,
           static_cast<unsigned short>(10), true, "example1");
  LOG_INFO(logger, "jumps over brown {} {} {} {} {} {}", false, true, static_cast<short>(9), 2,
           std::string("str2"), "example3");
  LOG_INFO(logger, "lazy example quick {} {} {} {} {}", true, 3.0, 7UL, 4.0f, std::string_view("view1"));
  LOG_INFO(logger, "logging dog over {} {} {} {} {} {}", std::string_view("view1"), 7UL, "example3",
           true, static_cast<unsigned short>(10), 5L);
  LOG_INFO(logger, "jumps quick logging {} {}", 2, 7UL);
  LOG_INFO(logger, "test lazy brown jumps {} {} {} {} {} {} {} {}", "example1", std::string("str1"),
           4.0f, 3.0, static_cast<unsigned short>(10), std::string_view("view1"), 6LL, 7UL);
  LOG_INFO(logger, "jumps example lazy test {} {}", 1, true);
  LOG_INFO(logger, "jumps over dog {} {} {} {} {} {} {}", "example3", std::string_view("view1"),
           static_cast<short>(9), static_cast<unsigned short>(10), std::string_view("view2"), true, 1);
  LOG_INFO(logger, "logging dog example {} {} {} {}", false, 5L, std::string("str2"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging quick brown {} {} {} {} {} {} {} {} {} {}", 8ULL, "example3",
           std::string("str1"), "example1", "example2", 1, 3.0, 4.0f, 7UL, std::string_view("view2"));
  LOG_INFO(logger, "logging over test {} {} {} {} {} {} {} {} {}", 2, std::string_view("view2"),
           "example1", 6LL, true, 7UL, static_cast<short>(9), static_cast<unsigned short>(10), 3.0);
  LOG_INFO(logger, "over example fox dog {} {}", std::string_view("view1"), 1);
  LOG_INFO(logger, "quick logging lazy dog {} {}", "example2", std::string("str2"));
  LOG_INFO(logger, "lazy over quick example {} {} {} {} {} {} {}", std::string("str2"), false, 4.0f,
           std::string_view("view2"), 3.0, 6LL, std::string("str1"));
  LOG_INFO(logger, "test dog brown {} {} {} {} {} {} {} {} {}", std::string("str1"), "example2",
           4.0f, 3.0, 8ULL, static_cast<unsigned short>(10), false, 7UL, 5L);
  LOG_INFO(logger, "brown quick logging jumps {} {} {} {} {} {}", static_cast<unsigned short>(10),
           "example2", true, false, 2, std::string_view("view1"));
  LOG_INFO(logger, "quick over example {} {} {} {} {} {} {}", 5L, 1, 4.0f,
           std::string_view("view2"), std::string("str2"), "example1", 3.0);
  LOG_INFO(logger, "fox quick over test {} {}", std::string_view("view1"), 3.0);
  LOG_INFO(logger, "dog lazy fox jumps {} {} {} {} {} {} {} {} {} {}", "example3", 1, 7UL,
           static_cast<unsigned short>(10), std::string("str2"), "example2", "example1",
           static_cast<short>(9), 6LL, 4.0f);
  LOG_INFO(logger, "dog fox jumps example {} {} {} {} {} {} {} {}", static_cast<short>(9),
           "example3", 5L, 7UL, 2, std::string_view("view2"), false, static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog quick example lazy {} {} {} {} {} {} {} {}", 8ULL, 2, "example3", 4.0f,
           std::string_view("view2"), static_cast<short>(9), static_cast<unsigned short>(10), 6LL);
  LOG_INFO(logger, "over example brown {}", 6LL);
  LOG_INFO(logger, "logging test jumps quick {} {} {} {} {}", true, std::string("str2"), 2,
           std::string_view("view1"), 7UL);
  LOG_INFO(logger, "brown fox over {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string_view("view2"), false, std::string("str1"), "example1", 7UL,
           std::string("str2"), true, 6LL, 1);
  LOG_INFO(logger, "logging test dog fox {}", 3.0);
  LOG_INFO(logger, "over brown dog {} {} {} {} {} {} {}", "example1", "example2", 6LL,
           std::string("str1"), 7UL, std::string_view("view1"), false);
  LOG_INFO(logger, "fox over lazy {} {} {} {} {} {} {} {} {} {}", static_cast<short>(9), 8ULL, 5L, 6LL,
           std::string_view("view2"), std::string_view("view1"), 1, std::string("str1"), "example2", 3.0);
  LOG_INFO(logger, "lazy quick fox {} {} {} {}", std::string("str2"), "example3", 1, std::string_view("view2"));
  LOG_INFO(logger, "brown fox logging lazy {} {} {} {} {}", std::string_view("view2"),
           std::string("str1"), 5L, std::string_view("view1"), "example3");
  LOG_INFO(logger, "brown over fox {} {} {}", 2, 6LL, 4.0f);
  LOG_INFO(logger, "logging fox jumps over {} {} {} {} {} {} {} {}", 7UL, 4.0f, "example1", 8ULL,
           "example2", 5L, false, std::string("str2"));
  LOG_INFO(logger, "over test logging jumps {} {} {} {} {} {}", true, 4.0f, "example1", 7UL,
           std::string_view("view1"), "example3");
  LOG_INFO(logger, "example quick test {} {} {} {} {} {} {} {}", std::string("str1"), 7UL, 6LL, 3.0,
           2, 8ULL, 1, static_cast<unsigned short>(10));
  LOG_INFO(logger, "example brown quick test {} {} {} {} {} {} {} {}", 2, false, 6LL, 4.0f,
           std::string_view("view1"), 8ULL, "example3", "example2");
  LOG_INFO(logger, "brown example quick test {} {}", std::string("str1"), 8ULL);
  LOG_INFO(logger, "logging example brown {} {} {} {} {}", std::string("str1"), 5L, 4.0f,
           static_cast<short>(9), "example1");
  LOG_INFO(logger, "quick logging lazy {} {}", static_cast<short>(9), "example3");
  LOG_INFO(logger, "dog jumps over fox {} {} {} {} {} {} {}", 3.0, static_cast<short>(9), 6LL,
           false, 7UL, std::string_view("view1"), true);
  LOG_INFO(logger, "brown dog example {} {} {} {} {} {} {} {} {}", "example1", std::string("str2"), true,
           static_cast<unsigned short>(10), 7UL, 8ULL, 1, std::string("str1"), std::string_view("view1"));
  LOG_INFO(logger, "example test quick logging {} {} {} {} {} {} {} {} {}", 6LL, "example1",
           "example3", 1, false, static_cast<unsigned short>(10), 2, std::string("str2"), std::string("str1"));
  LOG_INFO(logger, "quick lazy jumps example {} {} {} {} {} {} {} {}", "example2", 6LL, 4.0f, 7UL,
           2, 5L, std::string_view("view2"), static_cast<short>(9));
  LOG_INFO(logger, "dog test logging quick {} {}", false, "example2");
  LOG_INFO(logger, "example dog over lazy {} {} {} {} {} {} {} {} {} {}", 6LL, 2, 3.0,
           std::string_view("view2"), "example3", 4.0f, 8ULL, true, std::string("str2"), std::string("str1"));
  LOG_INFO(logger, "test example logging {} {} {} {}", 1, std::string("str1"), 6LL, false);
  LOG_INFO(logger, "logging over jumps lazy {} {} {} {} {} {} {} {} {}", true, 7UL, "example2",
           static_cast<short>(9), "example3", std::string_view("view2"), 5L,
           std::string_view("view1"), std::string("str2"));
  LOG_INFO(logger, "fox logging brown {} {} {} {} {} {} {} {} {} {}", std::string_view("view1"), 2,
           false, static_cast<short>(9), 3.0, "example2", std::string_view("view2"),
           static_cast<unsigned short>(10), true, "example1");
  LOG_INFO(logger, "over jumps logging {} {} {}", 5L, std::string("str2"), true);
  LOG_INFO(logger, "lazy fox example brown {}", std::string_view("view1"));
  LOG_INFO(logger, "test over quick {} {}", 8ULL, 5L);
  LOG_INFO(logger, "example jumps quick fox {}", 6LL);
  LOG_INFO(logger, "fox dog example quick {} {} {} {} {} {} {} {} {} {}", 5L, 7UL, "example2", 8ULL,
           3.0, static_cast<short>(9), 6LL, true, static_cast<unsigned short>(10), "example1");
  LOG_INFO(logger, "over test example dog {} {} {} {} {} {} {} {}", 2, 8ULL, 4.0f, "example1",
           static_cast<short>(9), std::string("str2"), 5L, std::string_view("view1"));
  LOG_INFO(logger, "over dog test {} {}", 7UL, 2);
  LOG_INFO(logger, "brown fox logging {}", 7UL);
  LOG_INFO(logger, "fox jumps logging dog {} {} {} {} {} {} {} {}", 3.0, "example1", "example3",
           std::string_view("view2"), false, 4.0f, 5L, true);
  LOG_INFO(logger, "brown example logging {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown jumps logging over {} {} {} {} {} {}", 6LL, 1, 8ULL, "example2",
           std::string_view("view2"), "example3");
  LOG_INFO(logger, "logging fox brown {} {} {} {} {}", 2, 8ULL, static_cast<short>(9), 6LL, 3.0);
  LOG_INFO(logger, "lazy dog jumps fox {} {} {} {} {} {} {}", std::string_view("view2"),
           std::string("str2"), std::string_view("view1"), "example1", 1, false, 4.0f);
  LOG_INFO(logger, "brown quick dog {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown dog lazy {} {} {} {} {}", static_cast<unsigned short>(10), 3.0,
           std::string_view("view2"), static_cast<short>(9), 2);
  LOG_INFO(logger, "lazy fox quick {} {} {}", 5L, "example1", static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox over quick {} {} {} {} {} {}", 4.0f, 6LL, std::string_view("view1"),
           "example2", 1, std::string_view("view2"));
  LOG_INFO(logger, "fox brown dog {} {} {} {} {} {} {}", false, 3.0, 4.0f,
           std::string_view("view2"), 1, 6LL, 5L);
  LOG_INFO(logger, "test brown jumps {} {} {} {} {}", true, 4.0f, static_cast<unsigned short>(10),
           "example1", std::string("str1"));
  LOG_INFO(logger, "example fox test dog {} {}", std::string_view("view2"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown example quick {} {} {} {} {} {} {} {} {} {}", 2, 6LL, "example3",
           static_cast<short>(9), std::string_view("view1"), false, 4.0f, std::string("str1"), 7UL, 1);
  LOG_INFO(logger, "example lazy over {} {} {} {} {} {}", static_cast<short>(9), 5L, "example3",
           std::string("str1"), 7UL, std::string("str2"));
  LOG_INFO(logger, "quick logging fox {} {} {} {} {} {}", 5L, true, false, 2, std::string_view("view1"), 4.0f);
  LOG_INFO(logger, "logging lazy dog {} {} {} {} {} {}", std::string_view("view1"), 4.0f,
           "example1", "example3", false, std::string("str2"));
  LOG_INFO(logger, "fox dog over test {} {} {} {} {} {} {} {} {}", std::string("str2"), 5L, 3.0,
           "example3", static_cast<short>(9), true, 6LL, 8ULL, 1);
  LOG_INFO(logger, "dog lazy jumps {}", static_cast<short>(9));
  LOG_INFO(logger, "test example dog {} {} {} {} {} {} {} {} {} {}",
           static_cast<unsigned short>(10), std::string("str2"), 7UL, std::string("str1"), true,
           static_cast<short>(9), std::string_view("view2"), false, 1, 8ULL);
  LOG_INFO(logger, "fox jumps dog test {} {} {} {} {}", 1, 8ULL, std::string("str1"), "example2",
           std::string_view("view2"));
  LOG_INFO(logger, "example brown fox quick {} {}", static_cast<short>(9), false);
  LOG_INFO(logger, "over quick dog {} {} {} {} {}", 5L, 1, 8ULL, 7UL, 3.0);
  LOG_INFO(logger, "brown test over quick {} {} {} {} {} {} {} {} {} {}", std::string_view("view1"),
           3.0, 4.0f, 2, false, "example3", 7UL, static_cast<short>(9), true, 5L);
  LOG_INFO(logger, "brown lazy over {} {} {} {} {} {}", std::string("str2"), 7UL,
           static_cast<short>(9), true, 8ULL, 2);
  LOG_INFO(logger, "brown lazy fox {} {} {} {} {} {} {} {}", 7UL, 4.0f, static_cast<short>(9), 1,
           6LL, false, "example2", std::string_view("view1"));
  LOG_INFO(logger, "fox logging jumps quick {} {}", 6LL, 5L);
  LOG_INFO(logger, "quick fox brown {} {} {} {} {}", std::string_view("view1"),
           std::string_view("view2"), "example1", 2, "example2");
  LOG_INFO(logger, "example quick over brown {} {} {}", 8ULL, static_cast<unsigned short>(10),
           std::string_view("view1"));
  LOG_INFO(logger, "quick over lazy {} {} {} {} {} {}", 7UL, 2, static_cast<unsigned short>(10), 1, 3.0, 8ULL);
  LOG_INFO(logger, "logging fox test brown {} {} {} {} {}", false, 8ULL, 1, 5L, std::string_view("view2"));
  LOG_INFO(logger, "fox over dog test {}", std::string_view("view2"));
  LOG_INFO(logger, "brown jumps logging {} {} {} {} {}", 4.0f, 8ULL, true,
           std::string_view("view1"), static_cast<short>(9));
  LOG_INFO(logger, "quick over brown jumps {} {} {} {} {} {}", "example2", "example3", 7UL, false, 8ULL, 5L);
  LOG_INFO(logger, "test dog example over {} {} {} {} {} {}", 2, 1, static_cast<short>(9),
           "example3", std::string_view("view2"), std::string_view("view1"));
  LOG_INFO(logger, "fox logging lazy over {} {} {} {}", "example2", std::string("str1"), 5L, 3.0);
  LOG_INFO(logger, "logging fox test {} {} {} {}", false, static_cast<short>(9),
           static_cast<unsigned short>(10), "example3");
  LOG_INFO(logger, "brown logging dog example {} {} {} {} {} {} {} {} {}", "example2",
           std::string_view("view1"), std::string("str1"), "example3", std::string_view("view2"),
           "example1", false, static_cast<unsigned short>(10), 5L);
  LOG_INFO(logger, "dog test example over {}", "example1");
  LOG_INFO(logger, "over jumps logging test {} {} {} {}", true, 4.0f, std::string("str2"), false);
  LOG_INFO(logger, "brown example test logging {} {} {} {} {} {}", 3.0, 6LL, std::string("str2"),
           static_cast<unsigned short>(10), "example2", std::string_view("view1"));
  LOG_INFO(logger, "example over dog quick {} {} {} {} {}", 1, 2, false, "example1", "example3");
  LOG_INFO(logger, "test dog example {} {} {} {} {} {} {} {} {} {}", true, "example2", "example3",
           std::string("str2"), 1, static_cast<unsigned short>(10), std::string_view("view1"),
           "example1", 7UL, false);
  LOG_INFO(logger, "dog example lazy test {} {} {} {}", "example2", std::string_view("view2"), 5L, 7UL);
  LOG_INFO(logger, "logging over dog {}", std::string("str1"));
  LOG_INFO(logger, "logging jumps test example {} {} {} {} {} {} {}", "example2", 8ULL,
           std::string_view("view1"), "example1", static_cast<unsigned short>(10), 5L, "example3");
  LOG_INFO(logger, "quick dog example {} {} {} {} {} {} {} {} {} {}", std::string("str1"),
           std::string_view("view2"), true, 8ULL, std::string_view("view1"), 7UL,
           static_cast<short>(9), 3.0, static_cast<unsigned short>(10), std::string("str2"));
  LOG_INFO(logger, "brown quick over {} {}", 7UL, 2);
  LOG_INFO(logger, "over lazy fox test {} {}", 8ULL, std::string("str2"));
  LOG_INFO(logger, "example test brown jumps {} {}", std::string("str2"), 4.0f);
  LOG_INFO(logger, "quick example lazy {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"), false,
           2, static_cast<short>(9), 3.0, "example3", std::string_view("view1"), "example1", true, 1);
  LOG_INFO(logger, "dog fox test {} {} {} {} {} {} {}", 3.0, false, static_cast<short>(9),
           static_cast<unsigned short>(10), 6LL, 1, "example3");
  LOG_INFO(logger, "test quick fox logging {} {} {} {}", true, 1, 7UL, 2);
  LOG_INFO(logger, "brown example dog {} {} {} {} {}", std::string("str1"), 5L, "example1",
           std::string("str2"), "example2");
  LOG_INFO(logger, "logging test dog {} {} {} {} {} {} {}", 5L, static_cast<short>(9), 8ULL, false,
           4.0f, 6LL, std::string("str2"));
  LOG_INFO(logger, "jumps brown quick {} {} {} {} {}", "example3", "example2", std::string("str2"), 4.0f, 5L);
  LOG_INFO(logger, "test over dog {} {} {} {} {} {} {} {} {} {}", 6LL, 2, "example3",
           std::string_view("view1"), static_cast<short>(9), 1, 5L, "example1", 4.0f, "example2");
  LOG_INFO(logger, "jumps test quick {} {} {} {} {} {} {}", false, "example2", 8ULL, std::string_view("view1"),
           static_cast<unsigned short>(10), static_cast<short>(9), std::string("str1"));
  LOG_INFO(logger, "test logging fox {} {} {} {} {} {} {} {} {}", static_cast<short>(9), "example3",
           std::string_view("view1"), true, std::string("str2"), 7UL,
           static_cast<unsigned short>(10), std::string("str1"), 8ULL);
  LOG_INFO(logger, "lazy dog over {} {} {} {} {} {} {} {} {}", 4.0f, 1, "example3",
           std::string("str2"), true, 3.0, 5L, "example2", std::string_view("view2"));
  LOG_INFO(logger, "test lazy dog {} {} {} {}", true, std::string_view("view2"), 6LL, "example1");
  LOG_INFO(logger, "jumps over logging example {} {} {} {} {}", std::string_view("view1"),
           "example2", static_cast<unsigned short>(10), std::string("str2"), 3.0);
  LOG_INFO(logger, "example over jumps {} {}", std::string("str1"), "example3");
  LOG_INFO(logger, "quick brown test {} {} {} {} {} {} {} {} {} {}", std::string_view("view1"),
           std::string("str1"), 6LL, 8ULL, "example1", std::string_view("view2"), "example3", 2, 7UL, 5L);
  LOG_INFO(logger, "dog lazy quick over {} {} {} {} {} {} {} {}", 5L, 8ULL, true, false,
           std::string("str1"), std::string_view("view2"), "example1", static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps test dog {} {} {}", "example1", 4.0f, static_cast<unsigned short>(10));
  LOG_INFO(logger, "example test lazy {} {} {} {} {} {} {} {} {}", "example1", true, std::string_view("view2"),
           std::string("str1"), 7UL, 6LL, 8ULL, static_cast<unsigned short>(10), "example2");
  LOG_INFO(logger, "fox brown quick over {} {} {} {}", 6LL, std::string_view("view1"), "example3", 4.0f);
  LOG_INFO(logger, "dog jumps test brown {}", true);
  LOG_INFO(logger, "quick dog test {} {} {} {}", static_cast<short>(9), 3.0, 2,
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "lazy over brown {} {} {} {} {} {} {} {} {} {}", std::string("str2"), 8ULL, 6LL,
           7UL, 4.0f, false, 5L, 3.0, std::string_view("view2"), "example1");
  LOG_INFO(logger, "quick jumps fox {} {} {} {}", static_cast<short>(9), 2, 8ULL, std::string_view("view1"));
  LOG_INFO(logger, "quick lazy test brown {} {} {} {} {}", 6LL, false, "example2", 2, 1);
  LOG_INFO(logger, "test lazy logging dog {} {} {} {}", 2, static_cast<short>(9),
           std::string_view("view2"), false);
  LOG_INFO(logger, "quick lazy logging {} {} {} {} {} {} {} {} {}", std::string_view("view1"),
           static_cast<short>(9), 2, std::string("str2"), 7UL, 3.0, std::string_view("view2"), 4.0f, 6LL);
  LOG_INFO(logger, "over lazy fox test {} {} {} {} {} {} {} {}", "example2", static_cast<unsigned short>(10),
           std::string("str1"), "example3", 1, 2, true, std::string_view("view2"));
  LOG_INFO(logger, "brown logging over {} {} {} {}", 1, false, 8ULL, true);
  LOG_INFO(logger, "dog brown jumps quick {} {} {} {} {}", std::string_view("view1"), true, 7UL,
           std::string_view("view2"), 5L);
  LOG_INFO(logger, "jumps dog fox logging {} {} {} {} {} {} {} {}", "example1", 6LL, 1,
           static_cast<short>(9), 3.0, 2, std::string("str2"), 7UL);
  LOG_INFO(logger, "logging lazy dog test {} {} {} {}", 5L, "example3", std::string_view("view1"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "over fox test {} {} {} {}", 4.0f, false, "example1", true);
  LOG_INFO(logger, "quick logging jumps over {} {} {} {} {} {} {} {}", false, true,
           std::string_view("view1"), 2, "example3", 3.0, "example2", std::string("str1"));
  LOG_INFO(logger, "test brown fox {} {}", std::string_view("view2"), 7UL);
  LOG_INFO(logger, "example lazy dog {} {} {} {} {} {} {} {}", "example2", 4.0f,
           std::string("str2"), std::string("str1"), 8ULL, 7UL, std::string_view("view2"), 2);
  LOG_INFO(logger, "example logging fox test {} {} {} {} {} {} {} {}", true, static_cast<short>(9),
           2, 7UL, 3.0, "example2", std::string("str2"), std::string("str1"));
  LOG_INFO(logger, "brown jumps logging test {} {} {}", 6LL, true, std::string_view("view1"));
  LOG_INFO(logger, "fox lazy jumps {} {} {} {}", "example3", std::string("str1"), true, 3.0);
  LOG_INFO(logger, "fox test over quick {} {} {} {} {} {} {} {}", 8ULL, 4.0f, std::string("str1"),
           3.0, 7UL, std::string_view("view2"), "example2", static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown dog over logging {} {} {} {} {} {} {}", 4.0f, 7UL, "example3",
           std::string("str2"), 3.0, 8ULL, "example2");
  LOG_INFO(logger, "lazy dog brown logging {} {}", 4.0f, 8ULL);
  LOG_INFO(logger, "quick over jumps lazy {} {} {} {} {} {} {} {} {} {}", std::string("str2"),
           "example2", 7UL, std::string("str1"), std::string_view("view2"), "example3",
           static_cast<short>(9), 2, "example1", true);
  LOG_INFO(logger, "jumps logging lazy over {} {} {} {} {}", true, std::string_view("view2"), 3.0,
           "example3", static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown quick example {} {} {} {}", true, "example1", std::string("str1"), 2);
  LOG_INFO(logger, "over fox quick lazy {} {} {}", 5L, 3.0, std::string_view("view1"));
  LOG_INFO(logger, "fox logging example {}", "example2");
  LOG_INFO(logger, "over dog test {} {} {} {} {} {} {}", 4.0f, static_cast<unsigned short>(10), 1,
           7UL, true, false, 6LL);
  LOG_INFO(logger, "quick logging lazy dog {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           true, "example3", 5L, 8ULL, 1, static_cast<short>(9), std::string("str1"), "example2", 6LL);
  LOG_INFO(logger, "brown jumps lazy test {} {} {} {} {} {} {}", 8ULL, "example3", 6LL,
           std::string_view("view2"), std::string("str2"), "example2", false);
  LOG_INFO(logger, "quick example test lazy {} {} {} {} {}", true, false, "example2", 6LL,
           "example3");
  LOG_INFO(logger, "brown jumps logging {} {} {} {} {} {}", static_cast<unsigned short>(10), 3.0,
           8ULL, "example3", true, 4.0f);
  LOG_INFO(logger, "fox jumps logging {}", "example3");
  LOG_INFO(logger, "jumps test example {} {}", std::string("str2"), 1);
  LOG_INFO(logger, "quick example jumps lazy {} {} {} {} {} {} {} {} {} {}", 5L, 6LL, static_cast<short>(9),
           4.0f, "example3", "example2", 1, std::string("str2"), std::string_view("view2"), false);
  LOG_INFO(logger, "fox quick over logging {} {} {} {} {} {} {} {} {}", 1, std::string_view("view2"),
           8ULL, false, 3.0, std::string_view("view1"), "example1", true, 7UL);
  LOG_INFO(logger, "brown logging test fox {} {} {} {} {} {} {} {}", std::string("str1"), 5L,
           static_cast<short>(9), 7UL, 6LL, std::string_view("view2"), "example2", 8ULL);
  LOG_INFO(logger, "brown example quick test {} {} {}", std::string_view("view2"), "example2",
           std::string_view("view1"));
  LOG_INFO(logger, "logging example test brown {} {} {} {} {} {}", std::string_view("view2"),
           "example3", std::string("str2"), 8ULL, 3.0, std::string_view("view1"));
  LOG_INFO(logger, "over quick fox dog {} {} {} {} {} {}", std::string("str2"), 5L, 6LL, "example3",
           8ULL, std::string_view("view2"));
  LOG_INFO(logger, "test example brown {} {} {} {} {} {} {} {} {} {}", "example1", 7UL, 6LL,
           "example2", 2, std::string("str1"), 1, false, 4.0f, 5L);
  LOG_INFO(logger, "over test logging {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10), 5L,
           "example3", static_cast<short>(9), 6LL, std::string_view("view1"), 2, "example1", 3.0, 7UL);
  LOG_INFO(logger, "logging over dog {} {}", std::string_view("view1"), 6LL);
  LOG_INFO(logger, "jumps test example over {} {} {}", 5L, 7UL, "example1");
  LOG_INFO(logger, "fox test over {}", 5L);
  LOG_INFO(logger, "quick jumps over brown {} {} {} {} {} {} {} {} {}", std::string("str2"),
           std::string_view("view2"), 1, static_cast<short>(9), std::string_view("view1"), true,
           false, 8ULL, static_cast<unsigned short>(10));
  LOG_INFO(logger, "over test jumps example {} {} {} {} {} {}", std::string("str2"), 1,
           std::string_view("view1"), "example2", std::string_view("view2"), 6LL);
  LOG_INFO(logger, "fox brown test lazy {} {} {} {}", static_cast<unsigned short>(10), 2,
           std::string("str1"), "example3");
  LOG_INFO(logger, "brown test jumps lazy {} {} {}", 7UL, std::string("str2"), 1);
  LOG_INFO(logger, "over jumps brown dog {} {} {}", 7UL, std::string("str2"), static_cast<short>(9));
  LOG_INFO(logger, "brown test example {} {} {} {} {} {} {}", 5L, static_cast<unsigned short>(10),
           "example3", 7UL, "example2", std::string_view("view1"), 6LL);
  LOG_INFO(logger, "test brown dog {} {} {} {} {} {} {}", 1, 2, 6LL, 8ULL, true, "example1", 5L);
  LOG_INFO(logger, "lazy fox over {} {} {} {} {} {} {} {} {}", std::string_view("view1"), true,
           std::string("str2"), 4.0f, 3.0, "example2", 1, std::string_view("view2"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog over jumps {} {}", "example2", 3.0);
  LOG_INFO(logger, "test quick jumps dog {} {} {} {} {} {}", std::string("str1"),
           std::string("str2"), 5L, std::string_view("view2"), 3.0, 4.0f);
  LOG_INFO(logger, "logging over example {} {} {} {} {}", static_cast<unsigned short>(10), 4.0f, 5L,
           2, "example2");
  LOG_INFO(logger, "dog example jumps {} {} {}", "example1", static_cast<unsigned short>(10), 8ULL);
  LOG_INFO(logger, "example dog fox logging {} {} {} {}", 5L, std::string("str2"), "example3",
           "example2");
  LOG_INFO(logger, "fox dog test lazy {} {} {} {} {} {} {}", "example3", std::string("str1"),
           "example2", 1, "example1", std::string_view("view2"), false);
  LOG_INFO(logger, "fox logging lazy quick {}", 2);
  LOG_INFO(logger, "test quick over {}", std::string_view("view2"));
  LOG_INFO(logger, "example logging jumps dog {} {}", std::string("str1"), std::string_view("view2"));
  LOG_INFO(logger, "over lazy logging {} {} {} {} {} {}", 7UL, 6LL, static_cast<short>(9),
           "example1", std::string_view("view1"), "example3");
  LOG_INFO(logger, "logging example dog {} {} {} {} {} {} {} {} {} {}", 3.0, static_cast<unsigned short>(10),
           5L, "example3", 2, static_cast<short>(9), false, std::string("str1"), 6LL, 4.0f);
  LOG_INFO(logger, "example quick lazy {} {}", static_cast<unsigned short>(10), false);
  LOG_INFO(logger, "example fox over logging {} {} {}", "example2", static_cast<short>(9),
           "example1");
  LOG_INFO(logger, "logging jumps brown {} {} {} {} {} {} {} {} {} {}", 5L, 2, 6LL, 4.0f,
           "example2", std::string_view("view2"), 3.0, static_cast<unsigned short>(10), "example3",
           std::string_view("view1"));
  LOG_INFO(logger, "quick logging example fox {}", 2);
  LOG_INFO(logger, "example over quick {} {} {} {} {} {} {}", 6LL, 1, 5L, 2, 4.0f, 8ULL, "example3");
  LOG_INFO(logger, "quick lazy test example {} {} {} {} {} {} {}", 4.0f, std::string("str2"), 2,
           7UL, false, "example2", 1);
  LOG_INFO(logger, "dog jumps fox lazy {} {} {} {} {} {}", true, 3.0, std::string("str2"), false,
           std::string("str1"), 5L);
  LOG_INFO(logger, "brown quick dog jumps {} {} {}", 6LL, 4.0f, false);
  LOG_INFO(logger, "test lazy over brown {} {} {}", std::string("str2"), static_cast<short>(9),
           "example2");
  LOG_INFO(logger, "fox example quick jumps {} {} {} {}", 6LL, std::string("str2"), 2, 8ULL);
  LOG_INFO(logger, "logging brown dog over {} {} {} {}", 4.0f, 2, std::string_view("view2"), 6LL);
  LOG_INFO(logger, "over fox brown lazy {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           4.0f, true, 7UL, 5L, "example2", std::string_view("view1"), std::string("str1"));
  LOG_INFO(logger, "fox quick lazy {} {} {} {} {} {} {} {} {}", "example1",
           std::string_view("view2"), true, 7UL, 2, std::string_view("view1"),
           static_cast<short>(9), std::string("str1"), std::string("str2"));
  LOG_INFO(logger, "test brown example fox {} {} {} {} {} {} {} {} {} {}", 1, static_cast<short>(9),
           false, true, 5L, std::string("str2"), "example3", 8ULL, 6LL, 7UL);
  LOG_INFO(logger, "dog lazy logging example {} {} {} {} {} {}", 5L,
           static_cast<unsigned short>(10), 3.0, 1, true, 8ULL);
  LOG_INFO(logger, "logging quick fox test {} {} {} {} {} {} {} {} {}", true, false, std::string("str1"), 7UL,
           static_cast<short>(9), static_cast<unsigned short>(10), 5L, "example3", std::string("str2"));
  LOG_INFO(logger, "test quick over brown {} {}", false, "example3");
  LOG_INFO(logger, "lazy quick over test {} {} {} {} {} {} {}", std::string("str1"),
           static_cast<short>(9), std::string("str2"), std::string_view("view1"), 8ULL, 7UL, 6LL);
  LOG_INFO(logger, "fox brown example {} {} {} {} {} {} {}", 7UL, static_cast<unsigned short>(10),
           6LL, true, false, "example2", std::string("str2"));
  LOG_INFO(logger, "brown logging quick {} {} {} {} {} {} {} {} {} {}", "example3", 4.0f, 5L, true,
           "example1", "example2", static_cast<short>(9), 2, 6LL, 1);
  LOG_INFO(logger, "test dog jumps {} {}", "example2", 4.0f);
  LOG_INFO(logger, "brown test dog {} {}", 5L, 3.0);
  LOG_INFO(logger, "dog jumps test {} {} {} {} {}", 8ULL, std::string("str1"), 6LL, 1, false);
  LOG_INFO(logger, "example over jumps {} {} {} {} {} {} {} {} {} {}", std::string_view("view1"),
           3.0, 2, "example1", 5L, 6LL, std::string_view("view2"), 4.0f, static_cast<short>(9), 8ULL);
  LOG_INFO(logger, "quick logging brown jumps {} {} {} {} {}", 7UL, 1,
           static_cast<unsigned short>(10), true, "example3");
  LOG_INFO(logger, "test dog fox brown {} {} {} {} {} {} {} {}", std::string_view("view2"), 6LL,
           7UL, std::string("str1"), static_cast<short>(9), true, std::string_view("view1"), false);
  LOG_INFO(logger, "over lazy brown {} {} {} {} {} {} {}", "example3", 8ULL, std::string_view("view1"),
           false, std::string("str1"), std::string_view("view2"), "example2");
  LOG_INFO(logger, "dog over logging lazy {} {} {} {} {} {} {} {}", "example1", 4.0f, "example3",
           true, 8ULL, std::string("str2"), std::string_view("view1"), 3.0);
  LOG_INFO(logger, "logging over dog quick {} {} {} {} {} {}", 5L, std::string("str2"), 7UL,
           "example3", std::string("str1"), 4.0f);
  LOG_INFO(logger, "dog example test {} {} {} {}", static_cast<unsigned short>(10),
           std::string("str1"), "example2", std::string_view("view2"));
  LOG_INFO(logger, "quick example jumps {} {} {} {} {}", 1, 7UL, 3.0, "example3", 6LL);
  LOG_INFO(logger, "quick brown dog {} {} {} {} {} {} {} {} {}", std::string_view("view2"), false,
           6LL, static_cast<unsigned short>(10), 5L, "example2", 8ULL, 7UL, true);
  LOG_INFO(logger, "example brown test quick {} {} {}", 3.0, 2, static_cast<unsigned short>(10));
  LOG_INFO(logger, "test over fox {} {} {} {}", "example3", "example1", true, static_cast<short>(9));
  LOG_INFO(logger, "logging quick example {} {} {} {} {}", static_cast<short>(9), 5L,
           std::string_view("view1"), "example1", 2);
  LOG_INFO(logger, "example brown over {} {} {} {} {} {}", std::string_view("view2"),
           std::string("str2"), 2, 7UL, std::string_view("view1"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog test lazy fox {} {} {} {} {} {} {}", std::string("str2"), 8ULL, 6LL, 2,
           4.0f, "example3", false);
  LOG_INFO(logger, "test brown dog {}", false);
  LOG_INFO(logger, "jumps example lazy {}", 4.0f);
  LOG_INFO(logger, "over lazy jumps logging {} {} {} {} {} {} {} {} {} {}", 8ULL,
           static_cast<unsigned short>(10), 3.0, std::string_view("view2"), false, true, 5L, 4.0f,
           static_cast<short>(9), "example1");
  LOG_INFO(logger, "fox lazy brown jumps {} {} {} {} {} {} {}", std::string("str2"),
           static_cast<unsigned short>(10), 5L, 1, "example2", 4.0f, false);
  LOG_INFO(logger, "example brown logging jumps {} {} {} {} {} {} {} {}", 6LL, std::string_view("view1"),
           2, false, static_cast<short>(9), static_cast<unsigned short>(10), "example1", 4.0f);
  LOG_INFO(logger, "fox test brown {} {} {} {} {}", static_cast<short>(9), 3.0, 8ULL, 5L, 4.0f);
  LOG_INFO(logger, "over logging dog test {} {} {}", std::string_view("view1"), 8ULL,
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown dog jumps test {} {} {}", 3.0, "example2", 5L);
  LOG_INFO(logger, "jumps example lazy {} {}", "example1", false);
  LOG_INFO(logger, "lazy dog test {} {} {} {}", std::string("str2"),
           static_cast<unsigned short>(10), false, static_cast<short>(9));
  LOG_INFO(logger, "test fox lazy logging {} {}", static_cast<unsigned short>(10), 8ULL);
  LOG_INFO(logger, "logging jumps fox {} {} {}", true, 3.0, 5L);
  LOG_INFO(logger, "quick example lazy brown {} {} {} {} {} {} {} {} {} {}", "example1", true, 3.0, 7UL,
           8ULL, std::string("str2"), std::string("str1"), 4.0f, "example2", std::string_view("view2"));
  LOG_INFO(logger, "brown example over lazy {} {} {} {} {} {} {}", std::string_view("view1"), 3.0,
           4.0f, "example3", std::string("str2"), 8ULL, 6LL);
  LOG_INFO(logger, "example test brown quick {} {} {} {} {} {}", "example2", 7UL, 4.0f,
           static_cast<short>(9), "example1", 2);
  LOG_INFO(logger, "over logging example jumps {} {} {} {} {} {}", static_cast<unsigned short>(10),
           6LL, 4.0f, 7UL, 3.0, true);
  LOG_INFO(logger, "example fox lazy test {} {} {} {} {} {}", static_cast<short>(9), 6LL, 7UL, 1, 4.0f, 8ULL);
  LOG_INFO(logger, "brown lazy quick {} {} {} {} {} {} {}", static_cast<short>(9), 6LL, true,
           static_cast<unsigned short>(10), "example2", false, 5L);
  LOG_INFO(logger, "quick example lazy {}", std::string_view("view2"));
  LOG_INFO(logger, "fox example over {} {} {}", static_cast<unsigned short>(10), std::string_view("view2"), 5L);
  LOG_INFO(logger, "dog over fox {} {} {} {}", 1, std::string_view("view2"), 2, std::string("str2"));
  LOG_INFO(logger, "test jumps quick dog {} {} {} {} {} {} {}", std::string("str1"), 1, "example3",
           4.0f, static_cast<unsigned short>(10), 8ULL, "example2");
  LOG_INFO(logger, "example jumps lazy {} {} {} {} {}", std::string("str2"), "example2",
           static_cast<short>(9), 6LL, 8ULL);
  LOG_INFO(logger, "quick jumps lazy example {} {} {} {} {} {} {}", 4.0f, 7UL, false,
           static_cast<unsigned short>(10), "example2", static_cast<short>(9), 1);
  LOG_INFO(logger, "dog quick brown {} {} {} {} {}", false, 4.0f, "example3", 7UL, true);
  LOG_INFO(logger, "quick brown jumps {} {} {} {} {} {} {}", 2, 3.0, "example3", 1, 8ULL,
           static_cast<unsigned short>(10), std::string("str1"));
  LOG_INFO(logger, "test lazy dog quick {} {} {} {} {}", std::string("str2"), static_cast<short>(9),
           8ULL, false, static_cast<unsigned short>(10));
  LOG_INFO(logger, "over quick lazy fox {} {}", "example1", std::string_view("view1"));
  LOG_INFO(logger, "quick over lazy {} {} {} {} {} {}", false, static_cast<short>(9), 2, 1, 3.0,
           std::string("str1"));
  LOG_INFO(logger, "fox lazy logging {} {} {} {} {} {} {}", 4.0f, "example3", true, false,
           std::string_view("view2"), static_cast<short>(9), std::string("str1"));
  LOG_INFO(logger, "example jumps dog fox {} {} {} {}", std::string_view("view1"),
           static_cast<unsigned short>(10), std::string("str1"), 8ULL);
  LOG_INFO(logger, "dog lazy test logging {} {} {} {} {} {} {} {}", std::string_view("view2"), 8ULL,
           2, std::string("str1"), static_cast<short>(9), "example3", std::string("str2"), true);
  LOG_INFO(logger, "logging dog jumps {} {} {} {} {} {} {} {} {} {}", "example2", 1, false, 6LL,
           3.0, static_cast<unsigned short>(10), std::string_view("view2"), 4.0f, true, "example3");
  LOG_INFO(logger, "example dog logging quick {} {} {} {} {} {}", false, "example3", 4.0f,
           std::string("str1"), 7UL, 5L);
  LOG_INFO(logger, "test fox quick brown {}", std::string_view("view2"));
  LOG_INFO(logger, "example quick logging {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string("str1"), 2, "example2", std::string("str2"), std::string_view("view2"),
           "example1");
  LOG_INFO(logger, "jumps fox quick test {} {} {} {} {} {} {} {}", 4.0f, 5L, "example2", 3.0,
           static_cast<short>(9), 1, 8ULL, std::string_view("view1"));
  LOG_INFO(logger, "logging lazy jumps quick {} {} {} {} {}", 7UL, "example2",
           std::string_view("view2"), 4.0f, 3.0);
  LOG_INFO(logger, "test logging lazy over {} {} {} {} {} {} {} {}", "example1",
           std::string_view("view2"), 6LL, 5L, std::string("str1"), 8ULL, 3.0, static_cast<short>(9));
  LOG_INFO(logger, "logging test fox {}", 8ULL);
  LOG_INFO(logger, "dog lazy jumps {} {} {} {} {} {} {}", 3.0, 7UL, "example2", 1,
           std::string_view("view2"), 6LL, std::string_view("view1"));
  LOG_INFO(logger, "dog over fox jumps {} {} {} {} {} {} {} {}", static_cast<short>(9),
           std::string_view("view2"), 2, "example2", 6LL, 8ULL, std::string("str2"),
           std::string_view("view1"));
  LOG_INFO(logger, "logging quick lazy {} {} {} {} {}", 8ULL, std::string("str1"), 4.0f,
           std::string("str2"), "example2");
  LOG_INFO(logger, "dog test fox {} {} {} {} {} {} {} {} {}", "example1", std::string("str2"), 1,
           3.0, std::string_view("view2"), 7UL, false, 8ULL, 2);
  LOG_INFO(logger, "over dog quick jumps {} {} {} {} {} {} {} {} {}", std::string("str2"), 8ULL,
           "example3", 3.0, false, true, 2, std::string_view("view2"), static_cast<short>(9));
  LOG_INFO(logger, "quick logging brown {} {} {} {} {}", "example3",
           static_cast<unsigned short>(10), 8ULL, "example1", false);
  LOG_INFO(logger, "lazy example logging fox {} {} {} {} {} {} {}", 7UL, 6LL, 3.0, false, 5L,
           std::string("str2"), true);
  LOG_INFO(logger, "fox over brown lazy {} {} {} {} {}", 3.0, 6LL, true, std::string_view("view2"),
           "example3");
  LOG_INFO(logger, "test fox quick {} {} {} {} {} {} {} {} {}", "example2", 4.0f, std::string("str2"),
           static_cast<unsigned short>(10), 6LL, 5L, static_cast<short>(9), false, true);
  LOG_INFO(logger, "brown quick lazy dog {} {} {} {} {} {}", static_cast<short>(9), "example1", 2,
           4.0f, 8ULL, 7UL);
  LOG_INFO(logger, "example dog logging {} {}", true, "example1");
  LOG_INFO(logger, "logging example jumps over {} {} {} {}", 4.0f, static_cast<unsigned short>(10),
           "example2", static_cast<short>(9));
  LOG_INFO(logger, "lazy brown example test {} {} {} {} {} {}", true, 1, static_cast<short>(9),
           8ULL, 3.0, std::string("str1"));
  LOG_INFO(logger, "test example jumps {} {} {} {} {}", "example3", 4.0f, true, std::string_view("view1"), 6LL);
  LOG_INFO(logger, "fox quick over {} {} {} {} {} {} {}", 3.0, std::string_view("view1"), 2, 8ULL,
           7UL, true, "example3");
  LOG_INFO(logger, "test logging jumps brown {}", 1);
  LOG_INFO(logger, "jumps lazy logging quick {}", "example2");
  LOG_INFO(logger, "lazy quick dog logging {} {} {} {} {} {}", "example3", 5L, "example1", 3.0,
           std::string("str2"), std::string("str1"));
  LOG_INFO(logger, "example test logging {} {}", std::string_view("view2"), "example1");
  LOG_INFO(logger, "brown logging dog fox {} {} {} {} {} {} {} {} {} {}", 1, static_cast<short>(9),
           "example2", 7UL, 8ULL, std::string("str2"), "example3", 5L, 2, 6LL);
  LOG_INFO(logger, "logging lazy dog {} {} {}", std::string("str1"), "example2", "example1");
  LOG_INFO(logger, "test lazy fox brown {} {} {}", static_cast<short>(9), std::string_view("view1"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "over test example fox {}", std::string_view("view2"));
  LOG_INFO(logger, "fox test lazy {}", std::string("str2"));
  LOG_INFO(logger, "over test dog {} {} {} {}", std::string("str2"), 7UL, std::string("str1"), 8ULL);
  LOG_INFO(logger, "fox example brown {} {} {} {} {} {} {} {} {}", true, std::string("str1"), 1,
           std::string_view("view1"), "example2", "example1", std::string_view("view2"), 6LL, 7UL);
  LOG_INFO(logger, "test jumps fox example {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           "example3", static_cast<short>(9), 5L, std::string("str2"), 8ULL, true, 7UL);
  LOG_INFO(logger, "fox jumps over {} {} {} {} {} {} {} {}", 5L, std::string_view("view1"), 4.0f,
           8ULL, "example2", std::string("str1"), "example3", static_cast<short>(9));
  LOG_INFO(logger, "jumps fox logging lazy {} {} {} {} {} {} {} {}", "example2", 3.0, 5L,
           std::string("str2"), 2, 6LL, 8ULL, 1);
  LOG_INFO(logger, "quick lazy dog example {}", static_cast<short>(9));
  LOG_INFO(logger, "jumps example lazy {} {} {} {} {} {} {} {} {}", 1, 3.0, 4.0f, true, 5L, 8ULL,
           "example3", std::string_view("view1"), 7UL);
  LOG_INFO(logger, "over test quick {} {} {}", static_cast<short>(9), 2, 5L);
  LOG_INFO(logger, "jumps fox lazy {} {} {} {} {}", "example2", 3.0, "example3",
           std::string_view("view1"), std::string_view("view2"));
  LOG_INFO(logger, "jumps dog test {} {} {} {} {} {} {}", false, 7UL, std::string("str2"), 8ULL,
           std::string_view("view2"), 6LL, std::string("str1"));
  LOG_INFO(logger, "fox quick brown {} {} {} {}", 7UL, "example2", std::string_view("view1"), 5L);
  LOG_INFO(logger, "over quick jumps {} {} {} {} {} {} {} {} {}", 4.0f, 6LL, static_cast<short>(9),
           3.0, "example2", std::string("str1"), "example3", "example1", std::string_view("view1"));
  LOG_INFO(logger, "lazy jumps over fox {} {} {} {} {} {} {} {} {}", std::string_view("view2"), 1,
           3.0, static_cast<unsigned short>(10), std::string("str2"), "example2",
           static_cast<short>(9), true, 6LL);
  LOG_INFO(logger, "jumps dog brown example {} {} {} {} {} {}", 8ULL, 6LL, std::string("str2"),
           "example2", false, 4.0f);
  LOG_INFO(logger, "test fox jumps logging {} {} {} {} {} {} {} {} {} {}", false, "example2", 4.0f,
           std::string("str1"), "example3", true, std::string_view("view2"), 1, std::string("str2"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging test example over {}", false);
  LOG_INFO(logger, "jumps brown dog {}", 5L);
  LOG_INFO(logger, "dog lazy brown quick {} {} {} {} {} {} {} {} {} {}", std::string("str2"),
           std::string("str1"), "example3", 8ULL, std::string_view("view1"), "example1", "example2",
           false, 6LL, static_cast<short>(9));
  LOG_INFO(logger, "lazy fox brown {} {} {}", std::string_view("view2"), static_cast<unsigned short>(10), 5L);
  LOG_INFO(logger, "jumps brown lazy {} {} {} {} {} {} {}", std::string_view("view1"),
           static_cast<short>(9), 1, 2, std::string("str1"), 4.0f, static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps lazy fox {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           3.0, 2, std::string("str2"), 6LL, "example2", "example3", true, 4.0f, std::string("str1"));
  LOG_INFO(logger, "test logging dog {} {} {} {} {} {}", false, std::string_view("view2"), 3.0, 6LL,
           "example1", 2);
  LOG_INFO(logger, "dog logging example fox {} {} {} {} {} {} {} {} {}", false, static_cast<short>(9),
           std::string("str1"), static_cast<unsigned short>(10), 6LL, true, 1, 3.0, std::string("str2"));
  LOG_INFO(logger, "fox test lazy {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string("str2"), 2, true, 3.0, static_cast<short>(9), std::string_view("view2"));
  LOG_INFO(logger, "dog test over {} {} {}", std::string_view("view1"), 2, static_cast<short>(9));
  LOG_INFO(logger, "fox example test {} {} {} {} {} {} {}", 5L, std::string_view("view1"),
           "example3", 8ULL, std::string("str1"), true, 7UL);
  LOG_INFO(logger, "dog over quick {} {} {} {}", "example3", 2, std::string_view("view1"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "lazy example over {} {}", 4.0f, true);
  LOG_INFO(logger, "over brown jumps fox {} {} {} {} {} {} {} {}", "example1", std::string_view("view2"),
           2, static_cast<short>(9), std::string("str2"), 5L, std::string("str1"), 1);
  LOG_INFO(logger, "over dog fox quick {} {} {} {} {} {}", static_cast<short>(9), false,
           std::string("str2"), 8ULL, "example2", "example3");
  LOG_INFO(logger, "fox quick jumps example {} {} {} {} {}", 2, "example3", static_cast<short>(9),
           std::string_view("view2"), std::string("str1"));
  LOG_INFO(logger, "test fox logging {} {} {} {} {} {}", "example2", 4.0f,
           static_cast<unsigned short>(10), 6LL, "example3", std::string("str1"));
  LOG_INFO(logger, "quick logging lazy {} {} {} {} {} {} {} {} {}", 1, 8ULL, 4.0f, 6LL,
           std::string("str2"), std::string_view("view1"), 2, true, false);
  LOG_INFO(logger, "example lazy quick fox {} {} {} {} {} {} {} {} {} {}", false, 7UL, 3.0, true,
           6LL, 5L, 8ULL, 1, "example1", std::string_view("view1"));
  LOG_INFO(logger, "jumps example over {} {} {} {} {}", true, 2, std::string("str1"),
           std::string_view("view2"), 3.0);
  LOG_INFO(logger, "test dog jumps fox {} {} {} {} {}", "example2", "example1", 4.0f, 5L, 3.0);
  LOG_INFO(logger, "brown over jumps {}", 8ULL);
  LOG_INFO(logger, "test jumps quick {} {} {}", 1, 5L, "example1");
  LOG_INFO(logger, "example test jumps brown {} {} {}", static_cast<unsigned short>(10), 5L,
           "example2");
  LOG_INFO(logger, "dog brown test lazy {} {} {} {} {} {} {}", std::string("str2"), 7UL, "example2",
           5L, 2, false, "example3");
  LOG_INFO(logger, "example fox dog over {} {} {} {} {} {} {} {}", true, 3.0, 8ULL, "example3", 1,
           std::string("str2"), static_cast<short>(9), 7UL);
  LOG_INFO(logger, "over dog jumps {} {}", std::string_view("view2"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "lazy quick jumps {} {} {} {} {} {} {}", static_cast<short>(9), 1, 2, false,
           4.0f, std::string_view("view1"), 3.0);
  LOG_INFO(logger, "over quick test logging {} {} {} {} {} {} {} {} {}", 2, std::string("str1"),
           7UL, 5L, "example2", true, "example1", std::string("str2"), 4.0f);
  LOG_INFO(logger, "over test logging {} {} {} {} {} {} {} {} {}", 5L, 4.0f, std::string_view("view1"),
           std::string_view("view2"), 3.0, std::string("str1"), "example3", 8ULL, 1);
  LOG_INFO(logger, "test lazy logging dog {} {}", 7UL, "example3");
  LOG_INFO(logger, "example fox lazy logging {} {} {} {} {} {} {} {} {}", 1, 8ULL, 3.0,
           std::string_view("view2"), "example3", std::string_view("view1"), true,
           static_cast<unsigned short>(10), "example2");
  LOG_INFO(logger, "dog over test {} {} {}", 1, 8ULL, std::string_view("view1"));
  LOG_INFO(logger, "dog logging test brown {} {} {}", std::string_view("view1"), 5L,
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox quick over example {} {} {}", static_cast<unsigned short>(10), "example3", true);
  LOG_INFO(logger, "lazy jumps example {} {} {}", 8ULL, 3.0, true);
  LOG_INFO(logger, "quick logging test {} {} {} {} {} {} {} {}", 3.0, std::string_view("view1"), 1,
           true, std::string_view("view2"), static_cast<short>(9), std::string("str2"), 4.0f);
  LOG_INFO(logger, "quick dog fox example {}", 5L);
  LOG_INFO(logger, "lazy fox brown {} {} {} {} {} {}", true, "example1", 3.0, "example3", 7UL,
           std::string("str1"));
  LOG_INFO(logger, "fox jumps quick test {} {} {} {} {}", std::string("str2"),
           std::string_view("view2"), static_cast<short>(9), "example2", 6LL);
  LOG_INFO(logger, "brown logging fox {} {} {} {} {} {} {} {} {} {}", 2, std::string_view("view2"),
           "example2", std::string("str1"), "example1", 7UL, 6LL, false, 3.0, "example3");
  LOG_INFO(logger, "dog lazy over logging {} {} {} {}", std::string("str2"), 1,
           std::string_view("view1"), std::string_view("view2"));
  LOG_INFO(logger, "dog test over {} {} {} {}", std::string_view("view2"), 5L, "example1",
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog example quick logging {} {} {}", 3.0, std::string_view("view1"), 8ULL);
  LOG_INFO(logger, "dog lazy test {} {} {} {} {} {} {}", true, static_cast<short>(9),
           std::string_view("view1"), 2, false, std::string("str2"), 5L);
  LOG_INFO(logger, "test jumps quick {} {} {}", std::string("str1"), 1, std::string_view("view1"));
  LOG_INFO(logger, "example lazy brown {} {} {} {} {} {} {}", std::string("str2"), std::string_view("view1"),
           2, static_cast<unsigned short>(10), 1, std::string_view("view2"), 8ULL);
  LOG_INFO(logger, "test example fox {} {} {} {} {} {} {}", "example1", "example3", true, 8ULL,
           static_cast<unsigned short>(10), 6LL, false);
  LOG_INFO(logger, "test brown over fox {} {} {} {} {} {}", "example1", std::string_view("view1"),
           8ULL, static_cast<short>(9), 6LL, false);
  LOG_INFO(logger, "jumps dog test logging {} {}", "example1", 4.0f);
  LOG_INFO(logger, "fox quick lazy jumps {} {} {}", false, 6LL, 1);
  LOG_INFO(logger, "fox brown test lazy {} {} {} {} {} {} {}", static_cast<short>(9),
           std::string_view("view2"), "example3", std::string("str2"), std::string_view("view1"), false, true);
  LOG_INFO(logger, "jumps fox brown lazy {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           2, "example1", true, static_cast<short>(9), std::string_view("view2"), "example3", 6LL);
  LOG_INFO(logger, "logging lazy jumps test {} {} {} {} {}", 7UL, std::string_view("view2"), 8ULL,
           "example1", static_cast<short>(9));
  LOG_INFO(logger, "jumps logging test {} {} {} {} {} {} {} {}", 2, false, 5L, 6LL, 7UL, 8ULL, 4.0f,
           std::string("str2"));
  LOG_INFO(logger, "fox quick brown test {} {} {} {} {} {}", "example2", 7UL,
           static_cast<unsigned short>(10), false, "example3", std::string_view("view2"));
  LOG_INFO(logger, "brown logging lazy {} {} {} {} {} {}", false, std::string_view("view1"),
           std::string("str1"), std::string_view("view2"), 3.0, "example1");
  LOG_INFO(logger, "test fox dog {} {} {}", std::string_view("view2"), 6LL, static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog over jumps {} {} {} {} {} {}", true, 3.0, "example1", "example3", 1, 2);
  LOG_INFO(logger, "fox example over {} {} {}", "example3", 8ULL, 6LL);
  LOG_INFO(logger, "logging test example {} {} {} {}", std::string_view("view1"), 7UL, 3.0, 2);
  LOG_INFO(logger, "jumps test dog {} {} {}", 1, 3.0, 2);
  LOG_INFO(logger, "brown test logging fox {} {} {} {} {} {} {} {}", 8ULL, 4.0f, 7UL,
           std::string("str2"), std::string_view("view1"), "example3", false, 6LL);
  LOG_INFO(logger, "quick dog over {} {} {} {} {} {} {} {}", true, 5L, 4.0f, "example1", false,
           "example2", static_cast<unsigned short>(10), 3.0);
  LOG_INFO(logger, "over quick logging {} {} {}", 6LL, static_cast<unsigned short>(10), 3.0);
  LOG_INFO(logger, "brown over lazy logging {} {} {} {} {} {}", "example2", "example1", 7UL, 4.0f,
           std::string_view("view2"), "example3");
  LOG_INFO(logger, "lazy quick jumps {} {} {} {} {} {} {}", 3.0, 5L, false, 6LL,
           std::string_view("view2"), "example1", static_cast<unsigned short>(10));
  LOG_INFO(logger, "over brown dog logging {} {}", "example3", "example1");
  LOG_INFO(logger, "lazy example jumps {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           "example1", 6LL, "example3", false, static_cast<short>(9), "example2", std::string("str2"), 8ULL);
  LOG_INFO(logger, "brown logging dog fox {} {} {}", std::string_view("view2"), 3.0, 2);
  LOG_INFO(logger, "over test brown {} {} {} {} {} {} {} {}", std::string("str2"), 7UL, false, true,
           1, 5L, std::string_view("view2"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox brown logging {} {} {} {} {} {} {} {}", 2, false, 6LL, 1, 4.0f,
           std::string_view("view1"), "example1", static_cast<short>(9));
  LOG_INFO(logger, "jumps dog quick {} {} {} {} {} {} {}", std::string("str1"), 7UL, true, 4.0f, 5L, 2, 3.0);
  LOG_INFO(logger, "example over lazy quick {} {} {} {} {} {} {}", 6LL, 8ULL, true,
           static_cast<unsigned short>(10), 4.0f, 2, std::string("str2"));
  LOG_INFO(logger, "brown test quick {} {} {} {} {}", 3.0, "example1", 7UL, static_cast<short>(9), 5L);
  LOG_INFO(logger, "test lazy jumps {} {} {} {} {} {} {} {} {} {}", 7UL, 1, std::string("str2"),
           std::string_view("view1"), "example3", "example2", static_cast<unsigned short>(10), 6LL,
           4.0f, std::string("str1"));
  LOG_INFO(logger, "logging over quick test {} {} {} {} {} {} {} {} {} {}", true, std::string("str1"),
           5L, 6LL, 3.0, std::string_view("view2"), 8ULL, static_cast<unsigned short>(10), false, 2);
  LOG_INFO(logger, "jumps brown logging {} {} {} {}", "example1", std::string("str1"),
           std::string_view("view1"), 7UL);
  LOG_INFO(logger, "test fox dog quick {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           6LL, std::string("str1"), 2, true, "example1", std::string_view("view2"), false, 3.0);
  LOG_INFO(logger, "fox dog logging {} {} {}", false, std::string("str1"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "lazy brown logging over {} {}", 7UL, std::string("str1"));
  LOG_INFO(logger, "quick brown lazy over {}", "example3");
  LOG_INFO(logger, "over logging test {} {} {} {} {} {}", std::string_view("view2"),
           static_cast<short>(9), 2, std::string("str1"), 7UL, true);
  LOG_INFO(logger, "over quick dog jumps {} {} {}", static_cast<short>(9), "example2",
           std::string_view("view1"));
  LOG_INFO(logger, "example test fox lazy {} {} {} {} {} {} {} {} {} {}", 6LL, 1, true,
           std::string_view("view1"), static_cast<short>(9), std::string_view("view2"),
           std::string("str1"), 8ULL, "example3", false);
  LOG_INFO(logger, "example fox quick over {} {} {} {} {} {} {} {} {}", 7UL, static_cast<short>(9),
           static_cast<unsigned short>(10), "example2", true, 5L, 4.0f, 2, 8ULL);
  LOG_INFO(logger, "quick lazy test brown {} {} {} {} {} {} {} {} {}", 8ULL, 6LL, 3.0,
           std::string("str1"), 2, 5L, "example3", false, 1);
  LOG_INFO(logger, "example dog jumps lazy {}", std::string_view("view2"));
  LOG_INFO(logger, "fox over quick {} {} {} {} {} {} {} {} {}", 7UL, static_cast<short>(9),
           std::string_view("view1"), std::string("str2"), static_cast<unsigned short>(10),
           "example1", 1, 8ULL, true);
  LOG_INFO(logger, "over jumps brown example {} {} {} {} {} {}", std::string_view("view1"), 2,
           "example1", 6LL, static_cast<unsigned short>(10), 5L);
  LOG_INFO(logger, "fox test over jumps {} {} {} {} {} {} {} {} {} {}", "example2", "example1", 7UL,
           "example3", 8ULL, std::string_view("view1"), 2, std::string("str2"), std::string("str1"), false);
  LOG_INFO(logger, "logging jumps lazy {} {} {} {} {}", std::string("str2"), "example3", 6LL,
           "example2", std::string("str1"));
  LOG_INFO(logger, "example jumps logging {}", 8ULL);
  LOG_INFO(logger, "logging test over {}", "example1");
  LOG_INFO(logger, "logging lazy test {} {} {} {} {} {} {} {} {}", "example2", 1,
           static_cast<unsigned short>(10), std::string_view("view2"), 3.0, static_cast<short>(9),
           7UL, 5L, 8ULL);
  LOG_INFO(logger, "logging dog test {} {} {}", std::string_view("view2"), 2, false);
  LOG_INFO(logger, "jumps example test {} {} {} {} {}", 1, true, 2, std::string("str2"), "example3");
  LOG_INFO(logger, "jumps example brown {}", "example2");
  LOG_INFO(logger, "logging fox quick lazy {}", static_cast<short>(9));
  LOG_INFO(logger, "logging over test jumps {} {} {} {}", std::string_view("view1"), 8ULL,
           std::string_view("view2"), std::string("str2"));
  LOG_INFO(logger, "quick test jumps {} {} {} {} {}", std::string("str2"), "example1", "example2", 8ULL, 1);
  LOG_INFO(logger, "test lazy dog brown {} {} {}", 2, static_cast<short>(9), 8ULL);
  LOG_INFO(logger, "test quick over {} {}", std::string("str2"), "example1");
  LOG_INFO(logger, "quick dog fox {} {} {}", 5L, 8ULL, "example3");
  LOG_INFO(logger, "test lazy fox {} {}", std::string("str1"), std::string("str2"));
  LOG_INFO(logger, "example brown dog {} {} {} {} {} {} {} {} {} {}", 4.0f, std::string("str2"),
           "example1", 1, 5L, "example2", std::string_view("view2"), static_cast<short>(9),
           std::string("str1"), 7UL);
  LOG_INFO(logger, "brown test example dog {} {} {} {} {}", "example3", false,
           std::string_view("view2"), std::string_view("view1"), 3.0);
  LOG_INFO(logger, "dog over lazy quick {} {} {} {} {} {} {} {} {} {}", 2, 6LL,
           static_cast<short>(9), std::string_view("view2"), std::string_view("view1"),
           std::string("str1"), true, "example2", false, static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown lazy example {} {} {}", static_cast<short>(9), true, 7UL);
  LOG_INFO(logger, "jumps quick dog {} {}", std::string("str1"), 3.0);
  LOG_INFO(logger, "brown test over quick {} {} {} {} {} {} {}", 3.0, true, std::string("str2"),
           std::string_view("view2"), 7UL, static_cast<short>(9), "example1");
  LOG_INFO(logger, "brown over example logging {} {} {} {} {}", 7UL, true, "example1",
           std::string_view("view2"), 8ULL);
  LOG_INFO(logger, "quick example test brown {} {} {} {} {} {} {} {} {} {}", std::string("str1"),
           std::string_view("view2"), 2, 5L, "example1", 1, 6LL, 8ULL, "example3", 7UL);
  LOG_INFO(logger, "lazy over logging {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10), 1,
           7UL, std::string_view("view1"), "example3", 8ULL, static_cast<short>(9), "example2", 3.0);
  LOG_INFO(logger, "test quick lazy {} {} {} {} {} {}", false, "example3", 6LL, 5L, "example1",
           std::string_view("view2"));
  LOG_INFO(logger, "dog logging fox {} {} {}", 4.0f, std::string("str1"), 2);
  LOG_INFO(logger, "lazy logging example jumps {} {} {}", static_cast<unsigned short>(10),
           std::string("str1"), 3.0);
  LOG_INFO(logger, "dog jumps test {} {} {} {} {} {}", "example2", std::string("str1"), 4.0f,
           static_cast<short>(9), "example1", std::string_view("view1"));
  LOG_INFO(logger, "example jumps logging over {} {} {} {}", 7UL, "example1", std::string("str2"),
           "example2");
  LOG_INFO(logger, "quick fox lazy {} {} {} {} {} {} {}", 1, 6LL, 3.0, 5L,
           static_cast<unsigned short>(10), 2, std::string("str2"));
  LOG_INFO(logger, "quick jumps test {} {} {} {} {} {} {} {} {}", std::string_view("view1"), 3.0,
           6LL, "example2", 7UL, true, std::string_view("view2"), static_cast<short>(9), false);
  LOG_INFO(logger, "jumps example quick lazy {} {}", false, std::string("str2"));
  LOG_INFO(logger, "test brown quick {} {} {}", std::string("str2"), std::string("str1"), 4.0f);
  LOG_INFO(logger, "lazy brown dog {} {} {} {} {} {}", std::string("str1"), 8ULL, "example2", 2, 7UL, false);
  LOG_INFO(logger, "fox over dog lazy {} {} {}", 5L, 3.0, std::string("str1"));
  LOG_INFO(logger, "over test quick jumps {} {} {} {} {} {}", std::string_view("view1"), "example3",
           std::string("str2"), 8ULL, std::string("str1"), std::string_view("view2"));
  LOG_INFO(logger, "quick test dog {} {} {} {} {} {} {} {} {} {}", 1,
           static_cast<unsigned short>(10), "example3", false, true, static_cast<short>(9), 6LL,
           std::string_view("view1"), "example1", 2);
  LOG_INFO(logger, "over brown test jumps {} {} {} {} {} {} {} {}", std::string_view("view2"), 7UL,
           std::string("str1"), static_cast<unsigned short>(10), 4.0f, std::string_view("view1"), false, true);
  LOG_INFO(logger, "test lazy jumps over {} {}", 1, static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown dog lazy {} {} {} {}", std::string_view("view1"),
           static_cast<unsigned short>(10), 7UL, true);
  LOG_INFO(logger, "logging example dog over {}", std::string("str1"));
  LOG_INFO(logger, "fox brown lazy logging {} {} {} {} {} {} {} {} {} {}", 2, 6LL, 4.0f, "example3",
           std::string_view("view1"), "example2", true, 1, false, static_cast<unsigned short>(10));
  LOG_INFO(logger, "over brown dog lazy {} {} {} {}", 6LL, 8ULL, std::string_view("view1"), 2);
  LOG_INFO(logger, "lazy logging test {} {}", 2, "example1");
  LOG_INFO(logger, "quick fox jumps test {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           true, std::string_view("view1"), false, "example1", std::string_view("view2"), 1);
  LOG_INFO(logger, "jumps test dog brown {} {} {} {} {} {} {}", 1, std::string("str2"), "example3",
           std::string_view("view1"), static_cast<unsigned short>(10), std::string("str1"), false);
  LOG_INFO(logger, "example quick logging {} {} {} {} {}", 5L, static_cast<short>(9), false, 2,
           "example1");
  LOG_INFO(logger, "dog example fox test {} {} {} {} {} {} {}", 3.0, "example3",
           std::string_view("view2"), false, 5L, 4.0f, "example2");
  LOG_INFO(logger, "dog lazy jumps {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           3.0, "example2", "example1", 1, 6LL, 5L, 8ULL, 2, std::string_view("view2"));
  LOG_INFO(logger, "fox over jumps quick {} {} {} {} {}", true, 3.0, static_cast<short>(9),
           std::string_view("view2"), 8ULL);
  LOG_INFO(logger, "lazy jumps logging {} {}", std::string_view("view2"), 1);
  LOG_INFO(logger, "quick logging over {} {} {} {} {} {}", 5L, std::string_view("view1"), 1,
           std::string_view("view2"), false, 2);
  LOG_INFO(logger, "logging over jumps example {}", std::string("str1"));
  LOG_INFO(logger, "jumps example fox {} {}", 3.0, "example1");
  LOG_INFO(logger, "brown dog lazy {} {} {} {} {} {} {}", 5L, false, 7UL, "example2",
           std::string_view("view1"), static_cast<short>(9), "example3");
  LOG_INFO(logger, "lazy example over test {} {} {} {} {} {} {} {}", std::string("str1"),
           std::string_view("view2"), "example1", std::string_view("view1"), "example3", 6LL,
           "example2", 2);
  LOG_INFO(logger, "quick logging fox brown {} {} {} {} {} {} {} {} {}", 3.0, 4.0f,
           static_cast<short>(9), 5L, false, 6LL, std::string("str1"), 1, std::string_view("view1"));
  LOG_INFO(logger, "over example quick fox {} {} {} {} {}", false, 7UL,
           static_cast<unsigned short>(10), "example1", static_cast<short>(9));
  LOG_INFO(logger, "jumps dog lazy {} {} {} {}", std::string("str1"), "example3", 1, std::string_view("view1"));
  LOG_INFO(logger, "brown dog quick {} {} {} {} {} {} {} {}", std::string("str2"), 1, static_cast<short>(9),
           "example3", static_cast<unsigned short>(10), 3.0, std::string_view("view2"), 4.0f);
  LOG_INFO(logger, "jumps lazy quick {} {}", false, 1);
  LOG_INFO(logger, "quick logging lazy {} {} {} {} {} {} {} {} {} {}", std::string("str1"), 8ULL,
           4.0f, 6LL, false, 1, static_cast<short>(9), true, std::string_view("view2"), "example1");
  LOG_INFO(logger, "brown example fox {} {} {} {}", "example2", 8ULL, 3.0, 6LL);
  LOG_INFO(logger, "dog over test {} {} {} {} {} {} {} {} {}", 5L, false, "example1", 1, 7UL,
           static_cast<unsigned short>(10), 8ULL, static_cast<short>(9), 4.0f);
  LOG_INFO(logger, "test fox logging quick {}", "example2");
  LOG_INFO(logger, "lazy brown example quick {} {} {} {} {}", std::string_view("view1"), true, 5L,
           std::string("str1"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "example dog jumps logging {} {} {} {} {} {} {} {}", 5L, 8ULL, 4.0f,
           static_cast<unsigned short>(10), std::string("str2"), static_cast<short>(9), 6LL, false);
  LOG_INFO(logger, "over test brown {} {} {}", 2, 4.0f, 3.0);
  LOG_INFO(logger, "over example brown quick {}", 5L);
  LOG_INFO(logger, "over example jumps lazy {} {}", std::string_view("view1"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox lazy over logging {} {} {} {} {} {}", 3.0, std::string_view("view1"), true,
           5L, 1, std::string("str1"));
  LOG_INFO(logger, "test jumps example logging {} {} {} {} {} {} {} {}", "example2", 5L, 7UL, 6LL,
           std::string_view("view1"), "example3", "example1", std::string("str2"));
  LOG_INFO(logger, "brown dog fox {} {} {}", true, std::string("str1"), 5L);
  LOG_INFO(logger, "example brown over {} {} {} {} {}", std::string_view("view1"), "example2", 2,
           std::string_view("view2"), 6LL);
  LOG_INFO(logger, "fox dog test {} {} {} {}", static_cast<unsigned short>(10), std::string("str1"), 8ULL, true);
  LOG_INFO(logger, "example quick lazy {} {}", 2, 1);
  LOG_INFO(logger, "jumps fox over {} {}", static_cast<short>(9), true);
  LOG_INFO(logger, "dog logging over lazy {} {} {}", "example1", std::string("str1"), 4.0f);
  LOG_INFO(logger, "quick dog lazy example {} {} {}", 3.0, 5L, 7UL);
  LOG_INFO(logger, "over fox quick {} {} {} {} {} {} {} {} {}", 4.0f, static_cast<unsigned short>(10),
           std::string_view("view1"), "example1", false, std::string("str1"), 6LL, 3.0, 1);
  LOG_INFO(logger, "lazy example logging quick {} {}", std::string("str2"), 1);
  LOG_INFO(logger, "jumps fox dog test {} {} {} {}", 3.0, "example2", 8ULL, 5L);
  LOG_INFO(logger, "quick example logging {} {} {} {} {}", std::string_view("view2"), 3.0, 8ULL,
           std::string("str2"), 5L);
  LOG_INFO(logger, "lazy example test over {} {} {} {} {} {}", 8ULL, 3.0, 1,
           std::string_view("view2"), std::string("str2"), "example1");
  LOG_INFO(logger, "dog example jumps {} {} {} {} {} {} {} {}", static_cast<short>(9), 7UL, 8ULL,
           "example1", 5L, "example3", 6LL, std::string("str1"));
  LOG_INFO(logger, "test jumps logging quick {} {} {}", std::string_view("view1"), 5L, false);
  LOG_INFO(logger, "fox jumps over {} {} {} {} {} {} {} {} {} {}", std::string("str2"),
           static_cast<short>(9), std::string_view("view2"), 1, "example1", "example3",
           static_cast<unsigned short>(10), 3.0, 5L, "example2");
  LOG_INFO(logger, "dog logging jumps test {} {} {} {} {} {} {} {} {}", "example3", 8ULL,
           "example1", 6LL, std::string("str2"), static_cast<unsigned short>(10), 2, 1, false);
  LOG_INFO(logger, "quick fox jumps {} {} {} {} {} {} {} {}", true, 6LL, std::string_view("view1"),
           8ULL, static_cast<unsigned short>(10), "example2", 1, 2);
  LOG_INFO(logger, "jumps over test logging {} {} {} {} {} {}", std::string("str2"), static_cast<short>(9),
           std::string_view("view1"), "example3", "example2", std::string_view("view2"));
  LOG_INFO(logger, "example brown fox dog {} {} {} {} {}", 2, true, 1, 6LL, std::string("str2"));
  LOG_INFO(logger, "jumps dog example {} {} {} {} {} {} {} {} {}", 8ULL, 2, std::string_view("view2"),
           1, std::string("str1"), "example3", "example2", static_cast<unsigned short>(10), true);
  LOG_INFO(logger, "example over brown {} {} {} {} {} {} {} {} {} {}", "example3",
           std::string_view("view1"), 5L, 4.0f, std::string("str1"), 6LL, true, "example2", 2, 8ULL);
  LOG_INFO(logger, "dog brown quick example {} {} {} {} {} {} {} {} {} {}", 7UL, "example3", true,
           8ULL, 1, 4.0f, false, 3.0, std::string("str1"), "example1");
  LOG_INFO(logger, "jumps lazy logging {} {} {} {} {}", 6LL, 8ULL, 7UL, std::string("str1"),
           std::string("str2"));
  LOG_INFO(logger, "example quick jumps {} {} {} {} {} {} {} {}", 1, 6LL, std::string_view("view2"),
           "example1", std::string("str2"), std::string("str1"), 5L, false);
  LOG_INFO(logger, "test dog jumps over {} {} {} {} {} {} {} {} {}", "example1",
           std::string_view("view2"), 6LL, 8ULL, true, "example3", 2, 1, std::string("str1"));
  LOG_INFO(logger, "brown dog fox {} {} {} {} {} {} {} {}", 2, 5L, static_cast<short>(9),
           "example1", 3.0, "example3", false, std::string("str1"));
  LOG_INFO(logger, "over jumps lazy {} {} {} {} {} {} {} {}", static_cast<short>(9), 2, 1, true,
           3.0, 8ULL, 4.0f, std::string_view("view1"));
  LOG_INFO(logger, "example brown test {} {} {} {} {} {}", 7UL, 6LL, std::string("str1"),
           "example2", std::string_view("view1"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps brown over fox {} {}", 7UL, 1);
  LOG_INFO(logger, "lazy example over {} {} {} {} {} {} {} {}", true, false, 6LL,
           std::string_view("view1"), std::string("str2"), std::string_view("view2"), "example2", 3.0);
  LOG_INFO(logger, "logging example dog fox {} {} {} {}", 1, std::string_view("view1"),
           std::string("str2"), 4.0f);
  LOG_INFO(logger, "fox lazy brown quick {} {} {} {} {}", 3.0, false, std::string_view("view1"),
           "example2", true);
  LOG_INFO(logger, "test fox dog quick {} {} {} {}", "example2", "example3", 3.0, static_cast<short>(9));
  LOG_INFO(logger, "jumps dog test {} {} {} {} {}", 5L, 7UL, true, 1, std::string("str1"));
  LOG_INFO(logger, "quick jumps fox lazy {} {} {} {}", false, true, std::string("str1"), "example2");
  LOG_INFO(logger, "jumps lazy over fox {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           "example1", 1, 3.0, std::string("str1"), true, "example2", "example3", static_cast<short>(9));
  LOG_INFO(logger, "brown fox lazy {} {} {} {} {} {}", true, static_cast<unsigned short>(10), 6LL,
           "example3", static_cast<short>(9), std::string_view("view1"));
  LOG_INFO(logger, "brown fox over {} {} {} {} {} {} {} {}", static_cast<short>(9), std::string("str1"),
           std::string("str2"), 5L, 1, 6LL, std::string_view("view1"), "example1");
  LOG_INFO(logger, "logging dog over brown {} {} {} {} {} {} {}", "example1", 2, "example2",
           std::string("str2"), 3.0, 4.0f, 8ULL);
  LOG_INFO(logger, "lazy fox logging {}", 7UL);
  LOG_INFO(logger, "quick example logging {} {} {}", false, std::string("str1"), std::string_view("view1"));
  LOG_INFO(logger, "quick dog test {} {} {} {}", "example3", 1, 3.0, 7UL);
  LOG_INFO(logger, "example lazy logging {} {} {} {} {} {} {} {}", 1, static_cast<short>(9),
           "example2", static_cast<unsigned short>(10), 3.0, 2, "example3", 8ULL);
  LOG_INFO(logger, "dog fox example {} {}", static_cast<unsigned short>(10), "example1");
  LOG_INFO(logger, "lazy example jumps brown {} {} {} {}", 5L, "example2", 6LL, 3.0);
  LOG_INFO(logger, "jumps lazy quick {} {} {} {} {} {} {} {} {}", 5L, 6LL, 8ULL,
           std::string("str1"), static_cast<unsigned short>(10), std::string_view("view2"),
           std::string("str2"), "example3", 4.0f);
  LOG_INFO(logger, "over test quick {} {} {}", std::string("str1"), std::string_view("view2"),
           "example3");
  LOG_INFO(logger, "example test brown over {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           8ULL, std::string_view("view1"), 6LL, static_cast<short>(9), "example1", 5L, true);
  LOG_INFO(logger, "dog logging example {}", 4.0f);
  LOG_INFO(logger, "example jumps dog over {} {} {} {} {} {} {} {}", "example1", "example2", true,
           6LL, std::string_view("view2"), 2, false, std::string("str1"));
  LOG_INFO(logger, "test example dog jumps {} {} {} {} {} {} {}", 1, std::string_view("view1"), 2,
           "example1", static_cast<short>(9), 3.0, 6LL);
  LOG_INFO(logger, "jumps fox dog {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           std::string_view("view1"), "example3", 6LL, 5L, 2, true, 4.0f, 1, "example2");
  LOG_INFO(logger, "fox quick over jumps {} {} {} {} {} {} {} {}", 7UL, 2, "example3", "example1",
           true, std::string_view("view1"), 5L, 1);
  LOG_INFO(logger, "test quick logging brown {} {}", true, "example2");
  LOG_INFO(logger, "quick jumps logging {} {} {} {} {}", 1, 7UL, 6LL, 3.0, static_cast<unsigned short>(10));
  LOG_INFO(logger, "test logging fox quick {} {} {} {} {} {} {}", 3.0, std::string_view("view2"),
           std::string("str1"), 4.0f, 8ULL, false, 2);
  LOG_INFO(logger, "example quick logging brown {} {} {} {} {} {} {} {} {} {}", 3.0, 8ULL, std::string("str2"),
           4.0f, false, true, 6LL, static_cast<unsigned short>(10), "example1", std::string("str1"));
  LOG_INFO(logger, "jumps logging quick {} {}", std::string("str1"), 8ULL);
  LOG_INFO(logger, "fox example lazy {} {} {} {} {} {}", std::string_view("view2"), 8ULL, 6LL, 4.0f,
           true, "example2");
  LOG_INFO(logger, "over example logging {} {} {}", std::string("str1"), "example1", "example3");
  LOG_INFO(logger, "fox brown lazy over {} {} {} {} {} {} {}", 1, static_cast<short>(9), 4.0f,
           std::string("str2"), 6LL, 5L, "example1");
  LOG_INFO(logger, "jumps logging quick {} {} {} {} {} {} {} {} {} {}",
           static_cast<unsigned short>(10), std::string_view("view1"), std::string_view("view2"),
           5L, true, std::string("str2"), 4.0f, 8ULL, 3.0, "example2");
  LOG_INFO(logger, "over logging example {} {} {} {} {} {} {} {} {} {}", std::string("str1"), 3.0,
           2, false, 8ULL, 4.0f, 1, 6LL, std::string_view("view2"), std::string("str2"));
  LOG_INFO(logger, "logging brown test fox {} {} {} {}", std::string("str2"), 3.0, std::string("str1"), 1);
  LOG_INFO(logger, "example dog brown jumps {} {} {} {}", 1, "example2", 5L, false);
  LOG_INFO(logger, "logging jumps example fox {} {} {} {} {} {} {}", 3.0, std::string_view("view2"),
           true, static_cast<unsigned short>(10), 5L, std::string_view("view1"), std::string("str1"));
  LOG_INFO(logger, "fox brown lazy test {} {}", 2, 8ULL);
  LOG_INFO(logger, "dog brown lazy fox {} {} {} {} {} {} {} {}", std::string_view("view2"), 1, 4.0f,
           true, 7UL, "example3", 6LL, 5L);
  LOG_INFO(logger, "lazy test dog {}", 3.0);
  LOG_INFO(logger, "brown example jumps {}", 4.0f);
  LOG_INFO(logger, "quick logging fox jumps {} {} {} {} {} {} {} {} {} {}", 1,
           static_cast<short>(9), std::string_view("view1"), std::string_view("view2"), true, 6LL,
           false, std::string("str1"), 2, 7UL);
  LOG_INFO(logger, "brown jumps test {}", 3.0);
  LOG_INFO(logger, "jumps lazy quick {} {} {} {} {} {} {}", 8ULL, true, std::string_view("view1"),
           false, std::string("str1"), "example1", std::string("str2"));
  LOG_INFO(logger, "example logging quick over {} {} {} {}", std::string("str1"), 1, "example1", true);
  LOG_INFO(logger, "example logging lazy over {} {} {} {} {} {} {} {} {}", "example2", 3.0,
           std::string_view("view2"), static_cast<short>(9), "example3", 2, std::string("str1"),
           false, std::string_view("view1"));
  LOG_INFO(logger, "dog fox example quick {} {} {}", std::string("str2"), true, static_cast<short>(9));
  LOG_INFO(logger, "test quick example fox {} {} {} {} {}", true, 2, 5L, "example3", 1);
  LOG_INFO(logger, "quick over jumps fox {} {} {} {} {}", 6LL, 8ULL, 4.0f, true, 5L);
  LOG_INFO(logger, "example fox logging jumps {} {}", 6LL, 3.0);
  LOG_INFO(logger, "quick jumps dog {} {} {}", 6LL, std::string_view("view2"), false);
  LOG_INFO(logger, "lazy example quick {} {} {} {} {} {}", std::string("str2"),
           static_cast<short>(9), std::string("str1"), 6LL, true, 8ULL);
  LOG_INFO(logger, "example logging over {} {} {} {} {}", std::string("str1"), 2,
           static_cast<short>(9), std::string_view("view2"), false);
  LOG_INFO(logger, "test quick example {} {} {} {} {} {} {}", static_cast<unsigned short>(10), 6LL,
           static_cast<short>(9), false, std::string_view("view2"), 8ULL, "example1");
  LOG_INFO(logger, "quick brown logging dog {} {} {} {} {} {} {}", 4.0f, 1, 2,
           std::string_view("view2"), 8ULL, static_cast<short>(9), 7UL);
  LOG_INFO(logger, "logging lazy jumps {} {} {} {} {} {} {} {}", "example1", 5L,
           static_cast<unsigned short>(10), std::string("str2"), true, std::string_view("view2"), 4.0f, false);
  LOG_INFO(logger, "over quick fox dog {} {} {} {} {} {} {} {} {}", 3.0, 1, false, 5L, 4.0f, 6LL,
           static_cast<short>(9), std::string_view("view2"), std::string_view("view1"));
  LOG_INFO(logger, "brown fox test example {} {} {} {} {} {} {} {} {} {}", std::string("str2"),
           true, static_cast<unsigned short>(10), std::string_view("view1"), 5L, "example2", false,
           3.0, 1, std::string_view("view2"));
  LOG_INFO(logger, "example lazy jumps fox {} {} {} {} {} {}", 7UL, false,
           std::string_view("view1"), 6LL, "example2", 5L);
  LOG_INFO(logger, "fox jumps over test {} {}", "example2", "example1");
  LOG_INFO(logger, "jumps dog logging {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox test jumps {} {} {} {} {} {} {} {}", true, std::string_view("view2"),
           std::string("str1"), 4.0f, 2, 6LL, std::string_view("view1"), std::string("str2"));
  LOG_INFO(logger, "over quick brown {} {} {} {} {} {} {} {} {} {}", false, 8ULL, "example2",
           "example3", "example1", std::string("str2"), static_cast<unsigned short>(10), 4.0f, 5L,
           std::string("str1"));
  LOG_INFO(logger, "quick brown fox example {} {} {} {} {} {}", "example1", false,
           static_cast<short>(9), std::string("str1"), "example2", 4.0f);
  LOG_INFO(logger, "test logging jumps quick {} {} {} {} {} {}", 4.0f, true,
           std::string_view("view1"), "example1", 5L, 7UL);
  LOG_INFO(logger, "test lazy example {} {}", 2, std::string("str2"));
  LOG_INFO(logger, "quick over dog logging {} {} {} {} {} {} {} {} {} {}", 8ULL, std::string_view("view1"),
           true, 6LL, "example2", 4.0f, "example1", static_cast<unsigned short>(10), "example3", 7UL);
  LOG_INFO(logger, "lazy jumps quick dog {} {} {}", std::string("str1"), "example2", false);
  LOG_INFO(logger, "test brown logging example {} {}", std::string_view("view2"), 6LL);
  LOG_INFO(logger, "over lazy dog jumps {} {} {} {} {} {}", std::string("str1"), 1, 4.0f, 6LL, 2,
           "example3");
  LOG_INFO(logger, "lazy over fox {} {}", std::string_view("view2"), std::string_view("view1"));
  LOG_INFO(logger, "brown dog lazy over {} {} {} {} {} {} {} {} {} {}", "example1", "example2", 7UL,
           static_cast<unsigned short>(10), 4.0f, false, 1, "example3", 2, std::string_view("view1"));
  LOG_INFO(logger, "jumps example over {} {} {} {}", 8ULL, static_cast<unsigned short>(10),
           "example1", std::string_view("view1"));
  LOG_INFO(logger, "test jumps over lazy {} {} {} {}", std::string("str2"), 2, 8ULL, std::string("str1"));
  LOG_INFO(logger, "logging example over {} {}", std::string("str2"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps lazy fox {} {} {} {} {} {}", 3.0, false, 5L, static_cast<short>(9),
           "example1", std::string("str2"));
  LOG_INFO(logger, "dog example quick brown {} {} {} {} {} {} {} {}", 5L, static_cast<short>(9),
           "example3", true, false, std::string_view("view2"), 7UL, 8ULL);
  LOG_INFO(logger, "dog over quick {} {} {} {} {} {}", static_cast<short>(9),
           static_cast<unsigned short>(10), "example2", 5L, "example3", 2);
  LOG_INFO(logger, "example test jumps {} {} {} {} {} {} {} {} {}", 8ULL, false,
           std::string_view("view2"), true, "example3", "example1", static_cast<unsigned short>(10),
           std::string_view("view1"), "example2");
  LOG_INFO(logger, "example brown jumps quick {} {} {} {}", "example1", std::string("str1"),
           static_cast<short>(9), 8ULL);
  LOG_INFO(logger, "jumps lazy dog {} {} {} {}", std::string_view("view2"), "example3",
           std::string("str2"), 3.0);
  LOG_INFO(logger, "dog lazy brown {} {}", 6LL, std::string("str1"));
  LOG_INFO(logger, "test quick dog {} {} {} {} {} {} {}", "example2", std::string_view("view2"),
           6LL, static_cast<short>(9), 1, std::string("str2"), 3.0);
  LOG_INFO(logger, "quick fox lazy over {} {} {} {} {} {} {} {}", 4.0f, false,
           std::string_view("view1"), 3.0, 8ULL, "example3", 6LL, "example2");
  LOG_INFO(logger, "brown fox jumps {}", "example3");
  LOG_INFO(logger, "logging quick fox {} {} {} {} {} {}", 1, std::string_view("view1"),
           std::string("str2"), 5L, false, 6LL);
  LOG_INFO(logger, "test dog over {}", 7UL);
  LOG_INFO(logger, "fox test lazy {} {} {} {} {} {} {} {} {}", 3.0, true, 6LL, std::string_view("view2"),
           "example2", static_cast<short>(9), std::string_view("view1"), "example1", 4.0f);
  LOG_INFO(logger, "example fox quick {} {} {} {} {} {} {}", 5L, 7UL, std::string_view("view1"),
           "example3", "example2", "example1", static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog lazy jumps {} {} {} {} {} {} {} {} {}", true, std::string_view("view1"),
           7UL, 8ULL, static_cast<unsigned short>(10), 1, 4.0f, 6LL, std::string("str2"));
  LOG_INFO(logger, "logging example lazy test {}", true);
  LOG_INFO(logger, "over quick test jumps {} {} {} {}", std::string("str2"), true, std::string("str1"), 3.0);
  LOG_INFO(logger, "quick example over brown {} {} {} {} {} {} {} {}", "example3", false,
           std::string_view("view2"), static_cast<unsigned short>(10), 2, static_cast<short>(9), 5L, 7UL);
  LOG_INFO(logger, "example test dog {} {}", 3.0, "example2");
  LOG_INFO(logger, "jumps dog brown logging {} {} {} {}", std::string("str2"), std::string("str1"),
           "example3", "example1");
  LOG_INFO(logger, "over logging lazy test {} {} {} {} {} {} {}", 6LL, true, std::string("str1"),
           "example2", std::string_view("view2"), 4.0f, "example1");
  LOG_INFO(logger, "dog quick lazy test {} {} {} {} {} {} {} {} {}", 2, "example2", "example3",
           std::string("str1"), static_cast<short>(9), 4.0f, "example1", 6LL, 5L);
  LOG_INFO(logger, "example test brown {} {} {}", 4.0f, "example1", 8ULL);
  LOG_INFO(logger, "lazy over brown jumps {} {} {} {} {} {} {}", std::string("str1"), 8ULL, 5L, 3.0,
           "example3", "example2", "example1");
  LOG_INFO(logger, "fox quick jumps dog {} {} {} {} {} {}", 4.0f, "example1", true,
           std::string("str1"), std::string_view("view1"), 3.0);
  LOG_INFO(logger, "dog test jumps over {} {} {} {} {} {} {}", std::string_view("view1"), 4.0f,
           static_cast<short>(9), 1, std::string_view("view2"), static_cast<unsigned short>(10), false);
  LOG_INFO(logger, "lazy quick test {} {} {} {}", 1, static_cast<short>(9), 3.0, std::string("str2"));
  LOG_INFO(logger, "jumps brown fox {} {} {} {} {} {}", std::string("str1"),
           static_cast<unsigned short>(10), 8ULL, "example1", std::string("str2"), 1);
  LOG_INFO(logger, "test jumps quick {} {} {} {} {} {}", "example2", 1, 2, std::string_view("view2"), 8ULL, true);
  LOG_INFO(logger, "fox over example quick {}", std::string("str2"));
  LOG_INFO(logger, "over dog test {} {} {} {} {} {} {} {} {} {}", false, 3.0, "example1",
           std::string("str1"), std::string("str2"), 2, "example2", "example3", static_cast<short>(9), 1);
  LOG_INFO(logger, "example logging dog {}", "example1");
  LOG_INFO(logger, "test logging jumps {} {} {} {} {} {} {} {}", 1, "example3", std::string("str1"),
           6LL, 3.0, "example1", 7UL, 5L);
  LOG_INFO(logger, "fox example jumps quick {} {} {} {} {} {} {} {} {} {}", 5L, std::string("str1"),
           false, 3.0, 2, 6LL, 7UL, true, "example3", std::string_view("view2"));
  LOG_INFO(logger, "fox lazy example test {} {}", true, 5L);
  LOG_INFO(logger, "example brown dog jumps {}", 4.0f);
  LOG_INFO(logger, "dog jumps fox {} {} {} {} {} {}", 8ULL, "example2", std::string_view("view2"),
           "example3", std::string("str2"), 3.0);
  LOG_INFO(logger, "quick lazy example logging {}", std::string_view("view2"));
  LOG_INFO(logger, "dog brown over {} {}", std::string_view("view2"), "example3");
  LOG_INFO(logger, "jumps brown over {} {}", "example2", "example3");
  LOG_INFO(logger, "test over quick {} {} {} {}", 4.0f, std::string("str2"), 8ULL, 5L);
  LOG_INFO(logger, "quick jumps brown example {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string("str1"), 1, 8ULL, "example2", 4.0f, static_cast<short>(9));
  LOG_INFO(logger, "over jumps dog {} {} {} {} {} {} {} {} {} {}", "example2",
           static_cast<unsigned short>(10), "example1", 4.0f, std::string_view("view1"), 3.0,
           std::string("str2"), 5L, false, "example3");
  LOG_INFO(logger, "fox jumps quick {} {} {} {} {} {} {} {}", true, 1, std::string_view("view2"),
           std::string_view("view1"), std::string("str1"), 5L, static_cast<unsigned short>(10),
           "example1");
  LOG_INFO(logger, "fox logging lazy {} {} {} {} {}", 2, 4.0f, std::string("str2"),
           std::string_view("view2"), 3.0);
  LOG_INFO(logger, "logging fox jumps {} {} {} {}", std::string_view("view2"), 4.0f,
           std::string_view("view1"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox brown dog test {} {}", 2, 7UL);
  LOG_INFO(logger, "lazy dog jumps brown {} {} {}", std::string_view("view2"), 3.0, "example1");
  LOG_INFO(logger, "fox dog test jumps {} {} {} {}", 6LL, static_cast<short>(9),
           static_cast<unsigned short>(10), 8ULL);
  LOG_INFO(logger, "quick brown logging over {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging dog jumps {}", "example2");
  LOG_INFO(logger, "quick jumps example {} {} {} {} {} {} {} {} {} {}", 8ULL, 2, static_cast<short>(9),
           "example2", 6LL, std::string_view("view1"), "example3", true, 5L, 4.0f);
  LOG_INFO(logger, "brown over quick logging {} {} {} {} {}", false, 7UL, "example3", true,
           std::string_view("view1"));
  LOG_INFO(logger, "jumps example fox {}", 1);
  LOG_INFO(logger, "over example jumps quick {} {} {}", 1, 2, std::string("str1"));
  LOG_INFO(logger, "test over dog quick {}", "example3");
  LOG_INFO(logger, "over quick logging {} {} {}", false, 4.0f, 3.0);
  LOG_INFO(logger, "fox test example jumps {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           8ULL, 2, "example2", std::string_view("view1"), std::string("str1"), static_cast<short>(9));
  LOG_INFO(logger, "brown jumps example logging {} {} {} {}", static_cast<short>(9),
           static_cast<unsigned short>(10), 3.0, "example3");
  LOG_INFO(logger, "over fox lazy {}", 6LL);
  LOG_INFO(logger, "test fox example {} {} {} {} {} {} {} {}", 2, true, std::string_view("view2"),
           static_cast<short>(9), std::string("str1"), 4.0f, "example1", std::string_view("view1"));
  LOG_INFO(logger, "dog over test {} {} {} {} {}", 5L, false, 3.0, 7UL, 8ULL);
  LOG_INFO(logger, "over jumps logging example {} {} {} {} {} {} {} {} {} {}",
           static_cast<unsigned short>(10), "example3", std::string("str2"), true,
           std::string("str1"), "example1", false, 6LL, 1, "example2");
  LOG_INFO(logger, "logging test fox over {} {} {} {} {} {} {} {} {}", false, true,
           static_cast<short>(9), 3.0, 4.0f, 5L, 8ULL, 6LL, 1);
  LOG_INFO(logger, "logging lazy example {} {} {} {} {} {} {} {} {} {}", 8ULL, "example2",
           std::string_view("view1"), 7UL, 6LL, static_cast<unsigned short>(10), "example3", false,
           3.0, "example1");
  LOG_INFO(logger, "dog brown example {}", "example1");
  LOG_INFO(logger, "logging dog example fox {} {} {} {} {} {} {} {} {}", "example2", 4.0f,
           "example1", true, 5L, static_cast<short>(9), 7UL, "example3", std::string_view("view1"));
  LOG_INFO(logger, "fox dog test {} {} {} {} {} {}", false, static_cast<unsigned short>(10),
           std::string_view("view1"), static_cast<short>(9), 7UL, "example1");
  LOG_INFO(logger, "fox brown example {} {} {} {} {}", static_cast<short>(9), "example3", 6LL,
           std::string("str1"), "example1");
  LOG_INFO(logger, "fox dog example {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox quick lazy {} {} {} {} {} {} {}", std::string_view("view2"), 1,
           std::string("str2"), "example2", 3.0, static_cast<short>(9), 6LL);
  LOG_INFO(logger, "dog test example logging {} {}", std::string_view("view1"), 8ULL);
  LOG_INFO(logger, "lazy quick over {}", true);
  LOG_INFO(logger, "over fox test {} {} {}", std::string_view("view2"), false, 1);
  LOG_INFO(logger, "lazy jumps over dog {}", 2);
  LOG_INFO(logger, "example fox brown over {} {} {} {} {}", "example1", 8ULL, "example2", 5L, 3.0);
  LOG_INFO(logger, "dog brown over fox {} {} {} {}", static_cast<unsigned short>(10), "example2",
           6LL, "example3");
  LOG_INFO(logger, "quick lazy over {} {} {} {} {} {}", 6LL, "example3",
           static_cast<unsigned short>(10), 5L, std::string_view("view1"), 7UL);
  LOG_INFO(logger, "over example fox quick {} {}", true, 4.0f);
  LOG_INFO(logger, "logging over jumps brown {} {} {} {} {} {} {} {} {} {}", "example3", 5L,
           std::string_view("view1"), 8ULL, static_cast<unsigned short>(10), 7UL,
           std::string("str1"), 1, static_cast<short>(9), true);
  LOG_INFO(logger, "lazy quick dog {} {} {} {} {} {} {}", 4.0f, "example2", "example1", 2,
           std::string("str1"), true, 3.0);
  LOG_INFO(logger, "dog example brown {} {} {} {} {} {} {}", 2, 7UL,
           static_cast<unsigned short>(10), 4.0f, std::string_view("view2"), "example3", "example1");
  LOG_INFO(logger, "jumps dog fox test {}", std::string_view("view2"));
  LOG_INFO(logger, "jumps dog quick fox {} {} {} {} {} {}", false, std::string_view("view1"),
           static_cast<short>(9), 8ULL, 4.0f, 5L);
  LOG_INFO(logger, "fox lazy test jumps {} {} {} {} {} {} {} {} {}", 7UL, std::string("str2"),
           std::string_view("view1"), std::string("str1"), 4.0f, static_cast<short>(9), "example2", 3.0, 2);
  LOG_INFO(logger, "test logging over fox {} {} {}", "example2", false, 8ULL);
  LOG_INFO(logger, "example quick brown lazy {} {} {} {} {} {} {} {}", 7UL, std::string("str2"), 5L,
           true, std::string_view("view1"), "example2", 2, 6LL);
  LOG_INFO(logger, "dog quick jumps {} {} {} {}", 5L, "example3", 1, std::string("str2"));
  LOG_INFO(logger, "dog fox lazy {}", false);
  LOG_INFO(logger, "logging fox quick {} {} {} {} {} {} {} {} {} {}", 1, std::string("str2"), 4.0f,
           7UL, static_cast<unsigned short>(10), std::string_view("view1"), static_cast<short>(9),
           std::string("str1"), "example2", 5L);
  LOG_INFO(logger, "lazy quick example test {} {} {} {} {} {} {} {}", 5L, "example3", false,
           std::string_view("view2"), static_cast<unsigned short>(10), 4.0f, "example1", 2);
  LOG_INFO(logger, "jumps quick dog {} {} {} {} {}", 5L, "example3", std::string("str1"), true, 7UL);
  LOG_INFO(logger, "example jumps logging {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           3.0, false, 2, std::string_view("view1"), "example1", std::string_view("view2"), 8ULL, 6LL);
  LOG_INFO(logger, "brown example test {} {} {} {} {} {}", 6LL, static_cast<unsigned short>(10),
           3.0, "example1", 4.0f, 7UL);
  LOG_INFO(logger, "brown jumps dog lazy {}", 2);
  LOG_INFO(logger, "test quick logging brown {} {} {} {} {}", 5L, "example3", 8ULL, true, 4.0f);
  LOG_INFO(logger, "fox logging brown {} {} {} {} {} {}", std::string_view("view1"), "example2",
           true, std::string("str2"), 2, 5L);
  LOG_INFO(logger, "test lazy quick over {} {} {} {}", 7UL, 5L, static_cast<unsigned short>(10),
           std::string_view("view1"));
  LOG_INFO(logger, "test over brown {} {} {} {} {} {} {} {}", std::string("str2"), false, 8ULL, 2,
           true, "example3", 1, std::string_view("view1"));
  LOG_INFO(logger, "jumps brown fox logging {} {} {} {} {} {} {} {} {}", false, true, 6LL,
           static_cast<short>(9), 2, 4.0f, std::string("str1"), 7UL, "example2");
  LOG_INFO(logger, "logging test over {} {} {} {}", std::string("str2"), "example1", 5L, 3.0);
  LOG_INFO(logger, "lazy test jumps {} {} {}", 2, "example1", std::string("str2"));
  LOG_INFO(logger, "quick jumps fox test {} {}", 1, std::string("str2"));
  LOG_INFO(logger, "example logging over jumps {} {}", 7UL, 6LL);
  LOG_INFO(logger, "over fox test {} {} {} {} {} {} {} {} {}", 8ULL, static_cast<short>(9), 1,
           std::string_view("view1"), "example1", 7UL, "example3", 3.0, 6LL);
  LOG_INFO(logger, "test lazy dog {} {} {}", 2, 3.0, std::string_view("view2"));
  LOG_INFO(logger, "quick test logging lazy {} {}", std::string_view("view2"), "example3");
  LOG_INFO(logger, "quick fox test brown {} {} {} {} {} {} {} {} {}", 7UL, 1,
           static_cast<unsigned short>(10), 8ULL, false, std::string_view("view2"), 3.0, "example2",
           std::string_view("view1"));
  LOG_INFO(logger, "lazy over jumps {}", std::string("str2"));
  LOG_INFO(logger, "test dog lazy {} {}", std::string_view("view1"), false);
  LOG_INFO(logger, "jumps over quick {} {} {} {} {} {} {} {}", 7UL, 1, std::string_view("view2"),
           "example2", 4.0f, true, static_cast<short>(9), "example3");
  LOG_INFO(logger, "logging brown jumps {} {} {} {} {} {} {} {}", std::string("str2"), false,
           std::string("str1"), "example3", 4.0f, 6LL, 8ULL, 5L);
  LOG_INFO(logger, "brown jumps quick over {} {} {} {} {}", 8ULL, 5L, "example2",
           static_cast<unsigned short>(10), std::string_view("view1"));
  LOG_INFO(logger, "brown fox example logging {} {} {} {} {} {}", 3.0, std::string_view("view1"),
           6LL, "example2", 8ULL, 7UL);
  LOG_INFO(logger, "dog test fox {} {} {} {} {} {}", 1, static_cast<unsigned short>(10),
           static_cast<short>(9), 2, 5L, true);
  LOG_INFO(logger, "dog logging quick {} {}", std::string("str2"), 7UL);
  LOG_INFO(logger, "jumps brown quick logging {} {} {} {} {} {}", std::string("str2"), 7UL,
           static_cast<unsigned short>(10), 4.0f, 5L, false);
  LOG_INFO(logger, "dog logging jumps {} {} {} {} {} {} {} {} {} {}", std::string("str2"), 2,
           "example1", "example3", std::string("str1"), false, static_cast<short>(9),
           std::string_view("view2"), std::string_view("view1"), 5L);
  LOG_INFO(logger, "fox over lazy {} {} {} {} {} {}", 1, std::string_view("view2"), 3.0, "example1",
           std::string_view("view1"), "example3");
  LOG_INFO(logger, "dog lazy quick fox {} {} {} {}", std::string_view("view2"), 4.0f, "example3", 2);
  LOG_INFO(logger, "fox lazy dog {} {} {} {} {} {} {} {} {} {}", "example1", 6LL, 3.0, false,
           "example2", std::string("str2"), std::string_view("view1"), true, 8ULL,
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox over jumps {} {}", 6LL, std::string("str2"));
  LOG_INFO(logger, "test logging jumps {} {} {} {} {} {} {} {}", "example1", static_cast<short>(9), 7UL,
           std::string("str2"), std::string_view("view2"), false, static_cast<unsigned short>(10), 3.0);
  LOG_INFO(logger, "test logging lazy example {} {} {} {}", 7UL, 5L, 2, false);
  LOG_INFO(logger, "logging dog lazy {} {} {} {} {} {} {} {} {} {}", "example2", 2, true,
           "example1", static_cast<short>(9), 4.0f, 5L, 1, "example3", 6LL);
  LOG_INFO(logger, "brown over fox {} {} {} {}", std::string_view("view1"),
           std::string_view("view2"), 8ULL, std::string("str2"));
  LOG_INFO(logger, "jumps logging lazy {} {} {} {} {} {} {}", std::string("str2"),
           std::string_view("view1"), 7UL, 1, 8ULL, 3.0, std::string("str1"));
  LOG_INFO(logger, "fox brown dog logging {}", false);
  LOG_INFO(logger, "brown over fox {} {} {} {} {} {} {} {} {}", 8ULL, static_cast<short>(9), 2, 6LL,
           std::string_view("view1"), false, 7UL, 3.0, 1);
  LOG_INFO(logger, "test over dog {} {} {} {}", 5L, std::string_view("view1"), 7UL, std::string_view("view2"));
  LOG_INFO(logger, "dog brown jumps {} {} {}", static_cast<short>(9), static_cast<unsigned short>(10), 8ULL);
  LOG_INFO(logger, "logging brown fox over {} {} {}", false, 8ULL, std::string_view("view1"));
  LOG_INFO(logger, "logging over brown example {} {} {} {} {} {} {} {}", 2, 1, std::string_view("view1"),
           3.0, std::string_view("view2"), static_cast<short>(9), std::string("str1"), 5L);
  LOG_INFO(logger, "lazy jumps logging test {} {} {} {} {}", "example2", false, std::string("str1"),
           "example3", 2);
  LOG_INFO(logger, "brown example lazy {} {}", std::string_view("view1"), static_cast<short>(9));
  LOG_INFO(logger, "example brown quick logging {} {} {} {} {} {}", 4.0f, "example2", 3.0, 1, false,
           std::string("str1"));
  LOG_INFO(logger, "over dog test {} {} {} {} {} {} {}", 4.0f, "example1", 7UL, 5L,
           std::string("str2"), std::string_view("view1"), false);
  LOG_INFO(logger, "quick lazy jumps {} {} {} {} {} {} {} {}", 6LL, 5L, "example1", 2, "example2",
           7UL, std::string_view("view1"), static_cast<short>(9));
  LOG_INFO(logger, "brown logging quick {} {} {} {} {} {} {} {} {}", std::string("str2"),
           std::string_view("view1"), std::string_view("view2"), "example3", false, 6LL, 4.0f, true, 5L);
  LOG_INFO(logger, "quick logging jumps {} {} {} {} {} {} {} {} {} {}", "example2", std::string_view("view2"),
           true, 6LL, 8ULL, static_cast<short>(9), 4.0f, 3.0, std::string("str1"), "example1");
  LOG_INFO(logger, "lazy jumps quick fox {} {}", 8ULL, std::string_view("view2"));
  LOG_INFO(logger, "over fox jumps {} {} {} {} {}", std::string_view("view1"),
           static_cast<unsigned short>(10), static_cast<short>(9), "example1", true);
  LOG_INFO(logger, "dog over lazy {} {} {} {} {}", 1, static_cast<short>(9), std::string("str1"), true, 3.0);
  LOG_INFO(logger, "jumps quick fox example {} {} {} {} {} {}", std::string("str1"), 8ULL, true,
           false, "example3", static_cast<short>(9));
  LOG_INFO(logger, "quick lazy test brown {} {} {} {}", "example2", true, 1, "example1");
  LOG_INFO(logger, "dog logging jumps {} {} {} {} {} {} {} {} {}", std::string("str1"),
           static_cast<short>(9), std::string("str2"), std::string_view("view2"), 4.0f, false,
           "example2", static_cast<unsigned short>(10), 1);
  LOG_INFO(logger, "lazy dog fox example {} {} {} {} {} {} {} {} {}", 2, "example1",
           static_cast<short>(9), true, "example2", false, std::string_view("view1"), 8ULL, 4.0f);
  LOG_INFO(logger, "quick test lazy {} {} {} {} {} {} {} {} {}", false, std::string("str2"), 1, 2,
           std::string_view("view1"), 8ULL, "example2", "example3", 5L);
  LOG_INFO(logger, "brown lazy test over {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           5L, static_cast<short>(9), 8ULL, 7UL, 1, 6LL, "example3", 2);
  LOG_INFO(logger, "dog jumps fox {} {} {} {} {} {} {} {}", 7UL, 8ULL, false, std::string("str2"),
           std::string_view("view2"), true, "example2", std::string("str1"));
  LOG_INFO(logger, "lazy jumps over {} {} {} {} {} {} {}", "example1", std::string_view("view1"),
           5L, static_cast<unsigned short>(10), 2, std::string("str1"), true);
  LOG_INFO(logger, "dog brown test {} {} {} {} {} {} {} {} {}", 2, std::string("str2"),
           std::string_view("view2"), 6LL, static_cast<short>(9), std::string("str1"), "example3", 5L, true);
  LOG_INFO(logger, "test jumps logging brown {} {} {} {} {} {} {} {}", "example2", 3.0, 6LL, true,
           5L, 2, 4.0f, static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown lazy logging {} {} {} {} {} {}", "example3", 2, false, true, 4.0f,
           "example2");
  LOG_INFO(logger, "logging example lazy brown {} {}", 5L, std::string("str1"));
  LOG_INFO(logger, "jumps example lazy logging {}", 8ULL);
  LOG_INFO(logger, "logging test brown {} {} {} {} {} {} {} {} {} {}", std::string("str2"), 6LL,
           "example1", 8ULL, 1, 3.0, std::string_view("view1"), std::string_view("view2"),
           "example3", static_cast<unsigned short>(10));
  LOG_INFO(logger, "example lazy jumps {} {} {}", "example1", static_cast<unsigned short>(10),
           static_cast<short>(9));
  LOG_INFO(logger, "dog test fox logging {} {} {} {} {} {}", 6LL, std::string_view("view1"), true,
           5L, 8ULL, 7UL);
  LOG_INFO(logger, "brown over fox dog {} {} {} {} {} {} {}", 6LL, "example3", 4.0f, "example1", 3.0, true, 1);
  LOG_INFO(logger, "dog logging brown {} {} {} {} {} {} {} {} {}", 3.0, true, std::string("str1"),
           static_cast<short>(9), 1, 2, 6LL, 5L, "example2");
  LOG_INFO(logger, "logging fox dog lazy {} {} {} {} {} {}", 3.0, static_cast<short>(9),
           std::string_view("view1"), "example1", 5L, 1);
  LOG_INFO(logger, "fox lazy dog brown {} {} {} {} {} {} {} {}", false, 1, 8ULL, 3.0,
           static_cast<unsigned short>(10), true, "example2", 4.0f);
  LOG_INFO(logger, "quick test dog brown {} {} {}", true, std::string("str2"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps logging brown {} {} {} {} {} {}", false, 4.0f, std::string_view("view1"),
           std::string("str1"), 5L, "example1");
  LOG_INFO(logger, "dog fox brown jumps {} {} {} {} {} {} {} {}", std::string_view("view2"), 5L,
           static_cast<short>(9), std::string("str1"), 8ULL, 4.0f, std::string_view("view1"),
           std::string("str2"));
  LOG_INFO(logger, "lazy over fox logging {} {} {} {} {} {}", std::string_view("view2"), 5L, 1, 4.0f, 8ULL, true);
  LOG_INFO(logger, "over quick example {} {} {} {} {} {} {}", static_cast<unsigned short>(10), 1,
           "example1", 7UL, "example2", true, 5L);
  LOG_INFO(logger, "dog example over {} {} {} {}", 5L, 3.0, 7UL, 1);
  LOG_INFO(logger, "dog example fox {} {} {} {}", "example1", 3.0, static_cast<unsigned short>(10), 1);
  LOG_INFO(logger, "quick fox dog {}", 2);
  LOG_INFO(logger, "fox logging example {} {} {} {}", std::string("str1"), "example1", true, false);
  LOG_INFO(logger, "brown fox example {} {} {} {} {} {} {}", static_cast<short>(9), 7UL, "example3",
           8ULL, 6LL, false, std::string_view("view1"));
  LOG_INFO(logger, "jumps logging dog {} {} {}", std::string("str2"), std::string_view("view1"),
           std::string("str1"));
  LOG_INFO(logger, "logging lazy dog {} {} {} {} {}", 6LL, 5L, static_cast<unsigned short>(10),
           std::string("str1"), "example2");
  LOG_INFO(logger, "test fox logging {}", 7UL);
  LOG_INFO(logger, "logging lazy quick {} {}", 2, static_cast<short>(9));
  LOG_INFO(logger, "brown over example test {} {} {} {} {} {} {} {} {}", std::string("str2"), 6LL,
           "example1", 4.0f, std::string("str1"), false, 1, 3.0, 2);
  LOG_INFO(logger, "brown over example {} {} {} {} {} {} {} {} {} {}", 1, 5L,
           std::string_view("view1"), std::string("str1"), static_cast<short>(9), 3.0, "example2",
           static_cast<unsigned short>(10), "example1", 4.0f);
  LOG_INFO(logger, "test quick fox lazy {} {} {} {} {} {} {}", false,
           static_cast<unsigned short>(10), std::string("str2"), 3.0, 2, true, 1);
  LOG_INFO(logger, "brown logging dog example {} {} {} {} {} {}", 8ULL, 5L,
           std::string_view("view1"), 4.0f, static_cast<short>(9), std::string("str2"));
  LOG_INFO(logger, "logging brown dog {} {} {} {} {} {}", std::string("str1"),
           std::string_view("view1"), std::string_view("view2"), "example3", true, 2);
  LOG_INFO(logger, "quick fox lazy {}", 8ULL);
  LOG_INFO(logger, "test logging example brown {} {} {} {} {}", "example2", false,
           static_cast<unsigned short>(10), 2, true);
  LOG_INFO(logger, "brown jumps fox {} {} {} {} {} {} {} {} {} {}", 4.0f, 8ULL, 2, std::string_view("view2"),
           std::string("str2"), 1, "example2", "example3", std::string("str1"), 3.0);
  LOG_INFO(logger, "over dog quick {} {} {} {} {} {} {} {}", std::string("str1"), 5L, true,
           static_cast<unsigned short>(10), 7UL, std::string("str2"), std::string_view("view1"), 4.0f);
  LOG_INFO(logger, "test dog brown lazy {} {} {} {} {} {} {} {}", true, std::string_view("view2"),
           6LL, std::string_view("view1"), false, 8ULL, "example3", 3.0);
  LOG_INFO(logger, "example fox lazy {} {}", 5L, static_cast<unsigned short>(10));
  LOG_INFO(logger, "example logging over {} {} {} {} {} {} {} {}", 8ULL, 2, 6LL, "example2",
           "example1", std::string("str2"), 5L, 7UL);
  LOG_INFO(logger, "jumps test example {} {}", 1, 7UL);
  LOG_INFO(logger, "over dog test example {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           2, "example3", std::string_view("view2"), 8ULL, 5L, 3.0, static_cast<short>(9), 6LL);
  LOG_INFO(logger, "quick dog brown {} {} {} {} {} {} {} {} {}", static_cast<short>(9), 4.0f,
           std::string("str1"), true, 6LL, 7UL, 8ULL, "example1", std::string("str2"));
  LOG_INFO(logger, "logging dog fox brown {} {} {} {}", std::string("str1"), 3.0, "example3", 6LL);
  LOG_INFO(logger, "quick test over {} {} {} {} {} {} {}", 6LL, 8ULL, std::string_view("view1"), 1,
           std::string_view("view2"), 7UL, true);
  LOG_INFO(logger, "test over fox {} {} {} {}", 5L, static_cast<short>(9), 8ULL, 7UL);
  LOG_INFO(logger, "fox logging test {} {} {}", 1, 7UL, std::string_view("view1"));
  LOG_INFO(logger, "fox over quick {} {} {} {} {} {} {} {}", "example2", std::string_view("view2"),
           static_cast<short>(9), false, 7UL, "example3", "example1", 4.0f);
  LOG_INFO(logger, "fox quick test example {}", std::string("str2"));
  LOG_INFO(logger, "quick over dog {}", "example2");
  LOG_INFO(logger, "example quick fox {} {} {} {}", 7UL, std::string("str1"), "example3", 4.0f);
  LOG_INFO(logger, "example fox brown dog {} {} {} {} {} {} {} {} {}", 5L, static_cast<short>(9),
           3.0, "example1", 1, true, 7UL, false, std::string("str2"));
  LOG_INFO(logger, "lazy dog jumps quick {} {} {} {} {}", std::string_view("view1"), 1,
           static_cast<short>(9), false, 6LL);
  LOG_INFO(logger, "logging dog test {}", 6LL);
  LOG_INFO(logger, "fox jumps quick logging {} {} {} {} {} {} {} {} {} {}", true, 8ULL, 1, 5L,
           "example1", std::string("str1"), 6LL, static_cast<unsigned short>(10), 2, "example3");
  LOG_INFO(logger, "jumps over example {} {} {} {} {} {} {} {} {} {}",
           static_cast<unsigned short>(10), std::string_view("view1"), std::string_view("view2"),
           8ULL, 4.0f, 3.0, std::string("str1"), true, 1, "example1");
  LOG_INFO(logger, "over jumps logging {} {} {} {} {} {} {} {} {} {}", 4.0f, 5L, false,
           std::string_view("view1"), static_cast<short>(9), 2, 7UL, 6LL, "example2",
           std::string_view("view2"));
  LOG_INFO(logger, "quick example jumps {}", 5L);
  LOG_INFO(logger, "logging brown lazy {} {} {} {} {} {} {}", std::string_view("view1"), false,
           static_cast<unsigned short>(10), true, "example3", std::string("str2"), 3.0);
  LOG_INFO(logger, "lazy dog quick brown {} {} {} {} {} {} {} {} {} {}", 2, "example3", 7UL, 3.0,
           std::string("str1"), std::string_view("view2"), true, 6LL, std::string_view("view1"), false);
  LOG_INFO(logger, "fox logging example test {}", 1);
  LOG_INFO(logger, "test quick brown fox {} {} {} {} {} {}", "example1", static_cast<short>(9),
           4.0f, 1, 5L, std::string_view("view1"));
  LOG_INFO(logger, "dog logging test {} {} {} {}", std::string("str1"), "example1", true,
           std::string_view("view1"));
  LOG_INFO(logger, "test lazy quick example {} {} {} {} {}", 8ULL, std::string("str1"), 4.0f, 2,
           std::string_view("view1"));
  LOG_INFO(logger, "brown logging jumps dog {} {}", false, 8ULL);
  LOG_INFO(logger, "test lazy logging example {} {} {} {} {} {} {} {}", std::string("str2"), false,
           "example2", 7UL, "example3", 4.0f, std::string_view("view1"), static_cast<short>(9));
  LOG_INFO(logger, "lazy example jumps brown {} {} {}", 6LL, std::string_view("view2"), std::string("str1"));
  LOG_INFO(logger, "brown fox test over {} {} {} {} {} {} {} {} {} {}",
           static_cast<unsigned short>(10), 1, false, 6LL, static_cast<short>(9), 7UL,
           std::string_view("view2"), "example2", std::string("str2"), true);
  LOG_INFO(logger, "over example test {} {} {} {} {} {}", static_cast<short>(9), 1,
           std::string_view("view2"), 8ULL, static_cast<unsigned short>(10), 5L);
  LOG_INFO(logger, "dog fox test example {} {} {}", "example3", std::string_view("view1"), std::string("str2"));
  LOG_INFO(logger, "jumps test over logging {} {} {} {} {} {} {}", 4.0f, 6LL, 5L, 2, "example1", 1,
           std::string_view("view2"));
  LOG_INFO(logger, "brown test lazy {} {} {} {} {} {} {}", false, std::string("str1"), 4.0f,
           "example3", std::string("str2"), 8ULL, true);
  LOG_INFO(logger, "example jumps brown {} {} {} {} {} {} {} {} {} {}", 2, "example3", 4.0f, 7UL,
           8ULL, 3.0, static_cast<unsigned short>(10), 1, "example1", std::string("str2"));
  LOG_INFO(logger, "dog fox jumps {} {}", std::string("str1"), "example2");
  LOG_INFO(logger, "dog brown example {} {} {} {} {} {}", 1, std::string("str1"), 7UL, 6LL,
           std::string("str2"), 4.0f);
  LOG_INFO(logger, "jumps brown quick {} {}", static_cast<short>(9), true);
  LOG_INFO(logger, "brown example jumps {} {} {}", false, 5L, std::string_view("view1"));
  LOG_INFO(logger, "logging quick test {} {}", std::string_view("view2"), "example3");
  LOG_INFO(logger, "test over quick fox {} {} {} {} {}", 2, 5L, 3.0, true, "example2");
  LOG_INFO(logger, "over lazy dog brown {} {} {} {} {}", "example2", 3.0, 2, static_cast<short>(9), false);
  LOG_INFO(logger, "fox dog logging {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string_view("view2"), static_cast<short>(9), 6LL, 5L, 1);
  LOG_INFO(logger, "over logging brown quick {} {} {} {} {} {} {} {} {} {}", static_cast<short>(9),
           "example2", std::string_view("view1"), 8ULL, 2, std::string("str1"),
           static_cast<unsigned short>(10), 7UL, 6LL, "example3");
  LOG_INFO(logger, "brown logging lazy quick {} {} {} {} {} {}", 5L,
           static_cast<unsigned short>(10), static_cast<short>(9), std::string("str2"), 8ULL, true);
  LOG_INFO(logger, "example test quick {} {}", std::string("str1"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown over jumps {} {} {}", true, 1, std::string_view("view1"));
  LOG_INFO(logger, "dog over lazy example {} {} {} {}", "example1", "example3", 2, 7UL);
  LOG_INFO(logger, "fox test quick {} {} {} {} {} {} {} {} {} {}", 1, "example2", 7UL,
           static_cast<unsigned short>(10), 3.0, 2, 5L, std::string_view("view1"), 4.0f, 6LL);
  LOG_INFO(logger, "lazy test brown {} {} {}", 2, false, 5L);
  LOG_INFO(logger, "lazy example fox quick {} {} {} {} {} {} {}", false, 4.0f, 5L,
           std::string("str1"), "example3", 7UL, std::string_view("view1"));
  LOG_INFO(logger, "quick brown test dog {} {} {} {} {} {} {} {} {} {}", 7UL, "example2", false,
           8ULL, std::string("str2"), 6LL, 2, std::string("str1"), "example1", true);
  LOG_INFO(logger, "quick logging test over {} {} {} {}", std::string_view("view1"), "example1",
           static_cast<short>(9), 2);
  LOG_INFO(logger, "jumps logging dog {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           std::string("str2"), 6LL, 3.0, 2, static_cast<unsigned short>(10), false, 8ULL, true, 4.0f);
  LOG_INFO(logger, "quick logging over dog {} {} {} {} {} {} {} {}", 5L, true, 3.0, "example2", 2,
           7UL, "example1", std::string_view("view1"));
  LOG_INFO(logger, "example over lazy fox {} {} {} {}", std::string_view("view1"), 6LL, "example1",
           std::string_view("view2"));
  LOG_INFO(logger, "jumps brown quick {} {} {} {}", "example1", 3.0, 5L, std::string("str1"));
  LOG_INFO(logger, "lazy quick over jumps {} {} {}", 2, true, static_cast<short>(9));
  LOG_INFO(logger, "brown example dog {}", 5L);
  LOG_INFO(logger, "over brown logging dog {} {}", static_cast<unsigned short>(10), 6LL);
  LOG_INFO(logger, "test lazy over dog {} {} {} {} {}", 6LL, static_cast<short>(9), 3.0, false, 1);
  LOG_INFO(logger, "over lazy jumps brown {} {} {} {}", static_cast<short>(9), std::string("str2"), 8ULL, 5L);
  LOG_INFO(logger, "lazy fox example {} {} {}", 8ULL, std::string_view("view1"), static_cast<short>(9));
  LOG_INFO(logger, "logging jumps dog test {} {} {}", static_cast<short>(9), true, 7UL);
  LOG_INFO(logger, "test example over lazy {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           6LL, 7UL, static_cast<short>(9), "example1", true, std::string("str1"), false);
  LOG_INFO(logger, "lazy over test {} {} {} {} {} {} {} {} {}", std::string("str1"), 7UL,
           "example2", true, std::string_view("view1"), 6LL, "example1", 4.0f, std::string("str2"));
  LOG_INFO(logger, "jumps example lazy {} {}", "example3", std::string_view("view2"));
  LOG_INFO(logger, "fox logging lazy test {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           5L, "example2", 4.0f, 3.0, std::string_view("view2"), 2, "example1");
  LOG_INFO(logger, "fox over quick test {} {} {} {} {} {} {} {} {}", "example1",
           static_cast<unsigned short>(10), static_cast<short>(9), "example2", false,
           std::string_view("view2"), 6LL, "example3", 5L);
  LOG_INFO(logger, "test quick over logging {} {}", std::string_view("view2"), 6LL);
  LOG_INFO(logger, "logging test quick over {} {}", true, 6LL);
  LOG_INFO(logger, "lazy quick fox {} {} {} {} {} {} {} {}", 4.0f, false, 8ULL, std::string_view("view1"),
           6LL, 3.0, std::string_view("view2"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog lazy test jumps {} {} {} {} {} {} {}", 2, 8ULL, 1,
           static_cast<unsigned short>(10), "example1", true, static_cast<short>(9));
  LOG_INFO(logger, "lazy dog test fox {}", std::string("str1"));
  LOG_INFO(logger, "test jumps example brown {} {} {} {} {} {} {}", false, 4.0f, 2,
           std::string_view("view1"), "example1", 6LL, std::string("str2"));
  LOG_INFO(logger, "fox dog lazy logging {} {} {} {} {} {} {} {} {}", "example3",
           std::string("str2"), 8ULL, "example1", false, "example2", 4.0f, 3.0, std::string("str1"));
  LOG_INFO(logger, "fox test example brown {} {}", std::string("str1"), "example2");
  LOG_INFO(logger, "test quick logging {}", std::string("str1"));
  LOG_INFO(logger, "brown fox example jumps {} {} {} {} {} {} {} {}", std::string_view("view1"),
           7UL, false, 6LL, std::string("str1"), 8ULL, std::string_view("view2"), "example3");
  LOG_INFO(logger, "lazy logging fox jumps {} {} {} {} {}", std::string_view("view1"), "example2",
           "example3", false, static_cast<short>(9));
  LOG_INFO(logger, "dog jumps example over {}", true);
  LOG_INFO(logger, "brown lazy quick {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string("str1"), true, std::string("str2"), std::string_view("view2"), 4.0f,
           "example3", false);
  LOG_INFO(logger, "dog fox jumps logging {} {} {}", 6LL, 5L, 1);
  LOG_INFO(logger, "jumps brown example quick {} {} {} {} {} {}", std::string_view("view1"), 7UL,
           "example3", 8ULL, 6LL, 3.0);
  LOG_INFO(logger, "quick example jumps lazy {} {} {} {} {}", static_cast<unsigned short>(10),
           false, std::string_view("view1"), 1, 7UL);
  LOG_INFO(logger, "dog lazy quick over {}", "example3");
  LOG_INFO(logger, "over dog brown {} {} {} {} {} {}", 8ULL, 7UL, std::string("str2"), "example3",
           1, std::string_view("view1"));
  LOG_INFO(logger, "test quick dog lazy {} {} {}", "example3", "example1", "example2");
  LOG_INFO(logger, "dog fox jumps over {} {}", static_cast<unsigned short>(10), "example3");
  LOG_INFO(logger, "jumps fox lazy dog {} {} {} {} {} {} {} {} {}", 5L, 2, "example3", "example2",
           static_cast<unsigned short>(10), 3.0, std::string("str2"), 6LL, 8ULL);
  LOG_INFO(logger, "over quick fox {} {} {} {} {} {} {}", std::string_view("view2"),
           static_cast<unsigned short>(10), 4.0f, 8ULL, 2, 1, std::string("str2"));
  LOG_INFO(logger, "dog test lazy {} {} {} {}", 8ULL, std::string_view("view1"), "example3", 3.0);
  LOG_INFO(logger, "fox jumps example dog {} {}", "example2", static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging lazy jumps example {} {} {} {} {} {} {}", std::string_view("view2"),
           static_cast<unsigned short>(10), std::string_view("view1"), 7UL, true, 5L, 4.0f);
  LOG_INFO(logger, "dog example fox {} {}", 6LL, std::string("str2"));
  LOG_INFO(logger, "logging dog quick {} {} {} {} {}", static_cast<short>(9), true, false,
           std::string_view("view1"), std::string("str1"));
  LOG_INFO(logger, "jumps example over {} {} {} {} {} {} {}", 3.0, false, std::string_view("view1"),
           4.0f, std::string("str2"), 5L, std::string_view("view2"));
  LOG_INFO(logger, "example jumps test brown {} {} {} {} {} {}", std::string("str2"), 6LL, 7UL,
           static_cast<short>(9), "example2", 8ULL);
  LOG_INFO(logger, "dog logging quick {} {} {} {} {} {} {}", "example1", std::string("str2"), 8ULL,
           "example2", "example3", static_cast<unsigned short>(10), 3.0);
  LOG_INFO(logger, "quick logging brown {} {} {} {} {}", true, 3.0, static_cast<unsigned short>(10),
           5L, std::string("str1"));
  LOG_INFO(logger, "quick lazy dog brown {} {} {} {} {} {} {} {} {}", 7UL, static_cast<unsigned short>(10),
           3.0, 1, "example2", "example1", 6LL, false, static_cast<short>(9));
  LOG_INFO(logger, "jumps quick over {} {} {} {} {} {}", std::string("str2"), 4.0f, "example3", 1,
           true, "example2");
  LOG_INFO(logger, "quick dog example jumps {} {} {} {} {} {} {} {}", "example1", 2, 6LL, false,
           static_cast<unsigned short>(10), 5L, 1, std::string_view("view1"));
  LOG_INFO(logger, "jumps dog quick test {}", 2);
  LOG_INFO(logger, "logging fox example {} {} {} {} {} {} {} {} {}", true, 2,
           static_cast<unsigned short>(10), std::string("str1"), std::string("str2"),
           std::string_view("view1"), "example1", 1, static_cast<short>(9));
  LOG_INFO(logger, "brown test dog {} {} {} {} {}", "example3", std::string_view("view1"), false,
           6LL, std::string("str1"));
  LOG_INFO(logger, "over example logging brown {} {} {} {}", "example1", 1, 6LL, 7UL);
  LOG_INFO(logger, "logging jumps test example {} {} {}", 1, 8ULL, 6LL);
  LOG_INFO(logger, "logging brown lazy example {} {}", 6LL, static_cast<short>(9));
  LOG_INFO(logger, "quick jumps lazy example {} {}", 7UL, 2);
  LOG_INFO(logger, "brown over logging lazy {} {}", std::string("str2"), 7UL);
  LOG_INFO(logger, "logging brown fox {}", std::string_view("view2"));
  LOG_INFO(logger, "quick fox test {} {} {}", "example2", 5L, static_cast<unsigned short>(10));
  LOG_INFO(logger, "quick test over brown {} {} {}", std::string_view("view1"), true, 2);
  LOG_INFO(logger, "brown lazy jumps {} {} {} {} {} {} {}", 5L, "example2", 3.0, true, 4.0f,
           std::string("str1"), static_cast<short>(9));
  LOG_INFO(logger, "test jumps lazy logging {} {} {}", std::string_view("view2"), static_cast<short>(9), 7UL);
  LOG_INFO(logger, "brown quick dog jumps {}", false);
  LOG_INFO(logger, "fox example test brown {} {} {} {} {} {}", 1, std::string_view("view2"), 3.0,
           false, 8ULL, "example1");
  LOG_INFO(logger, "dog quick brown test {} {} {} {}", std::string_view("view2"), "example1",
           std::string_view("view1"), 5L);
  LOG_INFO(logger, "quick lazy example fox {} {} {} {} {} {} {} {} {} {}", 1, 2, 8ULL, false, 6LL,
           true, "example2", "example1", 4.0f, static_cast<short>(9));
  LOG_INFO(logger, "lazy dog brown fox {} {} {} {}", 6LL, true, std::string("str2"), 3.0);
  LOG_INFO(logger, "quick test brown fox {} {} {} {} {}", std::string("str2"),
           std::string_view("view2"), false, "example3", 5L);
  LOG_INFO(logger, "dog jumps brown {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog test example jumps {} {} {} {} {} {}", static_cast<unsigned short>(10),
           true, 8ULL, std::string("str1"), std::string_view("view1"), 2);
  LOG_INFO(logger, "brown logging fox {} {} {} {} {} {} {} {}", "example1", static_cast<short>(9),
           "example3", "example2", true, static_cast<unsigned short>(10), std::string("str2"), 5L);
  LOG_INFO(logger, "lazy dog jumps {} {} {} {} {} {} {} {} {} {}", 7UL, 5L, std::string_view("view2"),
           std::string("str1"), true, 3.0, std::string("str2"), "example1", 8ULL, static_cast<short>(9));
  LOG_INFO(logger, "dog lazy quick {} {} {} {} {} {} {}", 8ULL, 6LL, true, 2, 1, 4.0f, "example2");
  LOG_INFO(logger, "logging over brown example {} {} {} {} {} {} {} {} {}", true, 3.0, 8ULL,
           "example1", std::string_view("view2"), 5L, "example2", 6LL, std::string_view("view1"));
  LOG_INFO(logger, "logging example test {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           5L, "example3", 8ULL, 2, "example2", 7UL, std::string_view("view1"),
           static_cast<short>(9), std::string("str2"));
  LOG_INFO(logger, "jumps over example test {} {} {} {} {} {} {} {} {}", 7UL, std::string("str2"), true,
           1, "example2", "example1", static_cast<unsigned short>(10), std::string_view("view1"), 6LL);
  LOG_INFO(logger, "brown lazy over {} {} {} {} {} {}", static_cast<short>(9), "example1",
           "example2", 8ULL, 3.0, std::string("str1"));
  LOG_INFO(logger, "logging jumps test {} {}", 1, "example1");
  LOG_INFO(logger, "quick brown logging {} {} {} {} {} {} {}", 1, 4.0f, 7UL, "example3",
           std::string_view("view2"), static_cast<short>(9), 3.0);
  LOG_INFO(logger, "jumps dog over test {} {} {} {} {} {} {} {} {}", std::string_view("view2"), false,
           std::string_view("view1"), 7UL, "example1", "example2", static_cast<short>(9), 4.0f, 6LL);
  LOG_INFO(logger, "over dog quick fox {} {} {} {} {} {}", std::string("str2"),
           std::string_view("view1"), "example3", std::string("str1"), "example2", 5L);
  LOG_INFO(logger, "quick test fox brown {} {}", 8ULL, static_cast<unsigned short>(10));
  LOG_INFO(logger, "example brown jumps quick {} {} {} {} {} {}", std::string_view("view2"), false,
           1, 6LL, 8ULL, 3.0);
  LOG_INFO(logger, "brown example test quick {} {}", static_cast<short>(9), 4.0f);
  LOG_INFO(logger, "example dog lazy {} {} {} {} {} {} {} {} {} {}", 5L, 2, std::string("str2"),
           static_cast<unsigned short>(10), 1, std::string_view("view2"), false, 4.0f, 6LL,
           std::string_view("view1"));
  LOG_INFO(logger, "over logging dog {} {} {} {} {} {} {} {} {} {}", 7UL, 5L, 8ULL, 3.0,
           static_cast<unsigned short>(10), "example1", std::string_view("view1"),
           std::string_view("view2"), true, std::string("str1"));
  LOG_INFO(logger, "quick test example {} {} {} {} {} {} {} {}", 8ULL, 7UL, false, "example1",
           "example3", std::string("str2"), 2, 6LL);
  LOG_INFO(logger, "brown test lazy fox {} {} {} {} {} {} {} {} {} {}", "example3", 6LL, "example2",
           std::string_view("view1"), 8ULL, static_cast<short>(9), 4.0f, 3.0, 5L, false);
  LOG_INFO(logger, "over test fox quick {} {} {} {} {} {} {}", 3.0, 2, std::string("str2"),
           static_cast<short>(9), 1, std::string_view("view1"), 8ULL);
  LOG_INFO(logger, "test brown jumps {} {} {} {} {} {} {} {}", 7UL, "example2", std::string("str1"),
           true, 6LL, false, 8ULL, "example3");
  LOG_INFO(logger, "over brown quick {} {} {} {} {} {} {} {}", 2, 7UL, "example1",
           static_cast<unsigned short>(10), std::string_view("view2"), std::string("str1"), 1, 3.0);
  LOG_INFO(logger, "quick jumps over logging {} {} {} {} {} {} {} {} {}", 5L, false, 3.0,
           std::string("str1"), "example1", 7UL, static_cast<unsigned short>(10),
           std::string("str2"), static_cast<short>(9));
  LOG_INFO(logger, "dog over jumps {} {} {} {} {} {} {} {}", "example2", "example3", 6LL, false,
           std::string_view("view1"), 2, 3.0, "example1");
  LOG_INFO(logger, "lazy quick test logging {} {} {}", false, std::string_view("view1"), 8ULL);
  LOG_INFO(logger, "fox quick over {} {} {} {} {} {} {} {} {} {}", "example1", 6LL, 4.0f,
           std::string_view("view1"), false, 1, 3.0, 7UL, std::string_view("view2"), 5L);
  LOG_INFO(logger, "jumps lazy over logging {} {} {} {} {} {} {} {} {}", std::string("str1"),
           "example1", 3.0, "example2", "example3", true, 4.0f, std::string_view("view1"), 6LL);
  LOG_INFO(logger, "fox example quick {} {} {} {} {} {} {} {} {} {}", static_cast<short>(9),
           "example3", false, 5L, 7UL, 3.0, 8ULL, std::string_view("view1"),
           static_cast<unsigned short>(10), 4.0f);
  LOG_INFO(logger, "jumps over dog brown {} {} {}", 3.0, 8ULL, "example1");
  LOG_INFO(logger, "jumps quick test {} {} {} {} {} {} {} {}", 7UL, 4.0f, "example2",
           std::string("str2"), 3.0, false, 8ULL, 5L);
  LOG_INFO(logger, "brown logging over fox {} {}", 5L, 6LL);
  LOG_INFO(logger, "jumps dog over quick {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string("str1"), "example2", "example1", std::string_view("view2"), "example3", 1);
  LOG_INFO(logger, "over quick brown {} {} {} {} {} {} {} {}", 8ULL, 6LL, std::string("str2"), true,
           "example2", 1, std::string_view("view2"), std::string_view("view1"));
  LOG_INFO(logger, "logging quick brown fox {} {} {} {} {} {} {}", std::string_view("view1"), 1,
           false, std::string("str1"), 6LL, "example2", std::string_view("view2"));
  LOG_INFO(logger, "jumps fox logging {} {}", static_cast<unsigned short>(10), static_cast<short>(9));
  LOG_INFO(logger, "fox example dog {} {} {} {}", static_cast<unsigned short>(10),
           std::string("str2"), true, "example3");
  LOG_INFO(logger, "brown quick logging fox {} {} {} {} {} {} {} {} {}", 8ULL, 5L, static_cast<short>(9),
           "example1", 6LL, std::string("str1"), std::string_view("view2"), "example2", true);
  LOG_INFO(logger, "lazy dog test {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           static_cast<unsigned short>(10), std::string_view("view1"), false, 8ULL, "example3", 3.0,
           2, std::string("str1"));
  LOG_INFO(logger, "lazy test example over {}", "example1");
  LOG_INFO(logger, "test brown fox example {} {} {} {} {} {} {} {} {} {}", 3.0, true, std::string("str2"),
           8ULL, 1, "example1", false, static_cast<short>(9), "example3", "example2");
  LOG_INFO(logger, "example lazy jumps {} {} {} {} {} {} {} {} {} {}", 2, 5L, "example2", 4.0f,
           static_cast<short>(9), 6LL, static_cast<unsigned short>(10), true, 1, 7UL);
  LOG_INFO(logger, "fox test jumps {} {} {} {} {} {} {} {} {}", std::string("str1"), "example2", 4.0f,
           std::string_view("view1"), 6LL, 1, static_cast<short>(9), 5L, std::string_view("view2"));
  LOG_INFO(logger, "brown lazy example {} {} {} {} {} {} {} {} {}", "example2",
           std::string_view("view1"), "example1", true, static_cast<short>(9), 5L, 2, 1, 8ULL);
  LOG_INFO(logger, "fox brown dog test {} {} {}", std::string_view("view2"), false, 4.0f);
  LOG_INFO(logger, "example test brown {} {} {} {}", 6LL, 8ULL, true, 1);
  LOG_INFO(logger, "over example logging brown {} {}", 5L, std::string("str1"));
  LOG_INFO(logger, "example dog test {} {} {} {} {} {}", false, "example2", 4.0f,
           std::string("str1"), "example3", 2);
  LOG_INFO(logger, "over quick logging {} {} {}", 1, std::string_view("view2"), 8ULL);
  LOG_INFO(logger, "logging jumps example {} {} {} {} {} {} {} {}", 7UL, std::string("str2"), 6LL,
           "example2", 5L, 8ULL, false, std::string("str1"));
  LOG_INFO(logger, "jumps dog quick {} {}", 8ULL, 5L);
  LOG_INFO(logger, "logging fox test {} {} {} {}", 7UL, false, 2, std::string_view("view2"));
  LOG_INFO(logger, "fox over dog quick {} {} {} {} {} {} {} {}", 7UL, "example1",
           std::string_view("view1"), true, 2, 4.0f, 6LL, false);
  LOG_INFO(logger, "test jumps quick {} {} {} {} {} {} {}", 2, false, "example1", 8ULL, 6LL,
           static_cast<unsigned short>(10), 4.0f);
  LOG_INFO(logger, "jumps logging dog fox {}", std::string_view("view1"));
  LOG_INFO(logger, "fox lazy test {} {} {} {} {} {} {} {}", std::string_view("view1"), 3.0,
           "example1", true, std::string("str2"), 5L, 4.0f, static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox brown jumps {} {} {} {} {} {} {}", std::string("str1"), 7UL,
           static_cast<short>(9), std::string_view("view1"), false, 1, "example3");
  LOG_INFO(logger, "over example fox {} {} {} {}", std::string_view("view1"), "example1", 8ULL,
           std::string_view("view2"));
  LOG_INFO(logger, "example lazy quick {} {} {} {} {} {} {}", 8ULL, 2,
           static_cast<unsigned short>(10), 3.0, std::string("str2"), 4.0f, 7UL);
  LOG_INFO(logger, "logging fox brown jumps {} {} {} {} {} {} {} {} {}", std::string_view("view1"),
           8ULL, std::string("str2"), true, std::string_view("view2"), 7UL, 3.0, false, "example2");
  LOG_INFO(logger, "over brown quick {} {}", true, 1);
  LOG_INFO(logger, "brown dog jumps {} {} {}", false, 8ULL, 2);
  LOG_INFO(logger, "dog brown quick lazy {} {} {} {} {}", "example3", "example1", 8ULL, 3.0,
           "example2");
  LOG_INFO(logger, "fox quick brown over {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           "example2", 2, 8ULL, "example1", std::string("str1"), 4.0f, false, 3.0);
  LOG_INFO(logger, "over fox lazy {} {} {} {} {} {}", 5L, 4.0f, std::string("str1"), 8ULL,
           std::string_view("view2"), 3.0);
  LOG_INFO(logger, "lazy dog test {} {} {} {} {} {} {} {}", 2, "example2", false, 6LL,
           std::string("str1"), 8ULL, static_cast<short>(9), 1);
  LOG_INFO(logger, "lazy quick brown logging {} {}", 1, 4.0f);
  LOG_INFO(logger, "over quick fox {} {} {} {} {} {}", std::string_view("view2"),
           static_cast<unsigned short>(10), 2, std::string("str2"), std::string("str1"), 3.0);
  LOG_INFO(logger, "test dog jumps lazy {} {} {}", 6LL, "example2", 7UL);
  LOG_INFO(logger, "lazy dog jumps over {}", 7UL);
  LOG_INFO(logger, "logging jumps fox lazy {} {} {}", false, 6LL, 4.0f);
  LOG_INFO(logger, "jumps fox example {} {}", "example2", std::string_view("view2"));
  LOG_INFO(logger, "quick jumps test dog {} {} {} {} {} {}", 7UL, 8ULL, 1, 6LL, 4.0f, std::string_view("view1"));
  LOG_INFO(logger, "brown example dog {} {} {} {} {} {}", 4.0f, false, std::string("str1"),
           static_cast<short>(9), 6LL, true);
  LOG_INFO(logger, "brown logging example {} {} {} {}", static_cast<unsigned short>(10), 7UL, false, 3.0);
  LOG_INFO(logger, "quick fox test {} {} {} {} {} {}", 3.0, std::string_view("view2"), true, false,
           std::string("str1"), std::string("str2"));
  LOG_INFO(logger, "lazy quick over {} {} {} {} {} {} {}", 6LL, false, static_cast<short>(9), true,
           7UL, 5L, std::string_view("view2"));
  LOG_INFO(logger, "jumps lazy over {} {}", "example3", 2);
  LOG_INFO(logger, "test dog jumps logging {} {} {} {} {} {} {} {} {} {}", false, 8ULL,
           static_cast<short>(9), 7UL, std::string_view("view2"), "example1", 1, 5L, "example3", 2);
  LOG_INFO(logger, "brown logging quick example {} {} {} {} {} {} {} {}", 2, "example3",
           static_cast<unsigned short>(10), std::string_view("view1"), true, std::string("str2"), false, 5L);
  LOG_INFO(logger, "over dog jumps quick {} {} {}", 3.0, 4.0f, 8ULL);
  LOG_INFO(logger, "fox lazy test quick {} {} {} {} {} {} {} {}", 5L, 4.0f, "example2",
           std::string("str2"), 6LL, static_cast<unsigned short>(10), 7UL, std::string_view("view1"));
  LOG_INFO(logger, "jumps over logging lazy {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           6LL, 7UL, 1, 5L, 4.0f, std::string_view("view2"), false, "example1", 3.0);
  LOG_INFO(logger, "lazy over test {} {}", std::string("str1"), 3.0);
  LOG_INFO(logger, "fox brown over {} {} {} {} {} {} {} {}", 6LL, std::string("str1"),
           static_cast<short>(9), 2, "example1", 5L, false, std::string_view("view2"));
  LOG_INFO(logger, "brown lazy fox {}", static_cast<short>(9));
  LOG_INFO(logger, "fox dog brown {} {} {} {} {}", true, 8ULL, std::string_view("view2"),
           std::string("str1"), 6LL);
  LOG_INFO(logger, "dog example over {} {} {} {}", static_cast<unsigned short>(10), 6LL, 1, 3.0);
  LOG_INFO(logger, "jumps dog brown {} {} {} {} {} {} {} {}", std::string_view("view1"),
           static_cast<short>(9), true, std::string("str2"), 8ULL, 2, 4.0f, "example3");
  LOG_INFO(logger, "jumps test logging {} {}", "example3", std::string_view("view1"));
  LOG_INFO(logger, "example dog logging over {} {} {} {} {} {} {} {} {} {}", 2,
           static_cast<short>(9), 5L, "example1", 6LL, true, std::string("str2"), 4.0f, 8ULL, 1);
  LOG_INFO(logger, "dog logging lazy example {} {} {} {} {} {} {} {} {} {}", std::string("str2"), 1,
           5L, 3.0, std::string("str1"), static_cast<unsigned short>(10), std::string_view("view1"),
           6LL, 7UL, static_cast<short>(9));
  LOG_INFO(logger, "jumps logging fox brown {} {} {} {} {} {}", false, 4.0f, std::string("str1"),
           std::string_view("view1"), "example2", 3.0);
  LOG_INFO(logger, "fox brown example {} {} {} {} {}", 1, 5L, "example2",
           static_cast<unsigned short>(10), std::string("str1"));
  LOG_INFO(logger, "test fox example brown {} {} {} {} {} {} {} {} {} {}", std::string("str1"),
           "example2", 7UL, 2, 8ULL, std::string_view("view1"), static_cast<unsigned short>(10),
           true, false, 4.0f);
  LOG_INFO(logger, "lazy brown jumps {} {} {} {} {} {} {} {} {} {}", 2, std::string("str1"),
           std::string("str2"), 4.0f, 1, "example3", static_cast<unsigned short>(10), true, false,
           "example1");
  LOG_INFO(logger, "quick jumps test lazy {} {} {} {} {}", 1, static_cast<short>(9), 7UL,
           std::string("str1"), 4.0f);
  LOG_INFO(logger, "example jumps over lazy {} {} {} {} {} {} {}", 8ULL, true,
           std::string_view("view2"), 4.0f, static_cast<short>(9), std::string_view("view1"), 6LL);
  LOG_INFO(logger, "brown fox dog logging {} {} {} {} {} {} {} {} {} {}", 2, std::string("str2"),
           "example1", 4.0f, 5L, false, 7UL, 1, 6LL, 8ULL);
  LOG_INFO(logger, "lazy quick fox over {} {}", std::string_view("view2"), 4.0f);
  LOG_INFO(logger, "jumps over example dog {} {}", std::string_view("view2"), static_cast<short>(9));
  LOG_INFO(logger, "brown example over {} {}", std::string_view("view1"), std::string("str1"));
  LOG_INFO(logger, "test example fox logging {} {} {} {}", 4.0f, std::string("str1"), "example1",
           "example3");
  LOG_INFO(logger, "fox over lazy test {} {} {}", "example1", 3.0, static_cast<short>(9));
  LOG_INFO(logger, "example dog lazy {} {} {} {} {}", "example1", 6LL, 7UL, 4.0f, 5L);
  LOG_INFO(logger, "lazy logging over {} {} {} {} {} {} {} {} {}", false, static_cast<short>(9),
           static_cast<unsigned short>(10), 8ULL, std::string("str2"), 2, 5L, "example1", std::string("str1"));
  LOG_INFO(logger, "test quick jumps {} {} {} {} {}", true, "example2", std::string_view("view1"),
           std::string("str1"), "example3");
  LOG_INFO(logger, "test brown dog {} {} {} {} {}", 8ULL, 2, 6LL, static_cast<unsigned short>(10),
           "example1");
  LOG_INFO(logger, "logging test brown {} {} {} {} {} {} {} {}", 1, "example2",
           static_cast<unsigned short>(10), std::string("str2"), 3.0, 6LL, static_cast<short>(9),
           std::string("str1"));
  LOG_INFO(logger, "test jumps lazy {} {} {} {} {} {} {}", 4.0f, "example1", 6LL, "example2", 2, 3.0, 8ULL);
  LOG_INFO(logger, "brown over dog {} {} {} {} {} {} {} {} {}", 1, 6LL, 3.0, std::string("str1"),
           std::string_view("view2"), 5L, static_cast<unsigned short>(10), false, static_cast<short>(9));
  LOG_INFO(logger, "logging brown over example {} {} {} {} {} {} {} {} {}", "example3", 3.0, true,
           false, std::string_view("view1"), 7UL, 6LL, std::string("str2"), "example2");
  LOG_INFO(logger, "jumps test quick {} {} {} {} {} {} {}", false, 4.0f, static_cast<short>(9),
           std::string_view("view2"), "example3", 3.0, std::string("str2"));
  LOG_INFO(logger, "dog jumps test example {} {} {} {} {} {}", 5L, static_cast<unsigned short>(10),
           4.0f, 8ULL, std::string_view("view1"), std::string("str1"));
  LOG_INFO(logger, "brown over lazy example {} {} {} {} {} {} {} {}", std::string("str1"),
           "example3", std::string_view("view1"), 5L, std::string_view("view2"), "example2", 2, true);
  LOG_INFO(logger, "lazy dog test {} {} {}", true, 1, 2);
  LOG_INFO(logger, "fox brown example {} {} {}", std::string("str1"), static_cast<unsigned short>(10), 2);
  LOG_INFO(logger, "example dog quick brown {} {} {} {} {} {} {} {}", 8ULL, 4.0f, 7UL, true,
           std::string("str2"), std::string("str1"), std::string_view("view2"), 2);
  LOG_INFO(logger, "example dog quick {}", true);
  LOG_INFO(logger, "test dog quick {} {} {} {} {}", std::string_view("view2"), 5L, 8ULL, 7UL, 6LL);
  LOG_INFO(logger, "brown lazy jumps {} {} {} {} {} {} {}", 1, 2, 3.0, std::string("str1"),
           "example1", std::string_view("view1"), true);
  LOG_INFO(logger, "fox over dog {} {} {} {} {} {} {} {}", "example3", std::string_view("view2"), 1,
           true, "example1", std::string("str2"), "example2", static_cast<unsigned short>(10));
  LOG_INFO(logger, "over brown quick {} {} {}", false, 8ULL, 4.0f);
  LOG_INFO(logger, "quick example jumps fox {}", 6LL);
  LOG_INFO(logger, "logging jumps quick {} {}", std::string("str2"), "example1");
  LOG_INFO(logger, "dog logging lazy {} {} {} {}", std::string_view("view2"), true, std::string("str1"), 8ULL);
  LOG_INFO(logger, "example fox jumps dog {} {} {} {} {} {}", true, 3.0, 1, 7UL, "example3", false);
  LOG_INFO(logger, "test example lazy {} {} {} {} {} {} {} {} {}", std::string_view("view1"), 3.0,
           1, 4.0f, 2, static_cast<unsigned short>(10), std::string("str1"), true, "example1");
  LOG_INFO(logger, "logging dog test example {} {} {} {} {}", "example2", 4.0f, 8ULL, true,
           std::string_view("view1"));
  LOG_INFO(logger, "lazy example quick {} {} {} {} {} {} {} {} {} {}", false, 8ULL, 3.0, 6LL,
           "example1", true, std::string_view("view1"), static_cast<unsigned short>(10), "example3",
           std::string("str1"));
  LOG_INFO(logger, "fox quick lazy {} {} {}", 7UL, 3.0, 1);
  LOG_INFO(logger, "brown quick fox test {} {} {} {} {} {} {}", std::string_view("view2"), 1, 6LL,
           5L, std::string("str2"), static_cast<unsigned short>(10), true);
  LOG_INFO(logger, "fox test logging {} {} {} {} {} {} {} {} {} {}", 5L, 1, 8ULL, std::string("str1"),
           "example3", std::string_view("view1"), 4.0f, std::string("str2"), "example2", false);
  LOG_INFO(logger, "brown fox jumps over {} {} {} {} {} {} {}", std::string("str1"), 5L, 3.0,
           std::string_view("view2"), static_cast<unsigned short>(10), false, std::string_view("view1"));
  LOG_INFO(logger, "test lazy example {} {} {} {}", 6LL, 1, 5L, "example1");
  LOG_INFO(logger, "over dog brown {} {} {}", std::string("str2"), 4.0f, static_cast<short>(9));
  LOG_INFO(logger, "example brown test {} {} {}", 4.0f, true, 3.0);
  LOG_INFO(logger, "lazy test logging fox {} {}", 5L, 3.0);
  LOG_INFO(logger, "example fox lazy quick {} {} {} {} {} {} {} {}", 6LL, 7UL, true,
           static_cast<unsigned short>(10), 3.0, std::string("str2"), "example1", std::string_view("view2"));
  LOG_INFO(logger, "brown over test fox {} {} {} {} {} {} {} {} {}", 6LL, "example2", 3.0, 4.0f, 1,
           std::string_view("view1"), std::string("str2"), false, true);
  LOG_INFO(logger, "lazy over example {}", 4.0f);
  LOG_INFO(logger, "lazy example fox {} {} {} {} {} {} {} {} {}", "example2", "example3", 3.0, 8ULL,
           1, std::string("str1"), 7UL, std::string_view("view2"), std::string("str2"));
  LOG_INFO(logger, "brown test lazy quick {} {} {} {} {} {} {} {}", std::string("str2"),
           static_cast<short>(9), "example3", "example1", 8ULL, 4.0f, static_cast<unsigned short>(10), 7UL);
  LOG_INFO(logger, "fox example logging {} {} {} {} {} {} {}", 3.0, 5L, 8ULL,
           std::string_view("view2"), 2, std::string("str2"), 4.0f);
  LOG_INFO(logger, "brown fox over lazy {} {} {} {} {} {}", false, static_cast<short>(9),
           std::string_view("view1"), "example2", true, 6LL);
  LOG_INFO(logger, "quick logging over {} {} {} {} {} {} {}", 5L, 4.0f, std::string("str2"),
           "example1", 2, std::string_view("view1"), 7UL);
  LOG_INFO(logger, "example lazy brown dog {} {} {} {} {} {} {} {} {}", "example1", false, 3.0,
           4.0f, "example3", std::string("str1"), "example2", 7UL, true);
  LOG_INFO(logger, "over dog test lazy {} {}", static_cast<unsigned short>(10), 6LL);
  LOG_INFO(logger, "quick over dog example {} {} {} {}", 3.0, false, "example3", 7UL);
  LOG_INFO(logger, "example test quick {} {} {} {} {} {} {} {} {} {}", true,
           std::string_view("view1"), 4.0f, std::string("str2"), 2, static_cast<unsigned short>(10),
           3.0, 7UL, 1, std::string_view("view2"));
  LOG_INFO(logger, "over dog quick {} {} {} {} {} {} {} {} {} {}", 4.0f, std::string("str1"), 7UL,
           2, 6LL, static_cast<unsigned short>(10), 8ULL, std::string_view("view1"), true, 3.0);
  LOG_INFO(logger, "lazy test fox {} {}", 5L, 1);
  LOG_INFO(logger, "test over dog {} {} {} {} {} {}", std::string("str2"), "example2", 7UL,
           std::string_view("view2"), 8ULL, false);
  LOG_INFO(logger, "brown fox test jumps {} {} {} {} {} {}", std::string_view("view2"),
           static_cast<unsigned short>(10), 4.0f, 2, std::string("str1"), 1);
  LOG_INFO(logger, "over fox lazy {} {} {} {} {} {} {} {} {}", false, 2, static_cast<short>(9),
           4.0f, std::string("str2"), "example3", 1, std::string("str1"), std::string_view("view1"));
  LOG_INFO(logger, "fox example quick logging {} {}", "example2", 6LL);
  LOG_INFO(logger, "jumps dog fox {} {}", 6LL, 4.0f);
  LOG_INFO(logger, "dog brown test logging {} {} {} {}", std::string("str2"),
           std::string_view("view2"), 1, std::string_view("view1"));
  LOG_INFO(logger, "over brown dog test {} {} {} {} {} {} {}", 3.0, std::string("str1"), "example1",
           "example2", 6LL, 7UL, static_cast<short>(9));
  LOG_INFO(logger, "jumps example lazy brown {} {} {} {}", "example3", 4.0f,
           std::string_view("view1"), "example1");
  LOG_INFO(logger, "over example dog {} {} {} {}", true, 1, false, std::string_view("view1"));
  LOG_INFO(logger, "test over logging {} {} {} {} {} {} {} {} {}", 5L, std::string_view("view2"),
           6LL, std::string_view("view1"), 1, true, "example1", std::string("str2"), "example2");
  LOG_INFO(logger, "lazy example jumps quick {} {} {} {} {} {} {} {}", std::string_view("view2"),
           std::string_view("view1"), static_cast<unsigned short>(10), std::string("str2"),
           "example1", static_cast<short>(9), 7UL, 6LL);
  LOG_INFO(logger, "jumps quick over brown {} {} {} {} {}", static_cast<short>(9), true, 2,
           std::string_view("view2"), "example3");
  LOG_INFO(logger, "lazy dog logging {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           static_cast<unsigned short>(10), 5L, std::string_view("view1"), "example2", 8ULL, 6LL,
           static_cast<short>(9), 3.0);
  LOG_INFO(logger, "quick jumps brown {} {} {} {} {} {} {} {} {} {}", 1, true,
           std::string_view("view1"), 3.0, "example2", 4.0f, std::string("str2"), "example1",
           std::string_view("view2"), std::string("str1"));
  LOG_INFO(logger, "over brown logging dog {} {} {} {} {}", std::string("str1"), 3.0, 8ULL,
           static_cast<short>(9), false);
  LOG_INFO(logger, "jumps brown over {}", "example1");
  LOG_INFO(logger, "lazy test quick {} {} {} {} {}", 2, false, true, static_cast<unsigned short>(10), 1);
  LOG_INFO(logger, "dog fox lazy brown {} {} {}", 6LL, std::string("str1"), "example3");
  LOG_INFO(logger, "test jumps logging {} {} {} {} {} {} {} {}", 3.0, static_cast<short>(9), 7UL,
           std::string("str2"), 1, "example1", 8ULL, true);
  LOG_INFO(logger, "quick lazy jumps fox {} {} {} {}", "example2", std::string_view("view2"),
           "example1", 4.0f);
  LOG_INFO(logger, "logging jumps brown {} {}", "example1", 5L);
  LOG_INFO(logger, "brown example over {} {} {} {} {} {} {}", std::string("str1"),
           std::string("str2"), 8ULL, 3.0, static_cast<short>(9), true, std::string_view("view1"));
  LOG_INFO(logger, "over jumps brown {} {} {} {} {} {}", std::string_view("view1"), 4.0f, 2,
           std::string("str2"), 3.0, 6LL);
  LOG_INFO(logger, "quick jumps logging test {} {} {} {}", false, std::string("str1"),
           std::string_view("view1"), 8ULL);
  LOG_INFO(logger, "jumps fox lazy dog {} {} {} {} {} {} {} {} {}", true, static_cast<unsigned short>(10),
           std::string_view("view1"), "example1", 7UL, static_cast<short>(9), "example2", 8ULL, 1);
  LOG_INFO(logger, "logging over test dog {}", std::string("str1"));
  LOG_INFO(logger, "quick logging test lazy {} {} {} {} {} {} {} {} {} {}", std::string("str2"), 8ULL, 1, 3.0,
           std::string_view("view2"), 6LL, true, 7UL, static_cast<unsigned short>(10), "example1");
  LOG_INFO(logger, "lazy over example brown {} {}", 2, 8ULL);
  LOG_INFO(logger, "dog logging quick test {} {} {} {} {}", true, false, 2, 6LL, "example1");
  LOG_INFO(logger, "example test jumps {} {} {} {} {} {} {} {} {}", false, "example1",
           std::string_view("view2"), 6LL, std::string("str1"), 3.0, 7UL, 4.0f, "example3");
  LOG_INFO(logger, "dog logging jumps {} {} {} {} {} {}", 3.0, 2, "example2", true, false,
           static_cast<short>(9));
  LOG_INFO(logger, "example test fox {}", 4.0f);
  LOG_INFO(logger, "brown logging example {} {} {} {}", std::string("str1"), 1, "example3", 3.0);
  LOG_INFO(logger, "dog jumps fox quick {} {} {} {} {} {} {} {} {} {}", static_cast<short>(9),
           "example1", "example2", static_cast<unsigned short>(10), 2, true, 8ULL,
           std::string_view("view1"), 1, 7UL);
  LOG_INFO(logger, "over dog fox lazy {} {} {} {} {} {} {} {} {} {}", 8ULL, 6LL,
           std::string_view("view2"), 3.0, std::string("str2"), 2, static_cast<short>(9),
           "example3", static_cast<unsigned short>(10), "example1");
  LOG_INFO(logger, "brown test logging {} {} {} {} {} {} {} {} {}", 3.0, std::string("str2"), 2,
           std::string_view("view1"), "example1", 6LL, true, std::string_view("view2"), false);
  LOG_INFO(logger, "lazy fox brown {} {} {}", static_cast<short>(9), std::string_view("view2"),
           "example1");
  LOG_INFO(logger, "jumps lazy fox quick {} {} {} {} {} {} {} {}", false, 4.0f, 2, "example3",
           std::string("str1"), true, "example1", 3.0);
  LOG_INFO(logger, "lazy logging over {} {} {} {} {} {}", "example2", "example1",
           std::string("str1"), false, 2, std::string_view("view2"));
  LOG_INFO(logger, "test dog jumps example {} {} {}", static_cast<short>(9),
           static_cast<unsigned short>(10), 8ULL);
  LOG_INFO(logger, "example dog over quick {} {} {} {} {} {} {} {} {} {}", true, 1,
           std::string("str1"), std::string_view("view1"), "example1", 3.0, false, 2, "example3",
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown test example over {} {} {} {} {} {}", 8ULL, 3.0, std::string("str1"),
           "example2", 5L, true);
  LOG_INFO(logger, "brown example jumps {} {} {} {} {} {} {}", 7UL, false, 6LL, 3.0, "example1",
           static_cast<short>(9), 8ULL);
  LOG_INFO(logger, "dog fox jumps {} {} {} {} {} {} {} {}", 6LL, 5L, 8ULL, 1, true, 2,
           std::string("str2"), std::string_view("view1"));
  LOG_INFO(logger, "brown quick test {} {}", static_cast<unsigned short>(10), 6LL);
  LOG_INFO(logger, "logging brown fox {} {} {} {}", true, 5L, 7UL, 3.0);
  LOG_INFO(logger, "test jumps lazy brown {}", "example2");
  LOG_INFO(logger, "logging test lazy {}", true);
  LOG_INFO(logger, "lazy fox dog example {} {} {} {} {} {} {} {} {} {}", std::string("str2"),
           std::string_view("view1"), std::string("str1"), 5L, 7UL, static_cast<short>(9),
           "example1", 3.0, "example2", 6LL);
  LOG_INFO(logger, "lazy test example logging {} {} {} {} {} {} {} {} {} {}", 6LL, "example3",
           false, 8ULL, std::string("str1"), "example1", 3.0, true, 4.0f, 7UL);
  LOG_INFO(logger, "dog logging over quick {} {}", 4.0f, false);
  LOG_INFO(logger, "logging brown fox {}", "example3");
  LOG_INFO(logger, "dog logging brown {} {} {} {} {} {} {} {}", "example3",
           std::string_view("view1"), static_cast<unsigned short>(10), "example2",
           std::string_view("view2"), 2, 8ULL, "example1");
  LOG_INFO(logger, "brown over dog {} {} {} {} {} {} {} {} {} {}", "example1", 6LL, 4.0f, true,
           std::string_view("view1"), 1, 3.0, static_cast<short>(9), "example3", 5L);
  LOG_INFO(logger, "logging example test quick {} {} {} {} {} {} {} {} {}", std::string("str1"),
           std::string_view("view2"), "example3", "example2", 8ULL, static_cast<short>(9), 1,
           static_cast<unsigned short>(10), false);
  LOG_INFO(logger, "fox over lazy jumps {} {} {} {} {} {} {} {} {}", 1, false,
           std::string_view("view1"), 6LL, 4.0f, 5L, 3.0, static_cast<short>(9), 7UL);
  LOG_INFO(logger, "lazy test example over {} {} {} {} {} {} {} {} {}", static_cast<short>(9),
           "example3", "example1", "example2", 8ULL, std::string("str1"), false, 6LL, 3.0);
  LOG_INFO(logger, "over logging fox brown {} {} {} {} {} {} {} {} {}", "example3", 3.0, 4.0f,
           std::string_view("view1"), 7UL, true, 6LL, std::string("str2"), 2);
  LOG_INFO(logger, "jumps lazy brown logging {} {} {}", "example3", std::string_view("view1"), 6LL);
  LOG_INFO(logger, "brown jumps lazy {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "test quick brown {} {} {}", "example1", false, std::string_view("view1"));
  LOG_INFO(logger, "fox brown over {} {} {} {} {} {} {} {} {}", 2, "example3", 3.0, 4.0f,
           std::string_view("view2"), "example2", 1, std::string("str1"), 7UL);
  LOG_INFO(logger, "fox logging jumps example {} {} {} {} {}", "example1", 4.0f,
           static_cast<unsigned short>(10), "example2", true);
  LOG_INFO(logger, "quick over jumps fox {} {} {} {} {} {} {} {}", 8ULL, 1, 4.0f, std::string_view("view2"),
           2, static_cast<unsigned short>(10), std::string_view("view1"), std::string("str1"));
  LOG_INFO(logger, "logging dog brown {} {}", 5L, std::string_view("view1"));
  LOG_INFO(logger, "fox example brown dog {} {} {}", 2, "example1", 4.0f);
  LOG_INFO(logger, "brown logging dog fox {} {} {} {} {} {} {} {} {} {}", false, 5L, 8ULL,
           std::string("str1"), 2, "example2", 3.0, static_cast<unsigned short>(10), 1, "example3");
  LOG_INFO(logger, "over jumps fox {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string_view("view2"), static_cast<short>(9), 2, 4.0f, std::string_view("view1"),
           "example3");
  LOG_INFO(logger, "example fox logging {} {} {} {} {} {} {} {} {} {}", std::string("str2"), 4.0f,
           std::string("str1"), 5L, static_cast<short>(9), 8ULL, 7UL,
           static_cast<unsigned short>(10), 3.0, 6LL);
  LOG_INFO(logger, "fox jumps dog {} {}", 4.0f, true);
  LOG_INFO(logger, "quick over example {} {} {} {} {} {}", 7UL, std::string("str2"), 5L, "example1", false, 1);
  LOG_INFO(logger, "brown lazy over logging {} {} {} {} {} {} {} {}", std::string("str2"), 1, 8ULL,
           static_cast<unsigned short>(10), 5L, 4.0f, true, 3.0);
  LOG_INFO(logger, "over jumps example {} {} {} {} {} {} {}", 5L, "example3", 4.0f, 7UL, "example2",
           static_cast<unsigned short>(10), true);
  LOG_INFO(logger, "jumps brown example {} {} {} {} {} {} {}", 2, 8ULL, 1, std::string("str1"),
           std::string_view("view1"), false, static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps quick brown lazy {} {}", false, 8ULL);
  LOG_INFO(logger, "example quick lazy {} {} {} {} {} {} {} {} {} {}", 3.0, 2,
           static_cast<unsigned short>(10), 7UL, 6LL, std::string("str1"), 1, std::string("str2"),
           "example3", std::string_view("view1"));
  LOG_INFO(logger, "lazy logging quick {} {} {} {} {} {} {} {}", 4.0f, true, 1, 5L, 3.0, 2,
           std::string_view("view1"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "lazy brown dog fox {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           false, static_cast<short>(9), "example1", 4.0f, 7UL, static_cast<unsigned short>(10),
           3.0, "example3");
  LOG_INFO(logger, "quick logging brown over {} {} {} {} {} {}", std::string_view("view2"),
           "example2", 6LL, static_cast<short>(9), static_cast<unsigned short>(10), 4.0f);
  LOG_INFO(logger, "logging fox example {} {} {} {} {} {} {}", 5L, std::string_view("view2"), 6LL,
           "example3", 7UL, std::string("str1"), 2);
  LOG_INFO(logger, "jumps lazy quick dog {} {} {} {}", 7UL, 5L, 6LL, std::string("str1"));
  LOG_INFO(logger, "jumps over lazy {} {} {} {} {} {}", "example2", static_cast<short>(9), 8ULL, 5L,
           1, "example3");
  LOG_INFO(logger, "dog jumps fox {} {} {} {} {}", 3.0, std::string_view("view2"), 8ULL,
           static_cast<short>(9), 2);
  LOG_INFO(logger, "quick example fox {} {} {} {} {}", 8ULL, true, 7UL, static_cast<short>(9),
           "example2");
  LOG_INFO(logger, "test quick fox logging {} {} {} {} {} {}", "example1", false,
           std::string("str2"), static_cast<short>(9), 3.0, "example2");
  LOG_INFO(logger, "jumps logging example quick {} {} {}", 4.0f, std::string("str1"), 8ULL);
  LOG_INFO(logger, "dog brown logging {} {}", false, "example2");
  LOG_INFO(logger, "dog fox brown quick {} {} {} {} {}", "example3", 1, std::string_view("view1"),
           static_cast<short>(9), 8ULL);
  LOG_INFO(logger, "quick jumps test dog {} {}", 7UL, 8ULL);
  LOG_INFO(logger, "fox example quick {} {} {} {} {} {} {} {} {} {}", 2, 4.0f, 1, 6LL, std::string("str1"),
           "example2", std::string_view("view1"), 7UL, false, static_cast<unsigned short>(10));
  LOG_INFO(logger, "test quick logging {} {} {} {} {} {} {}", std::string("str2"), false, 2, 7UL,
           8ULL, 4.0f, static_cast<short>(9));
  LOG_INFO(logger, "over test brown {} {} {} {} {} {} {} {} {} {}", 8ULL, 6LL, false, true,
           "example1", "example2", 7UL, std::string("str1"), 5L, std::string_view("view1"));
  LOG_INFO(logger, "test example over lazy {}", 8ULL);
  LOG_INFO(logger, "quick dog over {}", 4.0f);
  LOG_INFO(logger, "example quick jumps {} {} {}", 8ULL, std::string("str2"), 4.0f);
  LOG_INFO(logger, "fox brown lazy {} {} {} {} {}", 3.0, false, static_cast<unsigned short>(10),
           "example1", 1);
  LOG_INFO(logger, "quick lazy jumps brown {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           7UL, true, std::string("str1"), 1, "example1", std::string_view("view1"));
  LOG_INFO(logger, "over jumps logging quick {} {} {} {} {}", "example3", std::string("str2"), true, 5L, 2);
  LOG_INFO(logger, "quick fox logging {} {} {} {}", 6LL, 2, 7UL, 1);
  LOG_INFO(logger, "dog fox quick jumps {} {}", 6LL, static_cast<short>(9));
  LOG_INFO(logger, "dog over jumps fox {} {}", true, 5L);
  LOG_INFO(logger, "logging example dog jumps {} {} {} {}", 4.0f, "example2", static_cast<short>(9),
           std::string_view("view1"));
  LOG_INFO(logger, "logging fox test {} {} {} {} {} {} {} {} {} {}", "example1", 7UL, "example2", 2,
           1, 3.0, std::string("str2"), 5L, 4.0f, "example3");
  LOG_INFO(logger, "logging fox quick brown {} {} {}", "example1", 3.0, 6LL);
  LOG_INFO(logger, "quick lazy brown test {} {} {} {}", "example1", std::string_view("view1"),
           std::string("str1"), std::string_view("view2"));
  LOG_INFO(logger, "over fox dog {} {} {} {} {} {} {} {}", "example1", static_cast<unsigned short>(10),
           true, 5L, std::string_view("view2"), 3.0, std::string("str1"), 2);
  LOG_INFO(logger, "jumps logging over {} {} {} {} {} {} {} {} {} {}", 8ULL, std::string("str2"), 7UL,
           "example3", 4.0f, static_cast<unsigned short>(10), std::string_view("view1"), 2, 3.0, 6LL);
  LOG_INFO(logger, "jumps fox over {} {} {} {}", std::string_view("view2"), 5L, 7UL, std::string_view("view1"));
  LOG_INFO(logger, "example brown over jumps {}", 7UL);
  LOG_INFO(logger, "fox example jumps test {}", true);
  LOG_INFO(logger, "fox test over {} {} {}", std::string_view("view1"), "example3", std::string("str1"));
  LOG_INFO(logger, "logging dog brown over {} {} {} {} {}", 6LL, std::string_view("view2"),
           std::string("str2"), 5L, "example1");
  LOG_INFO(logger, "fox example dog {} {} {} {} {} {}", 2, 1, static_cast<unsigned short>(10), 4.0f,
           std::string("str2"), std::string_view("view2"));
  LOG_INFO(logger, "fox dog jumps {} {} {} {} {} {} {} {} {}", "example2", std::string("str1"),
           std::string_view("view2"), std::string_view("view1"), 8ULL, 4.0f, 2, "example3",
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox logging dog {} {} {} {} {} {} {} {}", "example1", 1, std::string("str1"),
           static_cast<unsigned short>(10), "example2", false, 6LL, "example3");
  LOG_INFO(logger, "quick test jumps {} {} {} {}", std::string("str2"), 6LL,
           static_cast<unsigned short>(10), false);
  LOG_INFO(logger, "over fox quick dog {} {} {} {} {} {}", std::string("str1"), true,
           std::string("str2"), 4.0f, 7UL, 2);
  LOG_INFO(logger, "fox dog test {} {} {} {} {}", "example1", std::string("str1"), 6LL,
           static_cast<unsigned short>(10), false);
  LOG_INFO(logger, "brown jumps example over {} {} {} {} {} {} {} {} {}", std::string("str2"),
           static_cast<short>(9), 5L, "example2", std::string("str1"), "example3",
           std::string_view("view2"), 4.0f, 3.0);
  LOG_INFO(logger, "quick logging dog {} {} {} {} {} {} {} {} {} {}", std::string("str2"), false,
           8ULL, std::string("str1"), 3.0, 5L, 4.0f, 6LL, 7UL, "example2");
  LOG_INFO(logger, "fox dog quick {} {} {} {} {}", 3.0, true, 4.0f, 2, "example1");
  LOG_INFO(logger, "quick brown over logging {}", 2);
  LOG_INFO(logger, "dog logging example {} {} {}", "example3", 1, "example1");
  LOG_INFO(logger, "logging over jumps {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           4.0f, static_cast<unsigned short>(10), std::string_view("view1"), "example3", "example1",
           std::string("str2"), "example2", 3.0, 6LL);
  LOG_INFO(logger, "jumps quick over {} {} {} {} {} {} {}", "example1", false, true, "example3", 5L,
           std::string("str2"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging lazy brown fox {}", 6LL);
  LOG_INFO(logger, "over quick brown dog {} {} {} {}", "example3", std::string("str2"), 1, 4.0f);
  LOG_INFO(logger, "brown logging fox jumps {} {} {}", true, 5L, 2);
  LOG_INFO(logger, "example test fox {} {}", true, 1);
  LOG_INFO(logger, "quick test fox logging {} {} {} {} {} {}", static_cast<unsigned short>(10),
           8ULL, 4.0f, std::string("str1"), 7UL, false);
  LOG_INFO(logger, "dog logging fox test {} {} {} {} {} {} {}", false, "example3",
           static_cast<unsigned short>(10), 5L, 2, 6LL, std::string("str1"));
  LOG_INFO(logger, "dog example jumps {} {}", std::string("str1"), 7UL);
  LOG_INFO(logger, "dog over jumps test {} {} {} {} {} {} {}", std::string("str2"), false, 4.0f,
           std::string_view("view1"), static_cast<unsigned short>(10), true, 7UL);
  LOG_INFO(logger, "lazy fox brown dog {} {}", std::string_view("view2"), 5L);
  LOG_INFO(logger, "over quick test example {} {} {} {} {} {} {}", 7UL, 5L, 8ULL, 1,
           static_cast<short>(9), std::string_view("view2"), std::string("str2"));
  LOG_INFO(logger, "brown over fox example {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10), 5L,
           std::string_view("view1"), std::string("str2"), static_cast<short>(9), 8ULL, "example3", 3.0);
  LOG_INFO(logger, "logging jumps brown {} {} {} {}", 7UL, 3.0, true, "example1");
  LOG_INFO(logger, "brown quick logging example {} {} {} {} {}", "example2",
           static_cast<unsigned short>(10), 5L, static_cast<short>(9), 6LL);
  LOG_INFO(logger, "jumps logging fox over {} {} {} {} {} {} {} {}", 3.0, 2, "example2", "example1",
           4.0f, std::string("str1"), std::string_view("view2"), true);
  LOG_INFO(logger, "over logging lazy dog {}", 6LL);
  LOG_INFO(logger, "dog lazy over {}", std::string("str1"));
  LOG_INFO(logger, "jumps over logging dog {} {} {} {} {} {} {} {}", 4.0f, "example1", 8ULL, false,
           2, 5L, static_cast<short>(9), std::string_view("view1"));
  LOG_INFO(logger, "brown quick jumps {} {} {} {} {} {} {} {}", std::string_view("view2"), true,
           "example1", 8ULL, static_cast<unsigned short>(10), std::string("str2"), "example2",
           std::string_view("view1"));
  LOG_INFO(logger, "logging dog lazy {} {} {}", "example3", std::string_view("view1"), std::string("str1"));
  LOG_INFO(logger, "test logging fox {} {}", 7UL, 8ULL);
  LOG_INFO(logger, "fox brown lazy {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           static_cast<short>(9), 1, 8ULL, false, "example2", std::string("str1"), 7UL);
  LOG_INFO(logger, "example fox dog {} {}", "example3", std::string_view("view2"));
  LOG_INFO(logger, "logging dog jumps {} {} {} {} {} {} {} {} {}", "example3",
           std::string_view("view2"), static_cast<unsigned short>(10), static_cast<short>(9), 7UL,
           1, "example2", std::string("str1"), true);
  LOG_INFO(logger, "example fox brown {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           static_cast<unsigned short>(10), 4.0f, 5L, 6LL, "example2", true, 2, "example1",
           std::string("str2"));
  LOG_INFO(logger, "fox over example {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10), 2,
           3.0, std::string("str1"), std::string("str2"), true, 1, "example3");
  LOG_INFO(logger, "example jumps brown fox {} {}", false, std::string_view("view2"));
  LOG_INFO(logger, "brown jumps dog logging {} {} {} {} {} {} {} {}", 7UL, "example2", 6LL,
           std::string_view("view1"), 1, static_cast<short>(9), std::string("str2"), 5L);
  LOG_INFO(logger, "example test quick logging {} {} {} {} {} {} {}", std::string_view("view1"),
           true, std::string("str2"), 7UL, std::string_view("view2"), "example3", 3.0);
  LOG_INFO(logger, "over lazy example jumps {} {} {} {} {} {} {}", std::string_view("view2"),
           "example2", std::string_view("view1"), 8ULL, 6LL, false, 3.0);
  LOG_INFO(logger, "jumps dog quick logging {} {}", "example2", std::string_view("view2"));
  LOG_INFO(logger, "lazy example fox test {} {}", std::string("str2"), "example1");
  LOG_INFO(logger, "example quick jumps {}", "example2");
  LOG_INFO(logger, "quick fox example {} {} {} {} {} {} {}", "example2", std::string("str2"), 1,
           8ULL, true, "example1", 3.0);
  LOG_INFO(logger, "lazy jumps dog {} {} {} {} {} {} {} {}", std::string_view("view1"),
           static_cast<unsigned short>(10), std::string_view("view2"), 7UL, std::string("str2"),
           false, 1, 6LL);
  LOG_INFO(logger, "fox lazy over {} {} {}", static_cast<short>(9), 4.0f, "example1");
  LOG_INFO(logger, "brown quick jumps test {} {} {} {} {} {} {} {}", 5L, 4.0f,
           std::string_view("view2"), "example2", "example1", true, false, 2);
  LOG_INFO(logger, "brown fox quick {} {} {}", "example3", static_cast<short>(9), std::string("str2"));
  LOG_INFO(logger, "lazy dog quick {} {} {} {} {} {}", 8ULL, "example1", 7UL, "example3", 3.0, 4.0f);
  LOG_INFO(logger, "over dog test {} {} {} {} {}", static_cast<short>(9), std::string("str2"),
           std::string_view("view1"), std::string("str1"), true);
  LOG_INFO(logger, "quick brown logging {} {} {} {} {} {} {} {} {} {}", 1, 2, "example2",
           std::string("str2"), 5L, false, static_cast<unsigned short>(10), std::string("str1"),
           std::string_view("view2"), "example1");
  LOG_INFO(logger, "dog over test {}", std::string("str2"));
  LOG_INFO(logger, "example logging fox test {} {} {}", true, 2, std::string_view("view1"));
  LOG_INFO(logger, "logging fox quick {} {} {} {} {}", "example3", 6LL, true, 4.0f, 7UL);
  LOG_INFO(logger, "logging brown test jumps {} {} {} {} {}", 2, 3.0, "example2", 5L, static_cast<short>(9));
  LOG_INFO(logger, "dog example logging lazy {} {} {}", "example2", "example1", 2);
  LOG_INFO(logger, "logging fox lazy jumps {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           false, "example2", 8ULL, "example1", "example3", 6LL);
  LOG_INFO(logger, "test logging quick over {}", "example1");
  LOG_INFO(logger, "test quick example {} {}", "example1", 1);
  LOG_INFO(logger, "dog lazy test brown {} {} {} {} {} {} {} {} {} {}", 5L, 2, true, std::string("str1"),
           "example2", std::string_view("view2"), 1, static_cast<unsigned short>(10), 3.0, 6LL);
  LOG_INFO(logger, "over brown jumps fox {} {} {} {} {} {} {} {} {} {}",
           static_cast<unsigned short>(10), 2, "example1", 6LL, "example3", 7UL,
           std::string_view("view2"), 3.0, "example2", std::string_view("view1"));
  LOG_INFO(logger, "dog quick test brown {} {} {} {}", std::string("str1"), "example2",
           std::string("str2"), 8ULL);
  LOG_INFO(logger, "logging fox brown {} {} {} {} {} {}", std::string("str1"), 6LL, 1, "example1", 4.0f, true);
  LOG_INFO(logger, "jumps over test quick {} {} {} {} {} {} {}", std::string("str2"), 6LL, true,
           "example2", 7UL, 5L, 3.0);
  LOG_INFO(logger, "example logging over {} {} {} {} {}", std::string("str1"), 5L, "example2", false, 7UL);
  LOG_INFO(logger, "example quick fox dog {} {} {} {} {} {}", std::string_view("view2"),
           static_cast<short>(9), std::string("str1"), std::string("str2"), false, 1);
  LOG_INFO(logger, "example dog lazy {} {} {}", 2, 6LL, 8ULL);
  LOG_INFO(logger, "dog lazy example {} {} {} {} {} {}", 5L, "example2", "example3", 7UL,
           std::string("str1"), 1);
  LOG_INFO(logger, "brown lazy logging quick {}", static_cast<short>(9));
  LOG_INFO(logger, "jumps test dog fox {} {} {} {} {} {} {} {}", 7UL, "example1", 5L, 6LL, 8ULL,
           false, true, 4.0f);
  LOG_INFO(logger, "dog quick logging test {} {} {} {} {}", 3.0, 2, true, 5L, std::string_view("view1"));
  LOG_INFO(logger, "dog over lazy {} {} {} {} {} {} {}", 5L, "example3", std::string("str1"),
           "example2", 6LL, 8ULL, std::string("str2"));
  LOG_INFO(logger, "example quick jumps {} {} {} {}", "example2", static_cast<unsigned short>(10), false, 7UL);
  LOG_INFO(logger, "quick fox brown {}", true);
  LOG_INFO(logger, "jumps over lazy dog {} {} {} {} {} {} {}", 7UL, 6LL, 8ULL, false, 5L, 3.0, 2);
  LOG_INFO(logger, "brown quick logging {} {} {}", 1, static_cast<unsigned short>(10), std::string("str1"));
  LOG_INFO(logger, "jumps lazy example {} {} {} {} {}", 1, static_cast<unsigned short>(10), 6LL, 5L,
           "example1");
  LOG_INFO(logger, "example jumps quick {} {} {} {} {} {}", std::string_view("view1"),
           std::string_view("view2"), 7UL, 3.0, 6LL, "example1");
  LOG_INFO(logger, "over jumps logging {} {} {} {} {} {} {} {} {} {}", 2, "example1", 4.0f, 6LL,
           7UL, 3.0, std::string_view("view2"), 1, static_cast<short>(9), 8ULL);
  LOG_INFO(logger, "lazy jumps dog {} {} {} {}", static_cast<short>(9), 5L, 2, 4.0f);
  LOG_INFO(logger, "fox dog jumps example {} {} {} {} {} {} {} {} {}", 5L, "example1", 4.0f, 2,
           8ULL, static_cast<short>(9), "example2", 6LL, 7UL);
  LOG_INFO(logger, "logging fox lazy jumps {} {} {} {} {} {} {} {} {}", 2, std::string("str1"),
           std::string_view("view2"), std::string_view("view1"), 4.0f, false, "example2", 5L,
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "test quick brown example {} {} {} {} {} {} {}", 3.0, false, std::string("str1"),
           true, 7UL, "example1", 2);
  LOG_INFO(logger, "dog fox jumps {} {} {} {} {} {} {} {} {}", 4.0f, 1, 2, "example1",
           static_cast<short>(9), "example3", 5L, 3.0, std::string("str2"));
  LOG_INFO(logger, "dog fox brown {} {} {} {}", 2, 1, std::string_view("view1"), std::string("str2"));
  LOG_INFO(logger, "lazy test brown example {} {} {} {} {} {}", "example1", true, "example2",
           std::string("str2"), static_cast<short>(9), 3.0);
  LOG_INFO(logger, "logging brown quick example {} {}", 5L, std::string("str1"));
  LOG_INFO(logger, "example fox lazy dog {} {} {} {} {} {} {}", true, 1,
           static_cast<unsigned short>(10), false, 5L, "example2", 6LL);
  LOG_INFO(logger, "logging dog brown quick {} {} {} {} {}", static_cast<short>(9), 1,
           std::string("str1"), 7UL, 4.0f);
  LOG_INFO(logger, "logging lazy quick {}", false);
  LOG_INFO(logger, "over lazy jumps {} {} {} {}", "example3", "example2", "example1", 2);
  LOG_INFO(logger, "lazy dog fox {} {} {} {} {}", "example1", true, 3.0, std::string("str1"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "over logging example dog {} {} {} {} {} {}", true,
           static_cast<unsigned short>(10), 1, "example1", std::string_view("view2"), "example3");
  LOG_INFO(logger, "brown lazy example {} {} {} {} {} {} {}", 1, 6LL, 7UL, "example2", "example1",
           std::string_view("view2"), false);
  LOG_INFO(logger, "example over test {} {} {} {}", true, std::string_view("view1"),
           static_cast<short>(9), static_cast<unsigned short>(10));
  LOG_INFO(logger, "test dog fox {} {} {} {} {} {} {} {}", "example3", std::string("str2"),
           "example2", 3.0, 7UL, true, std::string_view("view1"), 6LL);
  LOG_INFO(logger, "dog test fox {}", std::string_view("view1"));
  LOG_INFO(logger, "jumps over lazy {} {} {} {} {}", std::string_view("view2"), false, "example3", 3.0, 4.0f);
  LOG_INFO(logger, "example test over logging {} {} {} {} {} {} {} {} {}", 3.0, std::string("str2"),
           std::string("str1"), 4.0f, 8ULL, 1, std::string_view("view1"),
           static_cast<unsigned short>(10), 7UL);
  LOG_INFO(logger, "quick dog logging {} {} {} {} {} {} {} {}", static_cast<short>(9), 8ULL, true,
           2, std::string("str1"), "example1", 1, 3.0);
  LOG_INFO(logger, "quick logging jumps {} {}", "example3", "example1");
  LOG_INFO(logger, "example brown quick {}", "example1");
  LOG_INFO(logger, "jumps fox logging test {} {}", static_cast<short>(9), 4.0f);
  LOG_INFO(logger, "jumps fox lazy over {} {} {} {}", "example3", std::string("str2"), "example2", 5L);
  LOG_INFO(logger, "example test logging {}", 7UL);
  LOG_INFO(logger, "quick example dog {} {} {} {} {} {} {}", 4.0f, 6LL, 8ULL,
           static_cast<unsigned short>(10), true, "example3", 2);
  LOG_INFO(logger, "lazy test fox {} {}", static_cast<short>(9), "example1");
  LOG_INFO(logger, "logging lazy dog jumps {}", false);
  LOG_INFO(logger, "brown jumps lazy fox {} {} {} {} {} {} {}", 8ULL, "example3", "example1",
           std::string_view("view2"), 3.0, std::string("str1"), 2);
  LOG_INFO(logger, "lazy quick dog {} {} {} {} {}", 8ULL, std::string_view("view2"), 7UL, false,
           std::string_view("view1"));
  LOG_INFO(logger, "fox logging brown jumps {} {}", 8ULL, std::string("str1"));
  LOG_INFO(logger, "lazy quick brown example {} {} {} {}", 4.0f, "example3", true, false);
  LOG_INFO(logger, "dog example brown fox {} {} {} {} {} {} {} {} {}", false, 1, "example2", 4.0f,
           8ULL, true, "example3", 2, std::string_view("view2"));
  LOG_INFO(logger, "test quick over example {} {} {} {} {} {} {} {} {} {}", 2,
           std::string_view("view2"), 3.0, 4.0f, false, "example2", 6LL, "example3", 5L, 7UL);
  LOG_INFO(logger, "over dog fox {} {}", "example3", 8ULL);
  LOG_INFO(logger, "fox over jumps {} {}", 3.0, 2);
  LOG_INFO(logger, "jumps quick lazy over {} {}", static_cast<short>(9), 1);
  LOG_INFO(logger, "logging fox jumps {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string_view("view1"), "example2", 2, 4.0f, 6LL);
  LOG_INFO(logger, "lazy jumps example test {} {} {}", static_cast<unsigned short>(10), 3.0, true);
  LOG_INFO(logger, "lazy quick dog over {} {} {} {} {} {} {} {}", "example2",
           static_cast<unsigned short>(10), std::string("str2"), false, true,
           std::string_view("view1"), 8ULL, "example3");
  LOG_INFO(logger, "logging test fox {} {} {} {}", static_cast<unsigned short>(10),
           std::string_view("view2"), false, "example2");
  LOG_INFO(logger, "example quick over {} {} {} {} {} {} {} {}", "example3", std::string_view("view1"),
           4.0f, std::string("str1"), true, static_cast<short>(9), "example2", 1);
  LOG_INFO(logger, "fox jumps quick lazy {} {}", 6LL, 1);
  LOG_INFO(logger, "fox jumps logging brown {} {} {} {} {} {} {} {} {} {}", static_cast<short>(9), 1,
           std::string("str1"), "example2", 5L, "example3", std::string_view("view2"), 8ULL, 4.0f, 2);
  LOG_INFO(logger, "test over fox lazy {} {} {}", false, std::string("str2"), "example3");
  LOG_INFO(logger, "jumps brown fox dog {} {} {} {}", true, 8ULL, std::string("str2"), "example3");
  LOG_INFO(logger, "quick fox lazy dog {} {} {} {} {} {} {}", 7UL, "example1", 3.0, 4.0f,
           static_cast<short>(9), static_cast<unsigned short>(10), 5L);
  LOG_INFO(logger, "lazy example logging {} {} {} {} {} {} {} {} {} {}", true, 1, 7UL, 8ULL,
           std::string("str2"), std::string_view("view1"), 2, false, 6LL, "example3");
  LOG_INFO(logger, "dog brown logging {} {} {}", static_cast<unsigned short>(10), 8ULL, 7UL);
  LOG_INFO(logger, "test example lazy logging {} {} {} {} {} {} {} {}", 8ULL, 3.0, 5L, 4.0f,
           std::string_view("view1"), std::string("str2"), static_cast<unsigned short>(10),
           "example1");
  LOG_INFO(logger, "dog logging over {} {} {} {} {} {}", 2, std::string("str2"),
           static_cast<short>(9), 8ULL, "example3", static_cast<unsigned short>(10));
  LOG_INFO(logger, "test fox over {} {} {} {} {} {}", true, static_cast<short>(9), 5L, 1,
           std::string_view("view2"), "example2");
  LOG_INFO(logger, "lazy fox logging {} {} {}", 3.0, std::string_view("view1"), std::string_view("view2"));
  LOG_INFO(logger, "logging over example test {} {} {} {} {} {} {}", std::string_view("view1"),
           std::string("str2"), 2, "example3", 4.0f, 5L, "example1");
  LOG_INFO(logger, "fox example lazy over {} {} {} {} {} {} {} {}", "example1", false, true, 2, 5L,
           static_cast<unsigned short>(10), "example2", 7UL);
  LOG_INFO(logger, "example fox lazy test {} {} {} {} {} {} {}", std::string_view("view2"),
           std::string("str2"), static_cast<short>(9), std::string_view("view1"), 7UL, "example2", 6LL);
  LOG_INFO(logger, "quick test jumps example {} {} {} {}", "example3", 6LL, std::string_view("view1"), 3.0);
  LOG_INFO(logger, "jumps over brown example {} {}", 4.0f, 8ULL);
  LOG_INFO(logger, "jumps quick dog {} {} {} {} {} {}", 3.0, 8ULL, 7UL, 5L, "example1", std::string("str1"));
  LOG_INFO(logger, "quick dog brown lazy {} {} {} {}", 2, true, 8ULL, static_cast<short>(9));
  LOG_INFO(logger, "quick fox jumps {} {} {} {} {} {} {} {} {}", 8ULL, 7UL, static_cast<short>(9),
           2, 4.0f, 1, std::string_view("view2"), std::string_view("view1"), std::string("str2"));
  LOG_INFO(logger, "lazy example test logging {} {} {} {} {} {} {} {} {} {}", 5L, 2, 3.0, std::string("str1"),
           "example2", std::string_view("view2"), 1, std::string("str2"), true, "example1");
  LOG_INFO(logger, "fox lazy jumps {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           "example1", std::string_view("view1"), 2, 6LL, 4.0f, "example2",
           static_cast<unsigned short>(10), 1, true);
  LOG_INFO(logger, "dog test quick jumps {} {} {}", static_cast<unsigned short>(10), 1, 6LL);
  LOG_INFO(logger, "logging brown dog quick {} {} {} {}", true, 6LL, 4.0f, static_cast<short>(9));
  LOG_INFO(logger, "lazy over dog logging {} {} {} {} {} {} {}", "example2", "example3", 4.0f, 5L,
           true, static_cast<unsigned short>(10), std::string("str1"));
  LOG_INFO(logger, "lazy quick logging {} {} {} {}", 3.0, std::string("str1"), 2, std::string("str2"));
  LOG_INFO(logger, "dog jumps logging lazy {} {}", 6LL, 4.0f);
  LOG_INFO(logger, "logging fox dog quick {} {} {} {} {} {} {} {}", std::string_view("view1"),
           "example2", true, "example1", 1, std::string_view("view2"), 7UL, 2);
  LOG_INFO(logger, "brown logging over {} {} {} {} {} {} {}", std::string("str1"), 5L,
           std::string("str2"), static_cast<unsigned short>(10), "example3", 8ULL, "example1");
  LOG_INFO(logger, "test fox dog {} {} {}", std::string("str2"), static_cast<unsigned short>(10), 2);
  LOG_INFO(logger, "jumps fox brown {} {} {} {} {} {} {} {} {} {}", "example3",
           static_cast<short>(9), 5L, 8ULL, 3.0, std::string_view("view2"), 2, "example1", 4.0f, 1);
  LOG_INFO(logger, "jumps lazy test dog {} {} {} {} {} {} {}", 3.0, true, 7UL, 4.0f, 6LL,
           std::string("str1"), 2);
  LOG_INFO(logger, "logging quick brown lazy {} {} {} {} {}", false, std::string_view("view2"),
           std::string_view("view1"), 1, "example1");
  LOG_INFO(logger, "over brown fox logging {} {} {} {} {} {}", 5L, static_cast<short>(9),
           std::string("str2"), "example3", std::string("str1"), 8ULL);
  LOG_INFO(logger, "quick brown logging {} {} {} {} {} {} {} {} {} {}", std::string("str1"),
           "example3", "example1", 5L, static_cast<short>(9), 1, "example2", 7UL, 2, 8ULL);
  LOG_INFO(logger, "quick brown logging {} {} {} {} {} {} {} {} {} {}",
           static_cast<unsigned short>(10), static_cast<short>(9), std::string_view("view2"), 6LL,
           "example1", 7UL, 4.0f, false, std::string("str2"), 8ULL);
  LOG_INFO(logger, "fox dog lazy test {} {} {}", std::string("str2"), static_cast<short>(9), 5L);
  LOG_INFO(logger, "quick example over {} {} {} {} {} {} {} {} {} {}", 6LL,
           std::string_view("view2"), static_cast<unsigned short>(10), true, 2,
           static_cast<short>(9), "example1", std::string("str1"), std::string("str2"), "example2");
  LOG_INFO(logger, "over example fox {} {} {}", std::string("str2"), "example3", std::string_view("view1"));
  LOG_INFO(logger, "brown logging test {} {} {} {} {}", std::string("str1"),
           std::string_view("view1"), std::string("str2"), false, 1);
  LOG_INFO(logger, "logging quick test dog {} {} {} {} {}", 8ULL, std::string_view("view2"),
           std::string_view("view1"), "example1", true);
  LOG_INFO(logger, "dog example brown lazy {} {} {} {} {} {} {}", "example3",
           std::string_view("view1"), static_cast<unsigned short>(10), 2, 5L, "example2", true);
  LOG_INFO(logger, "jumps brown fox {} {} {} {} {} {} {} {} {}", "example1", 4.0f, std::string("str1"), 8ULL,
           std::string_view("view1"), static_cast<short>(9), "example3", true, std::string_view("view2"));
  LOG_INFO(logger, "logging over lazy {} {} {} {}", 2, false, 1, std::string_view("view2"));
  LOG_INFO(logger, "example quick brown fox {} {} {} {} {}", 6LL, false, 3.0, 2, static_cast<short>(9));
  LOG_INFO(logger, "test fox over logging {} {} {} {} {} {} {} {} {}", 2, 4.0f, 8ULL, 7UL,
           "example1", std::string_view("view2"), false, "example2", 5L);
  LOG_INFO(logger, "test lazy fox {} {} {} {} {} {}", 7UL, static_cast<short>(9), 4.0f,
           std::string_view("view1"), "example2", "example3");
  LOG_INFO(logger, "brown logging lazy {} {} {} {} {} {} {}", 4.0f, 7UL, std::string_view("view2"),
           8ULL, static_cast<unsigned short>(10), "example2", "example1");
  LOG_INFO(logger, "test lazy over {} {} {} {} {} {} {} {} {} {}", 7UL, std::string_view("view2"),
           2, true, "example1", 4.0f, false, std::string("str1"), "example2", "example3");
  LOG_INFO(logger, "logging over test {} {} {} {}", 5L, "example1", static_cast<unsigned short>(10), 7UL);
  LOG_INFO(logger, "over dog lazy quick {} {}", 8ULL, 6LL);
  LOG_INFO(logger, "brown example quick {}", "example1");
  LOG_INFO(logger, "test logging example fox {} {} {} {} {} {} {} {} {}", static_cast<short>(9),
           "example1", "example2", 4.0f, 3.0, std::string_view("view2"), 7UL, 5L, 8ULL);
  LOG_INFO(logger, "quick example fox {}", true);
  LOG_INFO(logger, "test dog fox {}", std::string_view("view1"));
  LOG_INFO(logger, "quick lazy over test {} {} {} {} {} {} {} {}", 8ULL, 2, std::string("str1"),
           "example2", "example1", 3.0, std::string_view("view1"), std::string_view("view2"));
  LOG_INFO(logger, "fox dog jumps {} {} {} {} {} {} {} {} {} {}", std::string("str1"), 7UL,
           "example1", 8ULL, 1, "example2", static_cast<short>(9), std::string_view("view1"), 2, false);
  LOG_INFO(logger, "example logging quick dog {} {} {} {} {} {}", static_cast<short>(9), "example2",
           std::string("str2"), 5L, 8ULL, std::string_view("view2"));
  LOG_INFO(logger, "logging dog quick fox {} {} {} {} {} {} {} {} {}", std::string("str1"), 4.0f,
           std::string("str2"), static_cast<short>(9), 3.0, static_cast<unsigned short>(10),
           std::string_view("view2"), "example1", 5L);
  LOG_INFO(logger, "over dog jumps {} {} {} {}", std::string_view("view2"), static_cast<short>(9),
           4.0f, static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging example fox jumps {} {} {} {} {} {} {}", true, false,
           static_cast<short>(9), 5L, 7UL, 6LL, std::string_view("view2"));
  LOG_INFO(logger, "over quick brown {} {} {} {} {} {} {} {}", 1, 2, false, true, 4.0f,
           std::string_view("view2"), static_cast<unsigned short>(10), std::string("str2"));
  LOG_INFO(logger, "brown lazy logging {} {} {} {} {} {}", "example3", 3.0,
           static_cast<unsigned short>(10), 8ULL, 6LL, std::string("str1"));
  LOG_INFO(logger, "example over dog {} {} {} {} {} {} {} {} {}", "example2", 2, 7UL, true, 8ULL,
           "example3", 4.0f, 1, false);
  LOG_INFO(logger, "example over fox {}", "example1");
  LOG_INFO(logger, "over example brown {}", "example1");
  LOG_INFO(logger, "over jumps fox lazy {} {} {} {} {} {} {} {} {} {}", 2, 4.0f, 8ULL,
           std::string_view("view1"), static_cast<short>(9), true, std::string("str1"), "example2",
           std::string_view("view2"), 6LL);
  LOG_INFO(logger, "brown logging example {} {}", "example3", std::string_view("view2"));
  LOG_INFO(logger, "brown over fox {} {} {} {} {} {} {} {}", 1, "example1", std::string("str2"),
           true, std::string_view("view1"), std::string_view("view2"), 3.0, static_cast<short>(9));
  LOG_INFO(logger, "dog over quick {} {} {} {} {}", std::string("str2"), 8ULL, 4.0f, 5L,
           static_cast<short>(9));
  LOG_INFO(logger, "brown dog quick lazy {} {} {} {} {}", "example3", 3.0, 7UL, "example1", std::string("str1"));
  LOG_INFO(logger, "dog lazy logging quick {} {} {} {} {}", std::string_view("view1"), 6LL, 2,
           "example1", "example2");
  LOG_INFO(logger, "over quick test {} {} {} {}", std::string("str1"), 3.0, 6LL, std::string_view("view1"));
  LOG_INFO(logger, "fox brown quick lazy {} {} {} {}", 3.0, "example3", 5L, std::string("str2"));
  LOG_INFO(logger, "example over lazy {} {} {}", 7UL, 5L, 6LL);
  LOG_INFO(logger, "quick example dog {} {} {} {}", "example1", std::string("str2"),
           std::string_view("view2"), 5L);
  LOG_INFO(logger, "example brown over logging {} {} {} {} {} {} {} {} {}", std::string("str2"),
           6LL, "example3", "example2", 4.0f, false, static_cast<unsigned short>(10), 1, 2);
  LOG_INFO(logger, "test brown quick {} {} {} {} {} {} {} {} {} {}", std::string("str2"), 7UL,
           std::string_view("view1"), true, "example1", 6LL, static_cast<short>(9), 1,
           std::string("str1"), 5L);
  LOG_INFO(logger, "brown dog jumps {} {} {} {} {} {} {} {} {} {}", 7UL, static_cast<unsigned short>(10),
           false, true, 8ULL, 1, 4.0f, std::string("str1"), "example3", std::string_view("view2"));
  LOG_INFO(logger, "example lazy over test {} {} {} {} {}", 1, 3.0, 2, static_cast<unsigned short>(10), 7UL);
  LOG_INFO(logger, "lazy quick fox example {} {} {} {} {} {}", "example3", "example2", 2,
           std::string("str1"), static_cast<unsigned short>(10), 4.0f);
  LOG_INFO(logger, "quick fox dog lazy {} {} {} {} {} {} {} {} {} {}", 5L, std::string_view("view2"),
           4.0f, 3.0, 2, std::string("str2"), "example1", 7UL, "example2", 6LL);
  LOG_INFO(logger, "quick jumps over {} {} {} {} {} {} {}", 3.0, true, 2, "example2",
           std::string("str2"), std::string_view("view2"), 7UL);
  LOG_INFO(logger, "over jumps fox {} {} {} {} {}", "example3", std::string("str1"),
           static_cast<short>(9), std::string_view("view2"), 2);
  LOG_INFO(logger, "quick brown example {} {} {} {} {} {} {}", "example1",
           std::string_view("view2"), true, "example2", false, 3.0, static_cast<unsigned short>(10));
  LOG_INFO(logger, "test dog lazy {} {} {} {} {} {} {}", 5L, std::string("str1"), 8ULL, 2,
           "example2", true, false);
  LOG_INFO(logger, "brown lazy example {} {} {}", "example3", "example1", std::string_view("view2"));
  LOG_INFO(logger, "test logging brown {} {} {} {} {} {} {} {}", 5L, 8ULL, false, "example1",
           "example3", 1, static_cast<short>(9), std::string_view("view1"));
  LOG_INFO(logger, "fox lazy over {} {} {} {} {} {} {}", 3.0, 5L, std::string_view("view2"), 2,
           static_cast<unsigned short>(10), static_cast<short>(9), 7UL);
  LOG_INFO(logger, "logging jumps over {} {} {} {}", "example2", 4.0f, 5L, std::string_view("view2"));
  LOG_INFO(logger, "jumps lazy quick brown {} {} {} {} {} {} {} {} {}", "example2", 4.0f, "example3",
           static_cast<short>(9), 2, 7UL, 5L, std::string("str1"), std::string_view("view1"));
  LOG_INFO(logger, "jumps example lazy dog {} {} {} {} {}", static_cast<short>(9),
           std::string("str2"), 3.0, static_cast<unsigned short>(10), false);
  LOG_INFO(logger, "lazy over example {} {} {} {} {}", 2, "example1",
           static_cast<unsigned short>(10), std::string("str2"), "example2");
  LOG_INFO(logger, "quick logging brown fox {} {}", std::string("str1"), 7UL);
  LOG_INFO(logger, "over test brown quick {}", true);
  LOG_INFO(logger, "lazy over dog brown {} {} {} {} {}", 2, 1, "example2", true, "example1");
  LOG_INFO(logger, "dog test fox {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           false, std::string_view("view2"), std::string_view("view1"), 2, "example1",
           static_cast<short>(9), "example3", "example2", std::string("str2"));
  LOG_INFO(logger, "over dog quick {} {} {} {} {} {}", "example2", 5L, static_cast<short>(9), 1, true, 3.0);
  LOG_INFO(logger, "logging jumps example fox {} {} {} {} {} {}", std::string("str2"), 2, false,
           "example3", "example1", static_cast<short>(9));
  LOG_INFO(logger, "jumps dog example {}", std::string("str1"));
  LOG_INFO(logger, "over logging fox {} {} {} {} {} {} {} {}", "example3", std::string_view("view2"),
           4.0f, false, 1, std::string("str2"), 5L, std::string_view("view1"));
  LOG_INFO(logger, "over brown fox lazy {} {} {} {} {}", true, 8ULL, "example2",
           std::string_view("view2"), static_cast<short>(9));
  LOG_INFO(logger, "fox lazy over {} {} {} {} {} {} {} {} {} {}", "example3", std::string("str1"), false,
           "example1", 1, "example2", 5L, std::string_view("view1"), static_cast<unsigned short>(10), 6LL);
  LOG_INFO(logger, "example dog brown {} {} {} {} {} {} {} {}", true, "example1", std::string_view("view1"),
           std::string_view("view2"), "example2", 5L, static_cast<unsigned short>(10), false);
  LOG_INFO(logger, "jumps dog test over {} {} {} {} {}", std::string("str2"), "example2", 2, 3.0,
           std::string_view("view1"));
  LOG_INFO(logger, "test over brown dog {} {} {} {} {} {}", std::string("str1"), 5L, false,
           static_cast<short>(9), std::string("str2"), std::string_view("view2"));
  LOG_INFO(logger, "jumps example dog logging {} {}", "example3", true);
  LOG_INFO(logger, "dog quick jumps {} {} {} {}", 3.0, static_cast<short>(9), false, 4.0f);
  LOG_INFO(logger, "dog over example logging {} {}", 4.0f, std::string("str1"));
  LOG_INFO(logger, "brown fox logging {} {} {} {} {} {} {}", false, 5L, 7UL, std::string_view("view1"),
           static_cast<short>(9), static_cast<unsigned short>(10), "example3");
  LOG_INFO(logger, "brown fox quick dog {} {} {} {} {} {} {}", 4.0f, true, "example2",
           std::string_view("view1"), 3.0, 7UL, 6LL);
  LOG_INFO(logger, "lazy dog quick test {} {} {} {} {}", static_cast<short>(9), std::string("str1"),
           "example1", 2, std::string_view("view2"));
  LOG_INFO(logger, "logging example dog {} {} {} {} {} {}", 7UL, static_cast<short>(9), false, 1, 2,
           "example2");
  LOG_INFO(logger, "jumps dog example test {} {} {} {} {} {}", 6LL, 4.0f, 7UL, "example1", false,
           std::string("str1"));
  LOG_INFO(logger, "jumps brown example over {} {} {}", 5L, 8ULL, static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging fox dog {} {} {} {} {} {}", true, 6LL, 3.0, 1, false, 8ULL);
  LOG_INFO(logger, "example logging brown {} {}", static_cast<unsigned short>(10), 5L);
  LOG_INFO(logger, "jumps example test quick {} {} {} {} {}", 5L, std::string("str1"),
           static_cast<unsigned short>(10), 1, true);
  LOG_INFO(logger, "fox jumps example test {}", "example3");
  LOG_INFO(logger, "over test quick dog {} {} {} {}", static_cast<unsigned short>(10), 6LL, 8ULL,
           "example2");
  LOG_INFO(logger, "quick example jumps dog {} {} {} {} {} {} {}", false, 7UL, 5L,
           static_cast<short>(9), "example3", std::string("str1"), true);
  LOG_INFO(logger, "test logging jumps lazy {} {} {} {} {} {}", std::string_view("view1"), 7UL, 1,
           2, "example1", false);
  LOG_INFO(logger, "example logging dog over {} {} {} {} {} {} {} {}", 1, 5L, "example2",
           static_cast<unsigned short>(10), 3.0, std::string_view("view1"), false, 4.0f);
  LOG_INFO(logger, "test over lazy dog {} {} {} {} {} {} {} {} {}", std::string("str2"), "example1",
           true, 1, "example3", 5L, static_cast<unsigned short>(10), std::string_view("view2"), 4.0f);
  LOG_INFO(logger, "quick lazy example brown {} {} {} {}", "example1", std::string("str2"),
           "example3", static_cast<unsigned short>(10));
  LOG_INFO(logger, "lazy example over {}", std::string("str2"));
  LOG_INFO(logger, "jumps over dog {} {} {} {} {} {} {} {} {} {}", std::string_view("view1"), 1,
           "example3", 7UL, std::string("str2"), static_cast<short>(9), "example2",
           std::string("str1"), 3.0, true);
  LOG_INFO(logger, "fox jumps brown {} {} {}", std::string_view("view1"), std::string("str1"), 5L);
  LOG_INFO(logger, "dog quick jumps logging {} {} {} {} {} {} {} {} {} {}", 6LL,
           std::string_view("view1"), true, 8ULL, false, std::string("str1"), 3.0,
           std::string("str2"), "example2", std::string_view("view2"));
  LOG_INFO(logger, "example fox lazy logging {} {} {} {} {} {}", "example1", true,
           std::string("str2"), 6LL, std::string_view("view2"), 4.0f);
  LOG_INFO(logger, "over fox jumps brown {} {} {} {} {} {}", static_cast<short>(9), true, false, 5L,
           std::string_view("view2"), 6LL);
  LOG_INFO(logger, "dog logging over lazy {} {}", "example3", 5L);
  LOG_INFO(logger, "example test lazy brown {} {} {} {} {} {} {} {} {}", 8ULL, std::string_view("view2"),
           "example1", "example3", static_cast<short>(9), 3.0, std::string("str1"), 5L, 7UL);
  LOG_INFO(logger, "logging quick over dog {} {} {} {} {} {} {} {} {}", std::string_view("view2"), 7UL,
           "example1", false, static_cast<short>(9), true, "example2", 5L, std::string_view("view1"));
  LOG_INFO(logger, "test logging lazy fox {} {} {} {} {} {} {} {}", 6LL, 7UL, 2, "example2",
           std::string("str2"), "example1", 5L, "example3");
  LOG_INFO(logger, "quick brown lazy {} {} {} {}", static_cast<short>(9),
           static_cast<unsigned short>(10), 8ULL, 7UL);
  LOG_INFO(logger, "quick logging lazy dog {} {} {}", static_cast<short>(9), "example1",
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown lazy logging {}", std::string("str1"));
  LOG_INFO(logger, "fox example lazy test {} {} {} {} {} {} {}", 8ULL, true, "example3",
           static_cast<unsigned short>(10), false, static_cast<short>(9), 3.0);
  LOG_INFO(logger, "over fox lazy {} {} {} {} {} {} {} {}", std::string("str2"),
           std::string("str1"), 5L, "example3", "example2", std::string_view("view2"),
           static_cast<unsigned short>(10), std::string_view("view1"));
  LOG_INFO(logger, "lazy example dog {} {} {} {} {}", 2, 1, 8ULL, 7UL, 4.0f);
  LOG_INFO(logger, "over test quick {} {} {} {} {} {} {} {} {} {}", std::string_view("view1"), 6LL, 1,
           std::string("str1"), 4.0f, "example2", static_cast<unsigned short>(10), 2, "example1", 3.0);
  LOG_INFO(logger, "brown test logging jumps {} {} {} {}", 2, 3.0, false, 5L);
  LOG_INFO(logger, "over lazy dog logging {} {} {} {} {} {} {} {} {}", 2, 8ULL, 4.0f, 7UL,
           std::string_view("view2"), static_cast<short>(9), 3.0, "example2", std::string_view("view1"));
  LOG_INFO(logger, "dog test jumps {} {}", 6LL, static_cast<short>(9));
  LOG_INFO(logger, "test fox example {} {} {} {}", true, 1, 6LL, std::string("str1"));
  LOG_INFO(logger, "jumps logging quick example {} {} {} {} {} {}", std::string_view("view2"), 2,
           8ULL, std::string("str2"), std::string("str1"), static_cast<short>(9));
  LOG_INFO(logger, "over logging dog {} {} {} {} {} {} {}", true, "example3",
           static_cast<unsigned short>(10), 3.0, "example2", std::string("str1"), 8ULL);
  LOG_INFO(logger, "fox brown logging {} {} {} {} {} {} {} {}", "example2", 2, 6LL, "example3",
           std::string_view("view1"), std::string_view("view2"), false, 7UL);
  LOG_INFO(logger, "fox test over quick {} {}", std::string("str1"), 5L);
  LOG_INFO(logger, "quick over brown {} {} {} {} {} {} {}", 8ULL, std::string("str2"), false, 2,
           4.0f, std::string_view("view2"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "over fox lazy logging {} {} {} {} {} {} {}", 1, 5L, std::string_view("view1"),
           6LL, 4.0f, "example3", "example2");
  LOG_INFO(logger, "quick over example {} {} {} {} {} {} {} {}", std::string_view("view2"),
           "example1", false, true, std::string_view("view1"), 2, 6LL, 4.0f);
  LOG_INFO(logger, "fox dog brown lazy {} {} {} {}", "example2", true, std::string("str2"), 3.0);
  LOG_INFO(logger, "jumps dog test quick {} {}", 8ULL, std::string_view("view2"));
  LOG_INFO(logger, "lazy quick fox jumps {} {} {} {} {} {} {}", std::string_view("view2"),
           static_cast<unsigned short>(10), 3.0, static_cast<short>(9), 8ULL, 7UL, "example2");
  LOG_INFO(logger, "quick example dog {} {} {} {} {} {} {} {} {}", std::string("str2"), false, 2,
           6LL, std::string_view("view2"), std::string_view("view1"), 5L, 7UL, std::string("str1"));
  LOG_INFO(logger, "over example lazy {} {} {} {} {} {} {}", 2, 8ULL, 5L, std::string_view("view2"),
           "example2", false, std::string("str2"));
  LOG_INFO(logger, "over jumps lazy test {} {} {} {}", true, std::string("str1"),
           static_cast<unsigned short>(10), std::string("str2"));
  LOG_INFO(logger, "lazy dog quick {}", std::string_view("view1"));
  LOG_INFO(logger, "quick brown over {} {} {} {}", 4.0f, 1, std::string_view("view1"), "example2");
  LOG_INFO(logger, "example quick jumps {} {} {} {}", 7UL, 4.0f, "example3", static_cast<short>(9));
  LOG_INFO(logger, "jumps dog over {} {} {} {} {} {} {} {} {}", 2, 8ULL, 5L, 4.0f,
           std::string_view("view2"), true, 7UL, 3.0, false);
  LOG_INFO(logger, "fox lazy example dog {}", std::string("str2"));
  LOG_INFO(logger, "logging over jumps {} {} {} {} {}", static_cast<unsigned short>(10), "example3",
           std::string_view("view2"), 7UL, true);
  LOG_INFO(logger, "brown test jumps lazy {} {} {} {} {}", 6LL, 7UL, 2, std::string_view("view1"), 4.0f);
  LOG_INFO(logger, "over fox lazy dog {} {} {} {}", "example3", 1, 3.0, static_cast<short>(9));
  LOG_INFO(logger, "jumps dog lazy logging {} {} {} {} {} {} {} {} {}", std::string("str1"), false,
           "example3", 5L, std::string_view("view2"), std::string_view("view1"), 2,
           static_cast<short>(9), true);
  LOG_INFO(logger, "fox over lazy {} {} {} {} {} {} {} {}", 2, 4.0f, std::string("str2"),
           "example3", 3.0, 1, 8ULL, "example2");
  LOG_INFO(logger, "logging lazy jumps {} {} {} {} {}", static_cast<short>(9), 4.0f, 5L, 6LL,
           std::string_view("view2"));
  LOG_INFO(logger, "lazy logging fox {} {} {} {} {} {} {} {}", 4.0f, 6LL, static_cast<short>(9), 2,
           std::string("str1"), "example1", std::string_view("view1"), 1);
  LOG_INFO(logger, "over brown quick lazy {} {} {} {} {} {} {} {}", 5L, 8ULL, true, 7UL, "example1",
           std::string("str1"), static_cast<unsigned short>(10), 1);
  LOG_INFO(logger, "logging brown quick {} {} {} {} {} {} {} {}", 8ULL, false, "example2", true,
           "example1", std::string_view("view1"), std::string_view("view2"), 7UL);
  LOG_INFO(logger, "example logging brown {} {} {} {} {}", 7UL, false,
           static_cast<unsigned short>(10), true, 5L);
  LOG_INFO(logger, "lazy test jumps logging {} {} {} {} {} {}", static_cast<short>(9), true,
           std::string("str1"), std::string_view("view2"), std::string_view("view1"), "example1");
  LOG_INFO(logger, "test logging dog over {} {} {} {}", false, static_cast<short>(9), 3.0, true);
  LOG_INFO(logger, "jumps over quick {} {} {} {} {}", 6LL, 3.0, std::string_view("view1"),
           std::string_view("view2"), 7UL);
  LOG_INFO(logger, "logging test quick {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10), 6LL,
           std::string("str1"), static_cast<short>(9), "example2", true, 5L, std::string("str2"), 3.0, 2);
  LOG_INFO(logger, "jumps lazy test over {} {} {} {}", false, 6LL, 8ULL, std::string("str2"));
  LOG_INFO(logger, "brown example fox {} {}", std::string_view("view2"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps brown example quick {} {} {} {}", 7UL, 2, 1, std::string_view("view2"));
  LOG_INFO(logger, "over example lazy {}", "example3");
  LOG_INFO(logger, "test jumps lazy over {} {} {} {} {} {} {}", 5L, "example3",
           std::string_view("view1"), false, 2, std::string("str1"), 6LL);
  LOG_INFO(logger, "logging over test brown {} {} {} {} {} {} {} {} {} {}", 8ULL, 1,
           std::string("str2"), true, std::string("str1"), "example3", static_cast<short>(9),
           std::string_view("view1"), std::string_view("view2"), "example1");
  LOG_INFO(logger, "brown over test quick {} {} {} {} {} {} {} {} {} {}", 2, 7UL,
           std::string_view("view1"), 1, true, false, 6LL, 5L, "example1", 8ULL);
  LOG_INFO(logger, "lazy over jumps example {} {} {} {} {} {} {}", std::string_view("view2"), 3.0,
           "example3", "example2", "example1", 6LL, 2);
  LOG_INFO(logger, "test brown dog {} {} {} {} {} {} {} {} {} {}", static_cast<short>(9), 2,
           std::string_view("view1"), std::string("str1"), 7UL, std::string_view("view2"),
           static_cast<unsigned short>(10), "example1", "example2", 6LL);
  LOG_INFO(logger, "dog fox logging quick {} {}", 5L, static_cast<short>(9));
  LOG_INFO(logger, "jumps test quick {} {} {} {} {} {} {}", 8ULL, true, 2,
           std::string_view("view2"), "example1", 1, 6LL);
  LOG_INFO(logger, "lazy example quick logging {} {} {} {} {} {} {} {}", 1, std::string_view("view2"),
           "example3", std::string("str1"), static_cast<short>(9), static_cast<unsigned short>(10), 7UL, 4.0f);
  LOG_INFO(logger, "logging quick dog {} {} {}", "example2", std::string("str2"), 7UL);
  LOG_INFO(logger, "jumps over quick {} {} {} {} {} {} {}", 4.0f, std::string("str2"), 6LL, false,
           "example1", 5L, 8ULL);
  LOG_INFO(logger, "quick test brown {} {} {} {} {} {} {}", 7UL, false, 1, 4.0f, "example3", 5L, 8ULL);
  LOG_INFO(logger, "over fox example {}", std::string("str2"));
  LOG_INFO(logger, "fox logging lazy {} {}", 5L, "example2");
  LOG_INFO(logger, "lazy brown quick over {} {} {} {} {} {} {} {} {} {}", static_cast<short>(9),
           std::string("str1"), 4.0f, true, 7UL, 2, "example2", 5L, false, 1);
  LOG_INFO(logger, "quick dog over fox {} {} {}", false, "example2", std::string_view("view1"));
  LOG_INFO(logger, "over dog quick {} {} {} {}", std::string_view("view2"),
           static_cast<unsigned short>(10), 6LL, std::string("str1"));
  LOG_INFO(logger, "dog over quick fox {} {}", "example2", "example1");
  LOG_INFO(logger, "lazy fox quick {} {} {} {} {} {}", true, 2, std::string("str1"),
           std::string("str2"), static_cast<unsigned short>(10), "example1");
  LOG_INFO(logger, "lazy dog fox {}", "example3");
  LOG_INFO(logger, "over example jumps dog {} {} {} {} {} {} {} {}", 4.0f, std::string("str2"),
           static_cast<unsigned short>(10), 2, "example1", std::string_view("view2"),
           std::string("str1"), static_cast<short>(9));
  LOG_INFO(logger, "example quick test logging {} {} {} {} {} {} {} {} {}", std::string("str2"),
           std::string_view("view1"), false, 1, 7UL, 2, 8ULL, "example1", "example2");
  LOG_INFO(logger, "brown test lazy {} {} {} {}", static_cast<short>(9), 6LL, 2, 4.0f);
  LOG_INFO(logger, "dog example logging {} {} {} {} {} {} {} {} {}", "example3",
           std::string_view("view1"), 3.0, std::string_view("view2"),
           static_cast<unsigned short>(10), true, 4.0f, 6LL, static_cast<short>(9));
  LOG_INFO(logger, "over jumps test quick {} {} {} {} {} {}", std::string_view("view2"), 7UL, 8ULL,
           std::string("str2"), 5L, "example1");
  LOG_INFO(logger, "example brown fox {} {} {} {} {} {}", "example3", 4.0f, 7UL, 5L, 1, 2);
  LOG_INFO(logger, "dog test quick {}", std::string_view("view2"));
  LOG_INFO(logger, "example lazy quick {} {}", 3.0, static_cast<short>(9));
  LOG_INFO(logger, "dog test fox {} {} {} {} {} {} {} {} {} {}", false, true, 2, std::string("str1"), 6LL,
           "example2", std::string_view("view1"), std::string_view("view2"), std::string("str2"), 3.0);
  LOG_INFO(logger, "brown quick example {} {}", std::string("str2"), 6LL);
  LOG_INFO(logger, "dog brown quick {} {} {} {} {} {} {} {}", 1, 4.0f, 5L, true, 6LL,
           std::string("str1"), "example3", false);
  LOG_INFO(logger, "fox dog jumps quick {}", 8ULL);
  LOG_INFO(logger, "lazy jumps test {} {} {} {} {} {}", 1, "example1", static_cast<short>(9), 3.0, false, 8ULL);
  LOG_INFO(logger, "fox logging example {} {} {} {} {} {} {} {} {}", "example2", 5L, 4.0f, 1,
           std::string_view("view1"), "example3", static_cast<short>(9), true, 6LL);
  LOG_INFO(logger, "quick lazy brown {}", static_cast<short>(9));
  LOG_INFO(logger, "test logging lazy {} {} {} {}", static_cast<unsigned short>(10),
           std::string_view("view2"), 4.0f, std::string_view("view1"));
  LOG_INFO(logger, "jumps dog over {} {}", false, 8ULL);
  LOG_INFO(logger, "quick test lazy {}", true);
  LOG_INFO(logger, "dog example logging lazy {} {} {}", 8ULL, std::string("str2"), "example2");
  LOG_INFO(logger, "fox jumps dog over {} {} {} {} {} {}", 4.0f, "example3", 1, 8ULL, 2, true);
  LOG_INFO(logger, "lazy fox over quick {} {} {} {} {} {} {} {}", std::string_view("view2"), 2, 5L,
           static_cast<short>(9), 3.0, 6LL, "example3", 4.0f);
  LOG_INFO(logger, "logging jumps fox lazy {} {} {} {} {}", std::string_view("view2"), false,
           static_cast<short>(9), true, 8ULL);
  LOG_INFO(logger, "lazy fox quick {} {} {}", "example2", std::string_view("view1"), true);
  LOG_INFO(logger, "jumps dog lazy {} {} {}", static_cast<short>(9), std::string("str2"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps quick logging test {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           "example1", false, 4.0f, 8ULL, std::string_view("view1"), static_cast<short>(9));
  LOG_INFO(logger, "fox test jumps example {}", "example2");
  LOG_INFO(logger, "example fox lazy quick {} {} {} {} {} {}", 8ULL, "example2", "example1", true,
           std::string_view("view2"), 6LL);
  LOG_INFO(logger, "test example over quick {} {} {} {}", 5L, "example2", false, 4.0f);
  LOG_INFO(logger, "over dog example {} {} {}", 6LL, std::string_view("view2"), std::string("str1"));
  LOG_INFO(logger, "jumps test fox logging {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           6LL, 7UL, std::string_view("view2"), static_cast<short>(9), "example1", true);
  LOG_INFO(logger, "dog fox over {} {} {} {} {} {} {}", true, 7UL, static_cast<unsigned short>(10),
           std::string("str1"), 3.0, std::string_view("view1"), 6LL);
  LOG_INFO(logger, "over fox dog {} {} {} {} {} {} {} {} {} {}", 4.0f, "example3", true,
           static_cast<short>(9), 5L, "example1", std::string("str2"), 3.0, "example2",
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog example brown jumps {} {} {} {} {} {}", static_cast<short>(9), 7UL, true,
           3.0, std::string("str1"), 1);
  LOG_INFO(logger, "lazy quick logging over {} {} {} {} {} {}", std::string_view("view1"),
           std::string_view("view2"), "example3", std::string("str2"), 5L, static_cast<short>(9));
  LOG_INFO(logger, "jumps test quick lazy {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           std::string_view("view1"), 3.0, 2, 4.0f, static_cast<short>(9), false, 5L,
           static_cast<unsigned short>(10), "example1");
  LOG_INFO(logger, "brown lazy fox jumps {} {} {} {} {} {} {} {} {}", 8ULL, std::string("str1"),
           "example1", true, std::string_view("view1"), false, 2, 7UL, 4.0f);
  LOG_INFO(logger, "example lazy logging fox {} {} {} {}", "example1", 3.0, "example2", 5L);
  LOG_INFO(logger, "fox lazy test brown {}", true);
  LOG_INFO(logger, "dog brown jumps {} {} {} {} {} {} {}", false, "example3", 2, 8ULL, "example1", 3.0, true);
  LOG_INFO(logger, "quick test logging dog {} {} {}", 3.0, false, "example1");
  LOG_INFO(logger, "test quick jumps {} {} {} {} {} {} {}", "example3", std::string("str2"), 8ULL,
           std::string("str1"), std::string_view("view2"), true, static_cast<short>(9));
  LOG_INFO(logger, "fox brown jumps {} {}", false, 3.0);
  LOG_INFO(logger, "brown quick dog over {} {} {} {} {} {} {} {} {} {}", 2,
           static_cast<unsigned short>(10), 1, "example1", std::string("str2"), true,
           std::string_view("view2"), 6LL, 8ULL, std::string_view("view1"));
  LOG_INFO(logger, "logging quick example {} {} {} {}", 5L, "example3", 4.0f, 1);
  LOG_INFO(logger, "dog brown quick {}", 8ULL);
  LOG_INFO(logger, "lazy brown test {} {} {} {} {} {} {} {}", std::string("str1"), "example1", true,
           6LL, 5L, 1, "example3", 8ULL);
  LOG_INFO(logger, "test example logging {} {}", 1, true);
  LOG_INFO(logger, "test jumps quick fox {} {} {} {} {} {} {} {} {}", "example1", 5L, 4.0f, false,
           8ULL, 7UL, static_cast<unsigned short>(10), 6LL, "example3");
  LOG_INFO(logger, "lazy dog logging {} {} {}", "example3", std::string("str2"), 1);
  LOG_INFO(logger, "over example dog quick {}", "example3");
  LOG_INFO(logger, "test quick lazy logging {} {} {} {} {} {} {}", std::string_view("view1"), 5L,
           "example3", 7UL, 3.0, true, 8ULL);
  LOG_INFO(logger, "test dog lazy {} {} {} {}", static_cast<unsigned short>(10), 2,
           std::string_view("view2"), static_cast<short>(9));
  LOG_INFO(logger, "dog example jumps brown {} {} {}", "example3", 7UL, static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown fox over dog {} {} {} {}", static_cast<unsigned short>(10), "example2", 2,
           std::string_view("view1"));
  LOG_INFO(logger, "test example quick logging {} {} {} {} {} {} {} {} {} {}", "example3",
           "example1", 8ULL, std::string_view("view2"), false, true, 6LL, std::string("str2"), 5L, 7UL);
  LOG_INFO(logger, "over example dog fox {} {} {} {} {} {} {} {} {} {}", 3.0, "example3", true,
           4.0f, false, 8ULL, std::string("str2"), static_cast<short>(9), 6LL, 1);
  LOG_INFO(logger, "example logging over dog {} {} {} {} {} {} {}", 6LL, std::string_view("view2"),
           std::string("str1"), "example3", 1, std::string_view("view1"), 5L);
  LOG_INFO(logger, "brown jumps logging quick {} {} {}", 2, 1, static_cast<short>(9));
  LOG_INFO(logger, "dog test jumps {} {} {}", 6LL, 1, 3.0);
  LOG_INFO(logger, "brown test dog quick {} {} {} {} {} {}", "example3", 1, static_cast<short>(9),
           std::string_view("view1"), 8ULL, std::string_view("view2"));
  LOG_INFO(logger, "quick jumps example dog {} {} {} {} {}", 2, std::string_view("view1"), 3.0,
           static_cast<unsigned short>(10), 5L);
  LOG_INFO(logger, "logging brown quick {} {} {}", 5L, 1, 4.0f);
  LOG_INFO(logger, "logging jumps test brown {} {}", "example3", 6LL);
  LOG_INFO(logger, "brown over example {}", 6LL);
  LOG_INFO(logger, "fox dog example logging {} {} {} {} {} {} {} {}", false, static_cast<short>(9),
           "example3", "example2", std::string("str2"), 8ULL, "example1", true);
  LOG_INFO(logger, "logging test over brown {}", 4.0f);
  LOG_INFO(logger, "over brown dog {} {} {} {} {} {} {} {} {} {}", 7UL, 4.0f, 2,
           std::string_view("view1"), 3.0, 5L, 6LL, 8ULL, "example3", 1);
  LOG_INFO(logger, "quick dog test {} {} {} {} {} {} {} {}", std::string_view("view1"), 6LL, 4.0f,
           "example2", "example1", 1, 7UL, 3.0);
  LOG_INFO(logger, "quick fox example test {} {} {} {} {} {}", "example3",
           static_cast<unsigned short>(10), 2, 4.0f, 8ULL, std::string_view("view2"));
  LOG_INFO(logger, "jumps dog logging fox {} {} {} {} {}", std::string("str2"),
           static_cast<unsigned short>(10), "example3", 6LL, std::string("str1"));
  LOG_INFO(logger, "lazy brown logging quick {} {} {} {} {}", static_cast<short>(9), 8ULL, true,
           static_cast<unsigned short>(10), 1);
  LOG_INFO(logger, "dog example jumps over {} {} {} {}", 2, 5L, 7UL, "example2");
  LOG_INFO(logger, "example logging fox {} {} {} {} {} {} {} {} {}", 8ULL, true, static_cast<short>(9),
           std::string("str1"), 4.0f, false, static_cast<unsigned short>(10), 7UL, "example1");
  LOG_INFO(logger, "lazy over quick {} {} {} {} {} {} {}", false, "example2", 3.0,
           std::string_view("view1"), std::string("str1"), 5L, 4.0f);
  LOG_INFO(logger, "fox brown jumps {} {} {} {}", std::string("str1"), 4.0f, 5L, std::string("str2"));
  LOG_INFO(logger, "logging test lazy {} {} {} {} {} {} {} {} {}", 8ULL, std::string("str1"),
           std::string("str2"), std::string_view("view2"), 2, 5L, 1, 7UL, 4.0f);
  LOG_INFO(logger, "brown example test {} {} {} {} {} {}", 4.0f, std::string_view("view2"), 2,
           std::string("str2"), static_cast<unsigned short>(10), static_cast<short>(9));
  LOG_INFO(logger, "logging over test dog {} {}", "example2", static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown example over {} {} {} {} {}", static_cast<short>(9),
           std::string_view("view1"), false, true, 8ULL);
  LOG_INFO(logger, "logging brown quick {} {} {} {} {}", "example2", 7UL, 8ULL, 6LL, 3.0);
  LOG_INFO(logger, "example jumps dog {} {} {} {} {}", std::string_view("view2"), 7UL, 6LL, true, 5L);
  LOG_INFO(logger, "test lazy example {} {} {} {} {} {} {}", std::string_view("view1"), 5L, true,
           std::string("str2"), 7UL, static_cast<unsigned short>(10), static_cast<short>(9));
  LOG_INFO(logger, "over dog jumps {} {}", static_cast<short>(9), 8ULL);
  LOG_INFO(logger, "over test jumps {} {} {} {} {} {}", 5L, 1, std::string("str1"), "example3",
           std::string("str2"), 4.0f);
  LOG_INFO(logger, "brown dog logging {} {} {} {} {} {}", 3.0, 8ULL, "example2", 5L,
           std::string_view("view2"), "example1");
  LOG_INFO(logger, "jumps dog example {} {} {} {} {}", "example2", 2, std::string("str2"),
           "example3", 5L);
  LOG_INFO(logger, "jumps dog logging fox {} {} {} {}", std::string("str1"), 3.0, false, std::string("str2"));
  LOG_INFO(logger, "fox dog quick jumps {} {} {} {} {} {} {} {} {} {}", 2, "example1", 4.0f, 3.0,
           5L, "example3", true, "example2", 6LL, 7UL);
  LOG_INFO(logger, "dog example test quick {} {} {} {} {} {} {} {} {} {}", std::string("str1"),
           "example1", 5L, 4.0f, 8ULL, true, 7UL, std::string_view("view2"), std::string("str2"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "quick jumps dog {} {} {} {} {} {} {} {} {}", "example3", "example2",
           static_cast<unsigned short>(10), 4.0f, true, std::string_view("view1"), 1, 2, 6LL);
  LOG_INFO(logger, "example jumps over logging {} {} {} {} {} {} {}", 1, false,
           static_cast<short>(9), std::string_view("view2"), "example2", 7UL, std::string("str2"));
  LOG_INFO(logger, "lazy over fox dog {} {} {} {} {} {} {} {}", 8ULL, 1, std::string("str1"), 2,
           4.0f, 3.0, static_cast<unsigned short>(10), "example1");
  LOG_INFO(logger, "brown over quick example {} {} {} {} {} {} {} {}", false, 3.0, 4.0f, "example3",
           std::string_view("view2"), 7UL, std::string("str2"), true);
  LOG_INFO(logger, "fox logging test over {} {} {} {} {} {} {} {}", 7UL, 8ULL,
           static_cast<unsigned short>(10), 5L, "example1", 2, true, std::string_view("view2"));
  LOG_INFO(logger, "brown example logging {} {} {} {} {} {}", true, std::string("str2"), 3.0,
           "example1", std::string_view("view2"), 6LL);
  LOG_INFO(logger, "brown lazy quick {} {} {} {} {} {} {} {} {}", true, 4.0f, std::string_view("view1"),
           static_cast<unsigned short>(10), 3.0, 8ULL, std::string("str2"), 6LL, std::string_view("view2"));
  LOG_INFO(logger, "brown over example {} {} {} {} {} {} {}", std::string_view("view2"), "example3",
           static_cast<unsigned short>(10), 5L, std::string("str2"), static_cast<short>(9), 1);
  LOG_INFO(logger, "jumps dog logging test {} {} {} {} {}", 3.0, std::string("str1"), "example1",
           "example3", static_cast<short>(9));
  LOG_INFO(logger, "jumps fox test quick {}", "example3");
  LOG_INFO(logger, "quick dog logging lazy {} {} {} {} {}", 8ULL, "example2", static_cast<short>(9), 4.0f, 5L);
  LOG_INFO(logger, "test example over {} {} {} {} {}", 7UL, std::string("str1"), 1, 8ULL,
           std::string_view("view2"));
  LOG_INFO(logger, "quick lazy brown {} {}", 8ULL, 6LL);
  LOG_INFO(logger, "test brown fox {}", 1);
  LOG_INFO(logger, "over example lazy {} {} {} {} {} {}", 2, 6LL, 1, 4.0f, std::string("str1"),
           std::string_view("view1"));
  LOG_INFO(logger, "fox test jumps {} {} {} {} {} {} {}", "example3", true, "example1",
           std::string_view("view2"), 7UL, 5L, std::string("str1"));
  LOG_INFO(logger, "logging lazy example {} {}", std::string_view("view2"), std::string_view("view1"));
  LOG_INFO(logger, "logging fox dog {} {} {}", std::string("str2"), true, std::string_view("view2"));
  LOG_INFO(logger, "jumps over brown {} {} {}", 4.0f, "example1", std::string_view("view1"));
  LOG_INFO(logger, "over lazy dog brown {} {} {} {} {} {} {} {} {}", false, std::string_view("view2"),
           std::string_view("view1"), 1, std::string("str1"), "example3", 3.0, "example1", 4.0f);
  LOG_INFO(logger, "dog test brown lazy {} {} {} {} {} {} {}", std::string("str2"), true,
           "example1", 2, std::string_view("view2"), "example3", 7UL);
  LOG_INFO(logger, "example lazy quick dog {} {}", 4.0f, static_cast<unsigned short>(10));
  LOG_INFO(logger, "test brown lazy dog {}", 5L);
  LOG_INFO(logger, "quick fox logging {} {} {} {} {} {}", std::string_view("view2"), "example1",
           std::string_view("view1"), 8ULL, "example3", static_cast<unsigned short>(10));
  LOG_INFO(logger, "over dog quick fox {} {} {} {} {} {} {} {} {} {}", false, 3.0, std::string_view("view1"),
           "example1", 1, static_cast<unsigned short>(10), 2, static_cast<short>(9), 6LL, 4.0f);
  LOG_INFO(logger, "jumps example brown {} {} {} {} {}", true, 1, std::string("str2"),
           static_cast<short>(9), 6LL);
  LOG_INFO(logger, "over test logging quick {} {} {} {} {} {} {} {}", 4.0f, "example2",
           std::string_view("view1"), 8ULL, 3.0, 5L, "example1", 2);
  LOG_INFO(logger, "lazy dog test jumps {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           6LL, 4.0f, 7UL, false, std::string_view("view2"), 3.0, "example2", 1, true);
  LOG_INFO(logger, "example dog fox brown {} {} {} {} {}", std::string("str1"), false, 3.0, 8ULL, true);
  LOG_INFO(logger, "jumps over logging test {} {}", 1, std::string("str1"));
  LOG_INFO(logger, "dog test brown {}", 4.0f);
  LOG_INFO(logger, "over dog test example {} {} {} {}", std::string("str1"), 8ULL, 5L, 2);
  LOG_INFO(logger, "quick test lazy logging {} {} {} {} {} {} {} {} {} {}", "example1", "example2",
           2, std::string_view("view1"), std::string("str2"), false, 1, static_cast<short>(9),
           std::string_view("view2"), std::string("str1"));
  LOG_INFO(logger, "example test fox over {} {} {} {} {} {} {}", 3.0, 6LL, std::string("str2"),
           static_cast<short>(9), std::string("str1"), 4.0f, 1);
  LOG_INFO(logger, "test jumps logging {} {} {} {} {} {}", 3.0, 5L, std::string("str1"),
           std::string("str2"), static_cast<short>(9), 1);
  LOG_INFO(logger, "dog example fox logging {} {} {} {} {} {} {}", std::string("str2"), 3.0, 7UL,
           5L, std::string_view("view2"), 8ULL, 1);
  LOG_INFO(logger, "over fox lazy {} {} {} {} {} {}", 1, 6LL, 2, std::string("str1"), "example1", false);
  LOG_INFO(logger, "logging test lazy dog {} {} {} {} {} {} {} {} {} {}",
           static_cast<unsigned short>(10), 8ULL, false, std::string("str1"),
           std::string_view("view2"), "example1", std::string_view("view1"), 2, 1, 5L);
  LOG_INFO(logger, "quick dog jumps {} {} {}", static_cast<short>(9), std::string("str2"), 8ULL);
  LOG_INFO(logger, "fox test dog over {} {} {} {} {} {} {}", 4.0f, true, false, 3.0, 7UL, 8ULL,
           "example3");
  LOG_INFO(logger, "jumps over logging quick {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps logging fox example {} {} {} {} {} {} {} {} {} {}", "example3",
           "example1", static_cast<unsigned short>(10), std::string("str2"), 8ULL,
           static_cast<short>(9), 4.0f, std::string("str1"), std::string_view("view2"), 7UL);
  LOG_INFO(logger, "fox over quick test {}", false);
  LOG_INFO(logger, "brown dog quick test {} {} {} {} {} {} {}", std::string("str2"),
           std::string("str1"), static_cast<short>(9), true, false, "example1", 3.0);
  LOG_INFO(logger, "over example brown {} {} {} {} {} {} {} {}", 7UL, 8ULL, "example3",
           static_cast<short>(9), "example1", std::string("str1"), std::string_view("view1"), true);
  LOG_INFO(logger, "jumps test example {}", 8ULL);
  LOG_INFO(logger, "over test example brown {} {} {} {} {} {} {} {} {}", true, 4.0f,
           std::string_view("view1"), "example3", std::string("str2"), 3.0, static_cast<short>(9),
           1, static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging jumps test example {} {} {} {} {}", 7UL, "example2", 1, 2, 5L);
  LOG_INFO(logger, "lazy jumps brown {} {}", "example2", std::string_view("view2"));
  LOG_INFO(logger, "brown over quick {} {} {} {} {} {} {}", std::string_view("view1"), 5L,
           static_cast<unsigned short>(10), std::string_view("view2"), "example1", 4.0f, 2);
  LOG_INFO(logger, "test example fox over {} {} {} {} {}", 4.0f, false, "example2", true, 8ULL);
  LOG_INFO(logger, "quick fox lazy over {} {} {} {} {} {}", std::string("str1"), static_cast<short>(9),
           5L, static_cast<unsigned short>(10), std::string_view("view2"), 8ULL);
  LOG_INFO(logger, "test logging lazy {} {} {}", "example1", 7UL, 1);
  LOG_INFO(logger, "fox dog example {} {} {} {} {} {}", "example3", std::string_view("view2"), 8ULL,
           false, std::string("str2"), "example2");
  LOG_INFO(logger, "over fox dog example {} {} {} {} {} {}", std::string_view("view1"), "example3",
           "example2", 2, "example1", 6LL);
  LOG_INFO(logger, "dog brown example quick {} {} {} {} {} {} {} {} {} {}", 3.0, "example3", 2, 8ULL,
           4.0f, std::string_view("view1"), 5L, 7UL, static_cast<unsigned short>(10), "example2");
  LOG_INFO(logger, "test fox dog jumps {} {} {} {} {}", "example1", static_cast<unsigned short>(10),
           "example3", 7UL, 1);
  LOG_INFO(logger, "fox dog over {} {} {} {} {}", std::string("str1"),
           static_cast<unsigned short>(10), 4.0f, 2, static_cast<short>(9));
  LOG_INFO(logger, "logging brown fox {}", 3.0);
  LOG_INFO(logger, "example fox over brown {} {} {}", 3.0, "example3", 1);
  LOG_INFO(logger, "quick dog lazy jumps {} {} {} {} {} {} {} {} {} {}", 8ULL, "example2",
           "example3", 1, 5L, false, 3.0, std::string_view("view2"), "example1", true);
  LOG_INFO(logger, "jumps dog test {} {} {} {} {} {} {} {} {} {}", 5L, 2, "example1", 1, 7UL, false,
           "example2", static_cast<unsigned short>(10), true, std::string("str1"));
  LOG_INFO(logger, "quick logging lazy example {} {}", false, "example3");
  LOG_INFO(logger, "example logging test {} {} {} {} {} {} {} {} {} {}", 6LL, 1,
           std::string("str1"), "example2", 3.0, 8ULL, "example1", 5L, 2, "example3");
  LOG_INFO(logger, "example jumps dog quick {} {} {} {} {} {} {}", 7UL, static_cast<unsigned short>(10),
           6LL, "example1", std::string_view("view2"), std::string("str1"), std::string("str2"));
  LOG_INFO(logger, "quick logging jumps example {} {}", "example3", static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog example lazy test {} {} {} {} {}", std::string("str2"),
           std::string_view("view1"), 6LL, static_cast<short>(9), static_cast<unsigned short>(10));
  LOG_INFO(logger, "lazy logging jumps dog {} {} {} {} {} {} {} {} {}", 1, "example2",
           std::string_view("view2"), true, static_cast<short>(9), 5L, std::string("str1"),
           "example3", static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging jumps dog lazy {} {} {} {} {} {} {} {} {}", false, std::string("str1"),
           std::string_view("view2"), "example2", 2, static_cast<short>(9),
           std::string_view("view1"), 6LL, 7UL);
  LOG_INFO(logger, "test lazy example {} {} {} {} {}", static_cast<short>(9), true, std::string("str1"), 1, 2);
  LOG_INFO(logger, "quick lazy test {} {}", std::string("str2"), 8ULL);
  LOG_INFO(logger, "logging quick example dog {} {} {} {} {}", 1, 3.0, static_cast<short>(9), 6LL,
           std::string("str2"));
  LOG_INFO(logger, "logging quick test {} {} {} {} {} {} {}", 8ULL, 7UL,
           static_cast<unsigned short>(10), 1, "example3", 4.0f, "example1");
  LOG_INFO(logger, "quick logging over jumps {} {}", 3.0, 1);
  LOG_INFO(logger, "over example brown jumps {}", 8ULL);
  LOG_INFO(logger, "dog jumps brown logging {} {} {} {} {} {} {} {}", "example2", std::string_view("view2"),
           std::string_view("view1"), 8ULL, static_cast<unsigned short>(10), 5L, std::string("str2"), 7UL);
  LOG_INFO(logger, "test example brown jumps {} {} {}", 6LL, 5L, 8ULL);
  LOG_INFO(logger, "jumps example quick over {} {} {} {} {} {} {} {} {} {}", 5L, "example1", false,
           3.0, "example3", std::string("str2"), 2, std::string("str1"), true, 4.0f);
  LOG_INFO(logger, "dog brown jumps lazy {} {} {} {} {} {} {} {} {} {}", 1, 3.0,
           static_cast<unsigned short>(10), "example1", std::string("str2"),
           std::string_view("view1"), std::string_view("view2"), 4.0f, "example2", false);
  LOG_INFO(logger, "fox dog over {} {} {} {} {} {} {} {} {} {}", 5L, 6LL, true, "example3",
           static_cast<unsigned short>(10), "example2", std::string("str1"), false, std::string("str2"), 8ULL);
  LOG_INFO(logger, "lazy jumps test dog {} {} {} {} {} {} {} {} {} {}", 5L, 8ULL, false, 7UL, 2,
           static_cast<short>(9), "example2", static_cast<unsigned short>(10), "example3", 6LL);
  LOG_INFO(logger, "over lazy example {} {} {}", std::string("str2"), "example2", std::string("str1"));
  LOG_INFO(logger, "fox dog example logging {} {} {} {} {}", std::string_view("view1"), "example1",
           3.0, 7UL, 2);
  LOG_INFO(logger, "example logging lazy {} {} {} {} {} {}", 4.0f, true, "example3",
           std::string_view("view1"), std::string("str2"), std::string("str1"));
  LOG_INFO(logger, "lazy fox quick over {} {} {} {} {}", 1, 7UL, 4.0f, 3.0, 2);
  LOG_INFO(logger, "logging quick brown fox {} {} {} {} {} {} {} {} {} {}", false,
           std::string_view("view2"), std::string("str1"), std::string_view("view1"), 3.0,
           "example2", 4.0f, 7UL, static_cast<short>(9), std::string("str2"));
  LOG_INFO(logger, "quick example fox {} {} {} {} {} {} {} {}", 5L, 8ULL,
           static_cast<unsigned short>(10), false, std::string("str2"), "example2", 7UL, 6LL);
  LOG_INFO(logger, "logging fox example {} {} {} {} {} {} {} {}", 2, 8ULL, 4.0f, true,
           static_cast<unsigned short>(10), 5L, 1, 6LL);
  LOG_INFO(logger, "over example fox test {} {} {} {} {} {}", 6LL, 7UL, std::string_view("view2"), 2, 3.0, true);
  LOG_INFO(logger, "lazy test brown {} {} {}", 8ULL, "example1", false);
  LOG_INFO(logger, "dog over jumps {} {} {}", std::string_view("view1"), 3.0, 6LL);
  LOG_INFO(logger, "over dog fox {} {} {} {} {}", std::string_view("view2"), "example2", false,
           static_cast<unsigned short>(10), std::string("str1"));
  LOG_INFO(logger, "over dog jumps {} {} {} {} {} {} {} {}", 7UL, 2, true, std::string("str1"),
           static_cast<unsigned short>(10), "example2", 3.0, 6LL);
  LOG_INFO(logger, "brown quick over {} {} {} {} {} {}", std::string_view("view1"),
           static_cast<unsigned short>(10), 5L, 6LL, "example1", std::string_view("view2"));
  LOG_INFO(logger, "logging jumps example brown {} {}", static_cast<unsigned short>(10), false);
  LOG_INFO(logger, "fox quick test brown {} {} {} {} {} {} {} {}", 7UL, static_cast<short>(9),
           "example1", false, 6LL, std::string_view("view1"), std::string("str2"),
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps test brown lazy {} {} {} {} {}", 1, 2, std::string("str1"),
           static_cast<short>(9), "example1");
  LOG_INFO(logger, "logging test lazy {} {} {} {} {} {} {}", false, static_cast<short>(9), 3.0,
           std::string("str1"), true, 6LL, static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog logging fox lazy {} {} {} {} {} {}", 4.0f, 6LL, 1, 2, std::string("str1"),
           std::string_view("view2"));
  LOG_INFO(logger, "lazy dog logging test {} {} {} {} {}", "example1", 8ULL,
           std::string_view("view1"), 7UL, std::string_view("view2"));
  LOG_INFO(logger, "quick jumps dog over {} {} {} {} {} {}", 7UL, "example1", 2, false, true, 1);
  LOG_INFO(logger, "logging jumps lazy {} {} {} {} {} {} {} {}", std::string_view("view2"), 1, 2,
           std::string_view("view1"), "example3", "example2", 3.0, true);
  LOG_INFO(logger, "jumps example over {} {} {} {} {} {} {} {} {} {}", 1, std::string_view("view2"),
           std::string_view("view1"), 3.0, 8ULL, static_cast<unsigned short>(10), "example1", false,
           std::string("str1"), 6LL);
  LOG_INFO(logger, "test quick example brown {}", "example3");
  LOG_INFO(logger, "jumps over example lazy {} {} {} {} {}", static_cast<short>(9), 5L, "example1",
           "example3", std::string("str2"));
  LOG_INFO(logger, "dog fox over {} {} {}", "example3", 3.0, "example1");
  LOG_INFO(logger, "over logging jumps {} {} {} {} {} {} {} {} {}", std::string_view("view2"), 8ULL,
           "example3", true, "example2", 3.0, std::string_view("view1"), std::string("str1"), false);
  LOG_INFO(logger, "dog lazy brown {} {} {} {} {} {} {}", "example1", std::string_view("view1"),
           static_cast<unsigned short>(10), std::string("str1"), false, 3.0, true);
  LOG_INFO(logger, "logging quick fox {} {} {} {} {} {}", 7UL, 3.0, static_cast<unsigned short>(10),
           4.0f, "example1", std::string_view("view1"));
  LOG_INFO(logger, "lazy quick example jumps {} {} {} {} {} {}", static_cast<short>(9), false,
           "example2", 7UL, 1, "example1");
  LOG_INFO(logger, "quick brown example {} {}", 4.0f, 7UL);
  LOG_INFO(logger, "over brown logging {} {} {} {} {}", std::string_view("view2"),
           std::string_view("view1"), 7UL, 2, static_cast<unsigned short>(10));
  LOG_INFO(logger, "brown over jumps logging {} {} {} {} {} {} {} {} {} {}", true, std::string("str1"), 1,
           static_cast<short>(9), false, 4.0f, static_cast<unsigned short>(10), 2, 6LL, "example1");
  LOG_INFO(logger, "jumps fox lazy test {} {} {} {} {} {} {} {} {} {}", 2, true, "example2", false,
           std::string("str1"), 5L, 4.0f, std::string_view("view2"), static_cast<short>(9), 7UL);
  LOG_INFO(logger, "example fox test {} {} {} {} {} {}", false, 8ULL, std::string("str2"), true,
           7UL, std::string_view("view1"));
  LOG_INFO(logger, "logging fox over {} {} {} {} {} {} {}", static_cast<unsigned short>(10), 8ULL,
           std::string_view("view2"), 6LL, std::string("str1"), 4.0f, 3.0);
  LOG_INFO(logger, "test dog lazy {} {} {} {} {} {}", std::string_view("view1"),
           static_cast<short>(9), std::string_view("view2"), 5L, false, 2);
  LOG_INFO(logger, "brown over quick {} {} {}", 8ULL, 7UL, false);
  LOG_INFO(logger, "lazy quick dog {} {} {} {} {}", "example1", "example2", "example3",
           std::string_view("view1"), 8ULL);
  LOG_INFO(logger, "over dog example {} {} {} {} {} {} {}", "example2", 6LL, false, "example1", 2,
           static_cast<unsigned short>(10), 7UL);
  LOG_INFO(logger, "brown test jumps {} {} {}", 4.0f, true, 6LL);
  LOG_INFO(logger, "jumps over logging quick {} {} {} {} {} {} {}", 6LL, 7UL,
           static_cast<unsigned short>(10), 3.0, 5L, 8ULL, 4.0f);
  LOG_INFO(logger, "lazy quick fox test {} {} {} {} {} {}", static_cast<unsigned short>(10), 4.0f,
           std::string_view("view1"), 1, 2, true);
  LOG_INFO(logger, "test example dog {} {}", "example2", 1);
  LOG_INFO(logger, "fox over brown quick {} {} {} {} {} {} {} {} {} {}", 2, "example3", 8ULL,
           "example2", std::string("str1"), static_cast<unsigned short>(10), std::string("str2"),
           static_cast<short>(9), std::string_view("view2"), 4.0f);
  LOG_INFO(logger, "test dog quick fox {} {} {} {} {} {} {} {} {}", 5L, "example2", true,
           "example1", "example3", false, 4.0f, 3.0, std::string_view("view2"));
  LOG_INFO(logger, "example fox dog over {} {} {} {} {} {} {}", true, static_cast<short>(9),
           std::string_view("view1"), 1, 5L, 4.0f, std::string("str2"));
  LOG_INFO(logger, "lazy brown fox over {}", std::string("str1"));
  LOG_INFO(logger, "test over dog logging {} {} {} {} {} {}", false, 8ULL,
           static_cast<unsigned short>(10), 2, 4.0f, "example1");
  LOG_INFO(logger, "brown fox over {} {} {} {} {} {} {} {} {} {}", std::string_view("view1"), 8ULL,
           6LL, false, "example3", 5L, std::string("str1"), "example2", "example1", 3.0);
  LOG_INFO(logger, "over test example {} {} {}", false, 8ULL, 3.0);
  LOG_INFO(logger, "example fox dog brown {} {} {} {} {} {} {}", std::string("str2"), "example1",
           4.0f, 1, static_cast<unsigned short>(10), 3.0, 6LL);
  LOG_INFO(logger, "brown fox lazy example {} {}", 4.0f, 2);
  LOG_INFO(logger, "over logging example {} {} {} {} {} {} {} {}", 2, std::string("str1"),
           "example1", 6LL, 3.0, std::string("str2"), "example2", true);
  LOG_INFO(logger, "dog logging test lazy {} {} {} {} {} {} {} {} {} {}", 1, std::string("str2"),
           "example1", "example3", std::string_view("view2"), std::string_view("view1"),
           static_cast<short>(9), 4.0f, 7UL, 3.0);
  LOG_INFO(logger, "jumps over test {} {} {} {} {} {} {} {} {}", 3.0, 4.0f,
           static_cast<unsigned short>(10), std::string("str1"), std::string("str2"), 5L, 7UL, true, false);
  LOG_INFO(logger, "over example quick brown {} {}", 6LL, true);
  LOG_INFO(logger, "logging over brown quick {} {} {}", "example2", static_cast<short>(9), 7UL);
  LOG_INFO(logger, "logging brown test fox {} {} {} {} {} {} {} {} {}", true, 7UL,
           std::string("str1"), std::string_view("view2"), static_cast<unsigned short>(10), 4.0f,
           std::string_view("view1"), "example3", false);
  LOG_INFO(logger, "example dog over {} {} {} {} {} {} {}", 2, 6LL, false, 7UL,
           static_cast<unsigned short>(10), 1, std::string_view("view1"));
  LOG_INFO(logger, "over quick lazy test {} {} {} {} {} {} {} {} {}", std::string_view("view2"), 1,
           5L, "example3", std::string("str1"), static_cast<unsigned short>(10),
           static_cast<short>(9), 4.0f, 6LL);
  LOG_INFO(logger, "jumps over example {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "logging quick lazy {} {}", 4.0f, 8ULL);
  LOG_INFO(logger, "over test logging dog {}", 7UL);
  LOG_INFO(logger, "test jumps brown quick {} {} {} {} {}", std::string("str1"),
           static_cast<short>(9), 3.0, std::string("str2"), std::string_view("view2"));
  LOG_INFO(logger, "test logging over {} {} {} {} {} {} {} {} {} {}", std::string_view("view1"),
           "example1", "example2", 2, false, "example3", 8ULL, 1, true, 7UL);
  LOG_INFO(logger, "lazy dog logging {} {} {} {} {} {} {} {} {} {}", std::string("str2"), 2,
           "example2", 4.0f, "example1", static_cast<unsigned short>(10), std::string("str1"), 7UL,
           static_cast<short>(9), std::string_view("view2"));
  LOG_INFO(logger, "jumps example quick {} {} {} {} {} {} {}", 8ULL, std::string("str1"),
           static_cast<unsigned short>(10), false, static_cast<short>(9), "example1", 4.0f);
  LOG_INFO(logger, "logging fox dog {} {} {} {} {} {} {} {} {}", 4.0f, "example1", 1, 7UL, true,
           std::string_view("view1"), false, static_cast<unsigned short>(10), std::string_view("view2"));
  LOG_INFO(logger, "logging example dog test {} {} {} {} {}", false, static_cast<short>(9), 7UL,
           static_cast<unsigned short>(10), 8ULL);
  LOG_INFO(logger, "jumps brown test example {} {} {}", false, std::string_view("view2"), std::string("str2"));
  LOG_INFO(logger, "quick test lazy dog {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"), 6LL, 3.0,
           std::string("str2"), static_cast<unsigned short>(10), 4.0f, 5L, true, false, "example3");
  LOG_INFO(logger, "quick brown fox {} {} {} {} {} {} {} {} {}", 8ULL, "example1", false,
           "example2", 3.0, 4.0f, 6LL, true, 5L);
  LOG_INFO(logger, "test brown over {}", std::string_view("view1"));
  LOG_INFO(logger, "logging brown quick over {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps lazy over {}", std::string_view("view2"));
  LOG_INFO(logger, "brown dog jumps over {} {} {}", std::string_view("view1"), 8ULL, "example3");
  LOG_INFO(logger, "quick test dog fox {} {} {} {} {} {} {} {} {}", true, false, static_cast<short>(9),
           std::string("str1"), "example1", 8ULL, std::string_view("view1"), 4.0f, std::string_view("view2"));
  LOG_INFO(logger, "test logging brown dog {} {} {} {} {} {} {} {} {} {}", 7UL, 6LL, 2,
           static_cast<unsigned short>(10), 4.0f, std::string_view("view2"), 1, false,
           static_cast<short>(9), "example2");
  LOG_INFO(logger, "lazy fox brown quick {}", "example2");
  LOG_INFO(logger, "dog fox example quick {} {} {} {} {}", static_cast<short>(9), false, true, 3.0,
           std::string_view("view1"));
  LOG_INFO(logger, "over jumps example quick {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           false, "example3", 1, 7UL, 2, true, 4.0f, "example1", static_cast<short>(9));
  LOG_INFO(logger, "fox dog brown {}", 8ULL);
  LOG_INFO(logger, "example fox dog {} {} {} {} {} {}", false, std::string_view("view2"),
           static_cast<short>(9), std::string("str1"), "example3", "example2");
  LOG_INFO(logger, "lazy test jumps {} {}", "example2", 6LL);
  LOG_INFO(logger, "logging example fox {} {} {} {} {} {} {} {}", static_cast<short>(9), 3.0,
           "example3", 1, std::string_view("view1"), std::string("str1"),
           static_cast<unsigned short>(10), "example2");
  LOG_INFO(logger, "lazy over fox example {} {} {} {} {} {} {} {} {} {}", static_cast<short>(9), 7UL, false,
           3.0, 4.0f, std::string_view("view1"), "example2", std::string_view("view2"), "example1", 8ULL);
  LOG_INFO(logger, "quick lazy logging {} {} {} {} {} {} {}", false, std::string_view("view1"),
           "example1", true, "example3", std::string_view("view2"), static_cast<short>(9));
  LOG_INFO(logger, "dog lazy jumps {} {} {} {}", 3.0, std::string("str2"), 1, 6LL);
  LOG_INFO(logger, "dog example brown test {} {} {} {} {} {}", 6LL, 2, 8ULL,
           static_cast<unsigned short>(10), "example2", 1);
  LOG_INFO(logger, "lazy jumps dog {} {} {} {} {} {} {} {} {} {}", 3.0, static_cast<short>(9), 6LL,
           std::string_view("view2"), true, 5L, std::string("str2"), "example2", "example3",
           std::string_view("view1"));
  LOG_INFO(logger, "fox dog logging quick {} {}", "example1", "example2");
  LOG_INFO(logger, "dog jumps example over {} {} {} {} {} {} {}", "example1", 2, 5L, 8ULL,
           std::string("str2"), 6LL, "example3");
  LOG_INFO(logger, "dog lazy quick {} {} {} {} {} {} {} {} {}", 8ULL, std::string("str1"),
           "example3", 3.0, std::string_view("view1"), true, static_cast<short>(9), 1,
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "jumps brown lazy {} {} {} {} {} {} {}", std::string("str2"), 2,
           static_cast<unsigned short>(10), true, "example3", std::string_view("view2"), 6LL);
  LOG_INFO(logger, "brown dog example {} {} {}", std::string("str2"), static_cast<unsigned short>(10), true);
  LOG_INFO(logger, "fox brown lazy {} {} {} {} {} {}", 6LL, "example1", std::string("str1"), true,
           std::string("str2"), "example3");
  LOG_INFO(logger, "quick over jumps logging {} {}", 4.0f, std::string("str1"));
  LOG_INFO(logger, "dog fox brown quick {} {} {} {} {} {} {} {} {}", "example2", std::string("str2"), 7UL,
           static_cast<unsigned short>(10), 6LL, 8ULL, std::string_view("view2"), "example1", 4.0f);
  LOG_INFO(logger, "quick example dog {} {} {} {} {} {}", false, "example1", true, 5L, 3.0,
           static_cast<unsigned short>(10));
  LOG_INFO(logger, "test brown lazy dog {} {}", std::string("str2"), 2);
  LOG_INFO(logger, "dog logging example {} {} {} {} {} {} {} {} {}", 7UL, std::string("str1"), 8ULL,
           6LL, 2, false, "example3", std::string_view("view1"), std::string("str2"));
  LOG_INFO(logger, "dog test quick {} {} {} {} {}", std::string_view("view2"), 2,
           static_cast<unsigned short>(10), "example3", 8ULL);
  LOG_INFO(logger, "lazy example dog {}", static_cast<short>(9));
  LOG_INFO(logger, "example dog logging {} {} {} {} {} {} {} {}", 5L, 8ULL, 2, false, "example2",
           true, static_cast<short>(9), 4.0f);
  LOG_INFO(logger, "test logging example brown {} {} {} {} {} {} {} {} {}", "example3", 4.0f, 5L,
           static_cast<short>(9), 2, std::string_view("view1"), std::string_view("view2"), true, 6LL);
  LOG_INFO(logger, "over example brown {} {} {} {} {}", 1, "example2", std::string_view("view2"),
           std::string_view("view1"), "example3");
  LOG_INFO(logger, "over example lazy quick {}", 8ULL);
  LOG_INFO(logger, "logging dog fox {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string_view("view2"), 8ULL, 1, "example3", static_cast<short>(9), std::string("str2"), true);
  LOG_INFO(logger, "quick dog example {} {} {} {} {} {} {} {}", 1, 3.0, std::string("str2"),
           static_cast<short>(9), std::string_view("view1"), static_cast<unsigned short>(10),
           "example3", 4.0f);
  LOG_INFO(logger, "dog quick logging jumps {} {} {} {} {}", 2, std::string("str1"), "example2", 1, true);
  LOG_INFO(logger, "dog fox logging test {} {} {} {} {} {} {} {} {} {}", 1, static_cast<short>(9),
           std::string_view("view2"), "example1", 3.0, 5L, true, 2, std::string_view("view1"), 8ULL);
  LOG_INFO(logger, "dog jumps fox quick {} {}", true, 2);
  LOG_INFO(logger, "jumps over dog quick {} {} {} {} {} {} {} {} {} {}", "example2", 6LL, 8ULL, 1,
           4.0f, 7UL, 2, std::string_view("view1"), false, 5L);
  LOG_INFO(logger, "logging example quick lazy {} {} {} {} {} {} {} {} {} {}", "example3", 2, false,
           std::string("str1"), 1, "example2", std::string_view("view2"), 6LL, true, 7UL);
  LOG_INFO(logger, "fox quick jumps logging {} {} {} {} {} {} {} {} {} {}",
           static_cast<unsigned short>(10), 1, std::string_view("view1"), static_cast<short>(9),
           false, true, std::string("str2"), 3.0, 5L, std::string("str1"));
  LOG_INFO(logger, "fox dog brown {} {}", std::string("str1"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox jumps lazy {} {} {} {}", std::string_view("view1"), "example3", false,
           std::string("str2"));
  LOG_INFO(logger, "brown lazy example fox {} {}", "example2", 1);
  LOG_INFO(logger, "quick lazy example jumps {} {}", true, std::string_view("view1"));
  LOG_INFO(logger, "test quick fox {} {} {} {}", 6LL, std::string("str1"), 8ULL, std::string("str2"));
  LOG_INFO(logger, "dog logging test lazy {}", std::string("str1"));
  LOG_INFO(logger, "lazy over dog example {} {} {} {} {} {} {}", 7UL, 8ULL, std::string_view("view1"),
           "example2", std::string("str2"), static_cast<unsigned short>(10), 5L);
  LOG_INFO(logger, "example logging brown jumps {} {} {} {} {} {} {} {}", 5L, 6LL,
           std::string_view("view2"), std::string_view("view1"), 2, true, 7UL, std::string("str1"));
  LOG_INFO(logger, "fox brown lazy over {} {} {} {} {} {} {} {} {} {}", 2, "example1", 6LL, 4.0f, 5L,
           true, std::string_view("view2"), std::string_view("view1"), "example3", std::string("str2"));
  LOG_INFO(logger, "logging dog brown lazy {} {} {} {} {} {} {} {} {} {}", 4.0f, "example3",
           "example2", std::string_view("view2"), 5L, false, std::string("str2"),
           std::string_view("view1"), 7UL, std::string("str1"));
  LOG_INFO(logger, "test fox quick brown {} {} {} {} {} {} {} {} {}", false, static_cast<unsigned short>(10),
           std::string("str1"), true, "example2", 2, 7UL, std::string_view("view1"), 4.0f);
  LOG_INFO(logger, "over brown jumps {} {} {} {} {} {} {} {} {}", 6LL, static_cast<short>(9), false,
           "example3", 5L, 8ULL, 1, std::string("str1"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "example brown dog lazy {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"),
           false, static_cast<short>(9), true, 1, 6LL, 8ULL, 7UL, 3.0, "example2");
  LOG_INFO(logger, "lazy dog over {} {} {} {} {} {}", std::string("str2"), "example3",
           std::string_view("view1"), "example2", static_cast<short>(9), static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox example quick lazy {} {} {} {}", 8ULL, 2, static_cast<short>(9), true);
  LOG_INFO(logger, "test over fox {} {} {} {} {} {} {}", false, "example2", "example1", 2, 1,
           "example3", 5L);
  LOG_INFO(logger, "test lazy example {} {} {} {} {} {} {}", 8ULL, std::string("str1"),
           std::string_view("view2"), "example2", std::string("str2"), static_cast<short>(9), 4.0f);
  LOG_INFO(logger, "jumps example over dog {} {} {} {} {} {} {} {} {} {}", std::string_view("view1"), 3.0,
           std::string("str2"), "example2", true, static_cast<short>(9), 4.0f, 5L, "example3", false);
  LOG_INFO(logger, "logging example test {} {} {} {}", static_cast<unsigned short>(10), "example1",
           static_cast<short>(9), "example2");
  LOG_INFO(logger, "quick fox test brown {} {} {}", 6LL, true, 7UL);
  LOG_INFO(logger, "over logging test lazy {} {} {} {}", 1, 8ULL, static_cast<short>(9), "example1");
  LOG_INFO(logger, "example lazy quick {} {} {} {} {} {} {} {}", "example2", "example1", 7UL, 1,
           std::string("str2"), std::string("str1"), 6LL, 4.0f);
  LOG_INFO(logger, "lazy brown dog {}", static_cast<short>(9));
  LOG_INFO(logger, "test quick jumps example {}", 6LL);
  LOG_INFO(logger, "dog quick lazy {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           std::string("str1"), 5L, 8ULL, "example3", 3.0, "example2");
  LOG_INFO(logger, "brown logging lazy quick {} {} {} {} {} {}", 2, 7UL, "example2", true,
           "example3", static_cast<short>(9));
  LOG_INFO(logger, "test brown fox dog {} {} {} {} {}", std::string("str1"), "example3", false,
           std::string_view("view1"), 6LL);
  LOG_INFO(logger, "logging test over dog {}", std::string_view("view1"));
  LOG_INFO(logger, "test dog brown {} {} {} {}", 3.0, 1, 6LL, std::string_view("view1"));
  LOG_INFO(logger, "example test quick {}", std::string_view("view2"));
  LOG_INFO(logger, "dog over fox {} {} {} {} {}", 6LL, static_cast<short>(9), 4.0f, 2, true);
  LOG_INFO(logger, "logging test fox {} {} {}", 1, 3.0, 7UL);
  LOG_INFO(logger, "over lazy dog jumps {} {} {} {} {}", std::string("str1"), false,
           std::string_view("view2"), 6LL, static_cast<unsigned short>(10));
  LOG_INFO(logger, "test quick over example {} {} {} {} {} {} {} {}", std::string_view("view1"),
           3.0, 6LL, 5L, static_cast<short>(9), 2, 7UL, false);
  LOG_INFO(logger, "example test lazy logging {}", 3.0);
  LOG_INFO(logger, "jumps lazy quick over {} {} {} {} {} {} {}", std::string_view("view2"), false,
           std::string("str1"), true, 6LL, 1, 2);
  LOG_INFO(logger, "fox jumps quick {} {} {} {} {}", true, static_cast<unsigned short>(10),
           "example3", 1, "example2");
  LOG_INFO(logger, "jumps dog lazy {} {} {} {} {} {} {} {}", false, std::string_view("view2"), 4.0f,
           "example2", static_cast<short>(9), std::string("str2"), 3.0, std::string_view("view1"));
  LOG_INFO(logger, "fox lazy logging jumps {}", 1);
  LOG_INFO(logger, "brown fox example {} {}", "example3", static_cast<short>(9));
  LOG_INFO(logger, "fox lazy jumps logging {} {} {} {} {} {}", std::string_view("view2"), 4.0f, 2, 6LL, 5L, 3.0);
  LOG_INFO(logger, "fox lazy logging jumps {} {} {} {} {} {} {}", std::string("str2"),
           std::string_view("view1"), 6LL, 1, std::string_view("view2"), 5L, static_cast<short>(9));
  LOG_INFO(logger, "quick dog jumps {} {}", std::string("str1"), 6LL);
  LOG_INFO(logger, "dog quick jumps example {} {} {} {} {} {} {} {}", 1, 3.0, false, 6LL,
           static_cast<short>(9), "example2", 4.0f, static_cast<unsigned short>(10));
  LOG_INFO(logger, "quick example logging brown {} {} {} {} {} {} {}", 5L, "example3", 4.0f, 7UL,
           static_cast<unsigned short>(10), "example2", false);
  LOG_INFO(logger, "lazy jumps test {} {} {} {} {} {} {} {}", 2, true, 4.0f, 8ULL,
           std::string_view("view1"), 6LL, "example2", std::string_view("view2"));
  LOG_INFO(logger, "lazy jumps dog {} {} {} {} {} {} {}", std::string_view("view1"), "example3",
           "example2", static_cast<short>(9), "example1", 6LL, std::string_view("view2"));
  LOG_INFO(logger, "jumps quick test {} {} {} {} {} {} {} {}", 2, 8ULL, std::string("str1"), false,
           1, std::string_view("view1"), static_cast<short>(9), std::string("str2"));
  LOG_INFO(logger, "example jumps lazy {} {} {} {} {}", 7UL, "example2", 3.0, std::string("str2"), true);
  LOG_INFO(logger, "jumps quick over example {} {} {}", 6LL, 7UL, std::string("str2"));
  LOG_INFO(logger, "lazy dog jumps test {} {}", 7UL, true);
  LOG_INFO(logger, "lazy jumps example {} {} {} {} {} {} {} {} {}", false, 7UL, true, 5L,
           "example3", std::string("str2"), "example2", 1, static_cast<short>(9));
  LOG_INFO(logger, "dog logging fox lazy {} {} {} {} {}", 8ULL, static_cast<unsigned short>(10),
           std::string("str1"), 7UL, 6LL);
  LOG_INFO(logger, "logging over fox lazy {} {} {} {} {} {} {} {}", 2, std::string_view("view2"),
           true, 6LL, std::string("str2"), 8ULL, std::string("str1"), 1);
  LOG_INFO(logger, "lazy fox brown {} {}", "example1", "example3");
  LOG_INFO(logger, "fox quick test over {} {} {}", "example3", 1, std::string("str2"));
  LOG_INFO(logger, "lazy over brown {} {} {} {} {} {} {} {}", true, std::string_view("view1"), 7UL,
           5L, std::string("str1"), std::string("str2"), 4.0f, static_cast<short>(9));
  LOG_INFO(logger, "test jumps brown dog {} {} {} {} {} {} {} {} {} {}", 8ULL, "example1",
           "example3", 1, static_cast<short>(9), std::string_view("view2"), 4.0f,
           std::string_view("view1"), 3.0, 7UL);
  LOG_INFO(logger, "fox example jumps {}", static_cast<unsigned short>(10));
  LOG_INFO(logger, "dog lazy quick {} {} {} {} {} {}", false, static_cast<short>(9),
           std::string_view("view1"), 1, "example3", std::string("str1"));
  LOG_INFO(logger, "jumps fox over logging {} {}", 1, "example1");
  LOG_INFO(logger, "lazy example test {} {} {}", 4.0f, 7UL, 8ULL);
  LOG_INFO(logger, "fox brown example {} {} {} {} {}", std::string_view("view1"),
           std::string("str1"), std::string("str2"), std::string_view("view2"), 4.0f);
  LOG_INFO(logger, "logging lazy jumps {} {} {} {} {} {} {}", 4.0f, 5L, static_cast<short>(9), 6LL,
           false, std::string_view("view2"), static_cast<unsigned short>(10));
  LOG_INFO(logger, "fox over logging {} {} {} {} {} {} {} {} {} {}", std::string_view("view2"), 5L,
           8ULL, "example3", 6LL, false, 2, "example1", 7UL, static_cast<short>(9));
  LOG_INFO(logger, "over example logging dog {} {} {} {} {} {}", 4.0f, 8ULL, "example2", true,
           static_cast<unsigned short>(10), static_cast<short>(9));
  LOG_INFO(logger, "example lazy dog {} {} {} {} {}", 8ULL, std::string_view("view2"),
           std::string("str2"), std::string("str1"), 1);
  LOG_INFO(logger, "fox example quick dog {} {} {} {} {} {} {} {} {}", "example2", false, 5L, 7UL,
           std::string("str2"), static_cast<short>(9), true, "example1", "example3");
  LOG_INFO(logger, "logging quick over {} {} {} {} {} {} {} {} {} {}", 2, 4.0f, "example3", std::string("str2"),
           std::string("str1"), 7UL, static_cast<unsigned short>(10), "example2", 8ULL, 3.0);
  LOG_INFO(logger, "fox over jumps brown {} {} {} {} {} {} {} {}", std::string_view("view2"),
           std::string("str2"), "example2", 3.0, 5L, false, "example3", 6LL);
  LOG_INFO(logger, "test logging quick dog {} {} {} {} {} {} {} {} {}", 6LL, 3.0, "example2", true,
           false, static_cast<unsigned short>(10), std::string("str2"), std::string("str1"), 8ULL);
  LOG_INFO(logger, "test logging quick lazy {} {}", std::string_view("view2"), 4.0f);
  LOG_INFO(logger, "logging brown example {} {} {}", 4.0f, "example1", std::string("str2"));
  LOG_INFO(logger, "jumps test over {} {}", std::string("str1"), std::string("str2"));
  LOG_INFO(logger, "fox jumps quick {} {} {} {} {} {}", 6LL, std::string("str2"), false, 7UL,
           std::string_view("view2"), 4.0f);
  LOG_INFO(logger, "quick over jumps logging {}", false);
  LOG_INFO(logger, "test dog logging {} {} {} {} {}", std::string("str2"), 5L, "example2",
           static_cast<unsigned short>(10), 1);
  LOG_INFO(logger, "example dog jumps lazy {} {} {} {} {}", "example2", 2, 7UL, false, static_cast<short>(9));
  LOG_INFO(logger, "example test logging {} {} {} {} {} {} {} {} {} {}", 6LL, 1, 3.0,
           static_cast<unsigned short>(10), std::string("str1"), "example2", 5L, "example3", 4.0f,
           static_cast<short>(9));
  LOG_INFO(logger, "lazy brown over logging {}", 7UL);
  LOG_INFO(logger, "over lazy quick fox {} {} {} {} {}", 8ULL, 1, 7UL, 5L, "example3");
  LOG_INFO(logger, "over example fox {} {} {}", true, 2, static_cast<short>(9));
  LOG_INFO(logger, "fox over brown jumps {} {} {}", std::string_view("view2"), 5L, 4.0f);
  LOG_INFO(logger, "brown jumps quick {}", "example1");
  LOG_INFO(logger, "test jumps lazy {} {} {} {} {}", 6LL, 8ULL, 4.0f, "example1", 2);
  LOG_INFO(logger, "dog fox lazy {} {}", 3.0, false);
  LOG_INFO(logger, "jumps test quick brown {} {} {} {} {} {} {} {}", "example3", 7UL, 2,
           std::string_view("view1"), false, true, "example1", "example2");
  LOG_INFO(logger, "dog over quick {} {} {} {} {} {} {} {} {} {}", 2, 5L, "example2", 7UL,
           "example3", std::string_view("view1"), std::string_view("view2"), std::string("str1"),
           false, "example1");
  LOG_INFO(logger, "quick dog over brown {} {} {} {} {}", 6LL, 4.0f, "example1", 3.0, 1);
  LOG_INFO(logger, "logging over fox {} {}", 2, "example3");
  LOG_INFO(logger, "quick logging example {} {} {} {}", 7UL, 4.0f, 1, 8ULL);
  LOG_INFO(logger, "quick fox brown {} {} {} {} {}", 5L, 3.0, 2, false, "example3");
  LOG_INFO(logger, "quick lazy logging test {} {} {}", static_cast<unsigned short>(10), 7UL, 2);
  LOG_INFO(logger, "example lazy logging {} {} {} {} {}", static_cast<unsigned short>(10), 1, 8ULL,
           7UL, static_cast<short>(9));
  LOG_INFO(logger, "lazy quick logging brown {} {} {} {} {} {} {} {}", static_cast<short>(9), 3.0,
           7UL, true, 5L, std::string_view("view2"), std::string("str1"), "example2");
  LOG_INFO(logger, "over fox test {} {} {}", false, std::string_view("view2"), 8ULL);
  LOG_INFO(logger, "test dog example jumps {} {} {} {}", "example2", 2, true, 8ULL);
  LOG_INFO(logger, "quick lazy dog test {} {} {} {} {} {} {} {} {} {}", 6LL,
           std::string_view("view2"), false, "example2", std::string_view("view1"),
           static_cast<short>(9), "example3", static_cast<unsigned short>(10), 1, std::string("str2"));
  LOG_INFO(logger, "over example quick {} {} {} {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           "example3", "example2", 2, 4.0f, std::string_view("view2"), "example1", false, 8ULL, true);
  LOG_INFO(logger, "lazy test fox dog {} {} {} {} {} {}", std::string_view("view1"), 5L, 2, 4.0f,
           static_cast<short>(9), 1);
  LOG_INFO(logger, "jumps lazy test brown {}", true);
  LOG_INFO(logger, "brown fox over {} {} {} {} {} {} {} {}", 4.0f, 8ULL, 3.0, 5L, 2, 6LL,
           std::string_view("view1"), 1);
  LOG_INFO(logger, "logging over quick test {} {} {} {} {} {} {}", 4.0f, static_cast<short>(9), 1,
           6LL, "example2", 5L, std::string_view("view1"));
  LOG_INFO(logger, "logging test dog {} {} {} {} {} {} {}", static_cast<unsigned short>(10),
           "example3", 2, false, 3.0, 6LL, 4.0f);
  LOG_INFO(logger, "example brown dog test {} {} {} {} {}", 6LL, 8ULL, true, 3.0, std::string_view("view1"));
  LOG_INFO(logger, "logging test jumps quick {} {} {} {}", "example3", 3.0, std::string("str1"), 7UL);
  LOG_INFO(logger, "dog lazy quick {} {} {}", 2, true, static_cast<short>(9));
  LOG_INFO(logger, "quick over example fox {} {} {} {} {} {} {}", 8ULL, static_cast<short>(9), 6LL,
           3.0, 7UL, "example1", 1);
  LOG_INFO(logger, "jumps example fox {} {} {} {} {} {}", std::string_view("view2"), true, 8ULL,
           static_cast<unsigned short>(10), "example2", false);
  LOG_INFO(logger, "jumps logging brown {} {} {} {}", true, std::string_view("view2"), 7UL, std::string("str1"));
  LOG_INFO(logger, "test fox jumps {} {} {}", 2, "example3", static_cast<short>(9));
  LOG_INFO(logger, "jumps lazy quick {}", "example2");
  LOG_INFO(logger, "logging brown over {} {} {} {} {}", "example1", 5L, 3.0, 7UL, 4.0f);
  LOG_INFO(logger, "dog lazy quick test {} {} {} {}", "example3", std::string_view("view1"), 6LL,
           std::string_view("view2"));
  LOG_INFO(logger, "jumps lazy dog {} {} {}", static_cast<short>(9), std::string_view("view2"),
           "example2");
  LOG_INFO(logger, "over fox brown lazy {} {} {} {} {} {} {}", 7UL, 2, "example1", "example3", 5L,
           std::string("str2"), 6LL);
  LOG_INFO(logger, "logging over test {} {} {} {} {} {}", std::string_view("view2"), 1, false, 2, true, 8ULL);
  LOG_INFO(logger, "jumps dog over test {} {} {} {} {} {} {} {} {} {}", 1, "example3", false, 2,
           8ULL, 3.0, 7UL, 5L, static_cast<unsigned short>(10), std::string_view("view2"));
  LOG_INFO(logger, "brown test lazy {} {} {} {} {} {} {} {} {}", std::string("str2"), 4.0f, 5L, 2,
           "example3", static_cast<unsigned short>(10), 8ULL, static_cast<short>(9),
           std::string_view("view1"));
  LOG_INFO(logger, "over dog brown {} {}", "example2", 5L);
  LOG_INFO(logger, "fox lazy over logging {} {} {} {} {} {} {} {} {} {}", std::string("str1"),
           "example2", 2, 1, std::string_view("view1"), 5L, 3.0, true, 8ULL, 7UL);
  LOG_INFO(logger, "fox example over logging {} {} {} {} {}", 2, std::string_view("view1"),
           static_cast<short>(9), 5L, std::string("str2"));
  LOG_INFO(logger, "lazy dog over fox {} {} {} {} {} {} {}", std::string("str1"), 3.0, false, 7UL,
           1, std::string("str2"), 2);
  LOG_INFO(logger, "brown fox quick lazy {} {}", 8ULL, std::string_view("view1"));
  LOG_INFO(logger, "example logging brown lazy {} {} {} {} {} {} {} {} {}", 8ULL, 6LL, 2, true,
           std::string("str2"), static_cast<short>(9), 7UL, "example2", "example3");
  LOG_INFO(logger, "quick over logging brown {} {} {} {}", true, static_cast<unsigned short>(10), 1, 4.0f);
  LOG_INFO(logger, "brown over fox {} {}", std::string("str2"), 8ULL);
  LOG_INFO(logger, "over jumps quick {} {}", false, static_cast<short>(9));
  LOG_INFO(logger, "quick test example {} {} {} {} {} {} {} {} {} {}", static_cast<short>(9),
           std::string("str2"), 3.0, 2, 1, std::string_view("view1"), "example2", false, 8ULL, 5L);
  LOG_INFO(logger, "brown lazy dog test {} {}", static_cast<unsigned short>(10), 3.0);
  LOG_INFO(logger, "jumps fox dog {}", std::string_view("view1"));
  LOG_INFO(logger, "dog fox brown {} {} {} {} {} {} {}", 1, 4.0f, 5L, 8ULL, 3.0, "example1",
           "example3");
  LOG_INFO(logger, "test lazy over {} {} {} {} {} {} {} {}", false, true, std::string_view("view2"),
           3.0, "example3", 7UL, 1, "example1");
  LOG_INFO(logger, "fox dog brown logging {} {} {} {} {}", false, true, 7UL, "example3", std::string("str2"));
  LOG_INFO(logger, "over dog lazy example {} {}", false, "example3");
  LOG_INFO(logger, "lazy jumps dog {} {} {} {}", 4.0f, std::string("str1"), "example2", false);
  LOG_INFO(logger, "lazy test fox {} {} {} {} {} {} {}", 7UL, true, std::string_view("view2"), 2,
           3.0, std::string_view("view1"), 4.0f);
  LOG_INFO(logger, "example jumps test {} {} {} {} {} {} {} {} {} {}", static_cast<short>(9),
           static_cast<unsigned short>(10), 4.0f, std::string("str2"), true, "example3",
           std::string_view("view2"), false, 6LL, 3.0);
  LOG_INFO(logger, "logging brown fox {} {}", std::string_view("view2"), 8ULL);
  LOG_INFO(logger, "lazy test dog jumps {} {} {}", 2, 8ULL, 4.0f);
  LOG_INFO(logger, "fox dog jumps {} {} {} {} {} {} {} {} {} {}", 8ULL, std::string("str2"), false,
           static_cast<short>(9), std::string_view("view2"), static_cast<unsigned short>(10),
           std::string_view("view1"), "example1", 3.0, 7UL);

  return 0;
}