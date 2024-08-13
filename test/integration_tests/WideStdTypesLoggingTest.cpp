#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/core/Filesystem.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Array.h"
#include "quill/std/Deque.h"
#include "quill/std/FilesystemPath.h"
#include "quill/std/ForwardList.h"
#include "quill/std/List.h"
#include "quill/std/Map.h"
#include "quill/std/Optional.h"
#include "quill/std/Pair.h"
#include "quill/std/Set.h"
#include "quill/std/UnorderedMap.h"
#include "quill/std/UnorderedSet.h"
#include "quill/std/Vector.h"
#include "quill/std/WideString.h"

#include <array>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace quill;

struct CWideStringComparator
{
  bool operator()(wchar_t const* a, wchar_t const* b) const { return std::wcscmp(a, b) < 0; }
};

/***/
TEST_CASE("wide_std_types_logging")
{
#if defined(_WIN32)
  static constexpr char const* filename = "wide_std_types_logging.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  // Set writing logging to a file
  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  {
    fs::path wp{L"C:\\some\\path"};
    LOG_INFO(logger, "wp {}", wp);

    fs::path p{"C:\\another\\path"};
    LOG_INFO(logger, "p {}", p);

    LOG_INFO(logger, "wp {} p {}", wp, p);

    std::array<std::wstring, 2> wsa = {L"test", L"string"};
    LOG_INFO(logger, "wsa {}", wsa);

    std::array<std::wstring_view, 2> wsva = {L"test", L"string_view"};
    LOG_INFO(logger, "wsva {}", wsva);

    std::array<wchar_t const*, 4> wscva = {L"c style", L"string test", L"test", L"log"};
    LOG_INFO(logger, "wscva {}", wscva);

    std::deque<std::wstring> wds = {L"test", L"string"};
    LOG_INFO(logger, "wds {}", wds);

    std::deque<std::wstring_view> wdsv = {L"test", L"string_view"};
    LOG_INFO(logger, "wdsv {}", wdsv);

    std::deque<wchar_t const*> wdcs = {L"c style", L"string test", L"test", L"log"};
    LOG_INFO(logger, "wdcs {}", wdcs);

    std::forward_list<std::wstring> wfs = {L"test", L"string"};
    LOG_INFO(logger, "wfs {}", wfs);

    std::forward_list<std::wstring_view> wfsv = {L"test", L"string_view"};
    LOG_INFO(logger, "wfsv {}", wfsv);

    std::forward_list<wchar_t const*> wfcs = {L"c style", L"string test", L"test", L"log"};
    LOG_INFO(logger, "wfcs {}", wfcs);

    std::list<std::wstring> sl = {L"test", L"string"};
    LOG_INFO(logger, "sl {}", sl);

    std::list<std::wstring_view> svl = {L"test", L"string_view"};
    LOG_INFO(logger, "svl {}", svl);

    std::list<wchar_t const*> scl = {L"c style", L"string test", L"test", L"log"};
    LOG_INFO(logger, "scl {}", scl);

    std::vector<std::wstring> wsv = {L"test", L"string"};
    LOG_INFO(logger, "wsv {}", wsv);

    std::vector<std::wstring_view> wsvv = {L"test", L"string_view"};
    LOG_INFO(logger, "wsvv {}", wsvv);

    std::vector<wchar_t const*> wscv = {L"c style", L"string test", L"test", L"log"};
    LOG_INFO(logger, "wscv {}", wscv);

    std::pair<std::wstring, std::wstring> wpss = {L"test", L"string"};
    LOG_INFO(logger, "wpss {}", wpss);

    std::pair<std::wstring_view, std::wstring> wpsv = {L"test", L"string_view"};
    LOG_INFO(logger, "wpsv {}", wpsv);

    std::pair<wchar_t const*, std::wstring> wscs = {L"c style", L"string test"};
    LOG_INFO(logger, "wscs {}", wscs);

    std::pair<int, std::wstring> wscsi = {1231, L"string test"};
    LOG_INFO(logger, "wscsi {}", wscsi);

    std::pair<std::wstring, double> wscsd = {L"string test", 443.2};
    LOG_INFO(logger, "wscsd {}", wscsd);

    std::optional<std::wstring> weo{std::nullopt};
    LOG_INFO(logger, "eo {}", weo);

    std::optional<std::wstring> wos{L"test"};
    LOG_INFO(logger, "wos {}", wos);

    std::optional<std::wstring_view> wosv{L"test"};
    LOG_INFO(logger, "wosv {}", wosv);

    std::optional<wchar_t const*> woc{L"test"};
    LOG_INFO(logger, "woc {}", woc);

    std::set<std::wstring> sa = {L"test", L"string"};
    LOG_INFO(logger, "sa {}", sa);

    std::set<std::wstring> sva = {L"test", L"string_view"};
    LOG_INFO(logger, "sva {}", sva);

    std::set<wchar_t const*, CWideStringComparator> scva = {L"c_style", L"aa", L"string_test",
                                                            L"test", L"log"};
    LOG_INFO(logger, "scva {}", scva);

    std::map<std::wstring, std::wstring> ccm = {{L"4", L"400"}, {L"3", L"300"}, {L"1", L"100"}};
    LOG_INFO(logger, "ccm {}", ccm);

    std::map<std::wstring, int> ccmx = {{L"4", 400}, {L"3", 300}, {L"1", 100}};
    LOG_INFO(logger, "ccmx {}", ccmx);

    std::map<wchar_t const*, std::wstring, CWideStringComparator> ccmc = {
      {L"4", L"400"}, {L"3", L"300"}, {L"1", L"100"}};
    LOG_INFO(logger, "ccmc {}", ccmc);

    std::map<int, std::wstring> ccmi = {{4, L"400"}, {3, L"300"}, {1, L"100"}};
    LOG_INFO(logger, "ccmi {}", ccmi);

    std::unordered_set<std::wstring> uss = {L"test"};
    LOG_INFO(logger, "uss {}", uss);

    std::unordered_set<std::wstring_view> usvs = {L"string_view"};
    LOG_INFO(logger, "usvs {}", usvs);

    std::unordered_set<wchar_t const*> uscs = {L"c_style"};
    LOG_INFO(logger, "uscs {}", uscs);

    std::unordered_map<std::wstring, std::wstring> uccm = {{L"4", L"400"}};
    LOG_INFO(logger, "uccm {}", uccm);

    std::unordered_map<std::wstring, int> uccmx = {{L"5", 500}};
    LOG_INFO(logger, "uccmx {}", uccmx);

    std::unordered_map<wchar_t const*, std::wstring> uccmc = {{L"6", L"600"}};
    LOG_INFO(logger, "uccmc {}", uccmc);

    std::unordered_map<int, std::wstring> uccmi = {{7, L"700"}};
    LOG_INFO(logger, "uccmi {}", uccmi);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wp C:\\some\\path"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       p C:\\another\\path"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wp C:\\some\\path p C:\\another\\path"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wsa [\"test\", \"string\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wsva [\"test\", \"string_view\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wscva [\"c style\", \"string test\", \"test\", \"log\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wds [\"test\", \"string\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wdsv [\"test\", \"string_view\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wdcs [\"c style\", \"string test\", \"test\", \"log\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wfs [\"test\", \"string\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wfsv [\"test\", \"string_view\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wfcs [\"c style\", \"string test\", \"test\", \"log\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sl [\"test\", \"string\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       svl [\"test\", \"string_view\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       scl [\"c style\", \"string test\", \"test\", \"log\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wsv [\"test\", \"string\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wsvv [\"test\", \"string_view\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wscv [\"c style\", \"string test\", \"test\", \"log\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wpss (\"test\", \"string\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wpsv (\"test\", \"string_view\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wscs (\"c style\", \"string test\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wscsi (1231, \"string test\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wscsd (\"string test\", 443.2)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       eo none"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wos optional(\"test\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       wosv optional(\"test\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       woc optional(\"test\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sa [\"string\", \"test\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sva [\"string_view\", \"test\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_INFO      " + logger_name +
                "       scva [\"aa\", \"c_style\", \"log\", \"string_test\", \"test\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ccm [(\"1\", \"100\"), (\"3\", \"300\"), (\"4\", \"400\")]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ccmx [(\"1\", 100), (\"3\", 300), (\"4\", 400)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ccmc [(\"1\", \"100\"), (\"3\", \"300\"), (\"4\", \"400\")]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ccmi [(1, \"100\"), (3, \"300\"), (4, \"400\")]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       uss [\"test\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       usvs [\"string_view\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       uscs [\"c_style\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       uccm [(\"4\", \"400\")]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       uccmx [(\"5\", 500)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       uccmc [(\"6\", \"600\")]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       uccmi [(7, \"700\")]"}));

  testing::remove_file(filename);
#endif
}