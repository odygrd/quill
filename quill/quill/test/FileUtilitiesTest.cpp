#include "quill/detail/misc/FileUtilities.h"
#include "quill/detail/misc/Common.h"
#include <gtest/gtest.h>

using namespace quill;
using namespace quill::detail;
using namespace quill::detail::file_utilities;

/***/
TEST(FileUtilities, extract_stem_and_extension)
{
  {
    // simple file
#if defined(_WIN32)
    filename_t fname = L"logfile";
#else
    filename_t fname = "logfile";
#endif

    auto const res = extract_stem_and_extension(fname);
    EXPECT_EQ(res.first, filename_t{fname});
    EXPECT_EQ(res.second, filename_t{});
  }

  {
    // simple directory
#if defined(_WIN32)
    filename_t fname = L"etc\\eng\\logfile";
#else
    filename_t fname = "etc/eng/logfile";
#endif

    auto const res = extract_stem_and_extension(fname);
    EXPECT_EQ(res.first, filename_t{fname});
    EXPECT_EQ(res.second, filename_t{});
  }

  {
    // no file extension
#if defined(_WIN32)
    filename_t fname = L"logfile.";
#else
    filename_t fname = "logfile.";
#endif
    auto const res = extract_stem_and_extension(fname);
    EXPECT_EQ(res.first, filename_t{fname});
    EXPECT_EQ(res.second, filename_t{});
  }

  {
    // no file extension - directory
#if defined(_WIN32)
    filename_t fname = L"etc\\eng\\logfile.";
#else
    filename_t fname = "etc/eng/logfile.";
#endif

    auto const res = extract_stem_and_extension(fname);
    EXPECT_EQ(res.first, filename_t{fname});
    EXPECT_EQ(res.second, filename_t{});
  }

  {
    // hidden file
#if defined(_WIN32)
    filename_t fname = L".logfile.";
#else
    filename_t fname = ".logfile.";
#endif
    auto const res = extract_stem_and_extension(fname);
    EXPECT_EQ(res.first, filename_t{fname});
    EXPECT_EQ(res.second, filename_t{});
  }

  {
    // hidden file - directory
#if defined(_WIN32)
    filename_t fname = L"etc\\eng\\.logfile";
#else
    filename_t fname = "etc/eng/.logfile";
#endif

    auto const res = extract_stem_and_extension(fname);
    EXPECT_EQ(res.first, filename_t{fname});
    EXPECT_EQ(res.second, filename_t{});
  }

  {
    // valid stem and extension
#if defined(_WIN32)
    filename_t fname = L"logfile.log";
    filename_t fname_expected = L"logfile";
    filename_t extension_expected = L".log";
#else
    filename_t fname = "logfile.log";
    filename_t fname_expected = "logfile";
    filename_t extension_expected = ".log";
#endif
    auto const res = extract_stem_and_extension(fname);
    EXPECT_EQ(res.first, filename_t{fname_expected});
    EXPECT_EQ(res.second, filename_t{extension_expected});
  }

  {
    // valid stem and extension - directory
#if defined(_WIN32)
    filename_t fname = L"etc\\eng\\logfile.log";
    filename_t fname_expected = L"etc\\eng\\logfile";
    filename_t extension_expected = L".log";
#else
    filename_t fname = "etc/eng/logfile.log";
    filename_t fname_expected = "etc/eng/logfile";
    filename_t extension_expected = ".log";
#endif

    auto const res = extract_stem_and_extension(fname);
    EXPECT_EQ(res.first, filename_t{fname_expected});
    EXPECT_EQ(res.second, filename_t{extension_expected});
  }

  {
    // valid stem and extension - directory
#if defined(_WIN32)
    filename_t fname = L"\\etc\\eng\\logfile.log";
    filename_t fname_expected = L"\\etc\\eng\\logfile";
    filename_t extension_expected = L".log";
#else
    filename_t fname = "/etc/eng/logfile.log";
    filename_t fname_expected = "/etc/eng/logfile";
    filename_t extension_expected = ".log";
#endif

    auto const res = extract_stem_and_extension(fname);
    EXPECT_EQ(res.first, filename_t{fname_expected});
    EXPECT_EQ(res.second, filename_t{extension_expected});
  }
}

/***/
TEST(FileUtilities, append_date_to_filename)
{
  std::chrono::system_clock::time_point ts =
    std::chrono::system_clock::time_point{std::chrono::seconds{1583376945}};
  filename_t expected_fname = QUILL_FILENAME_STR("logfile_2020-03-05.log");
  filename_t base_fname = QUILL_FILENAME_STR("logfile.log");
  EXPECT_EQ(append_date_to_filename(base_fname, ts), expected_fname);
}

/***/
TEST(FileUtilities, append_index_to_filename)
{
  {
    // index is 0
    filename_t expected_fname = QUILL_FILENAME_STR("logfile.log");
    filename_t base_fname = QUILL_FILENAME_STR("logfile.log");
    EXPECT_EQ(append_index_to_filename(base_fname, 0), expected_fname);
  }

  {
    // index is non zero
    filename_t expected_fname = QUILL_FILENAME_STR("logfile.1.log");
    filename_t base_fname = QUILL_FILENAME_STR("logfile.log");
    EXPECT_EQ(append_index_to_filename(base_fname, 1), expected_fname);
  }
}