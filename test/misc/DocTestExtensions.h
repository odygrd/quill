#pragma once

#include "doctest/doctest.h"
#include "quill/core/Attributes.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

#define REQUIRE_STREQ(str1, str2)                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (strcmp(str1, str2) != 0)                                                                   \
    {                                                                                              \
      MESSAGE("Expected equality of these values: \n\t str1: "                                     \
              << std::string_view{str1} << "\n\t str2: " << std::string_view{str2});               \
      REQUIRE_UNARY(false);                                                                        \
    };                                                                                             \
  } while (0)

#define REQUIRE_WSTREQ(str1, str2)                                                                 \
  do                                                                                               \
  {                                                                                                \
    if (wcscmp(str1, str2) != 0)                                                                   \
    {                                                                                              \
      MESSAGE("Expected equality of these values: \n\t str1: " << str1 << "\n\t str2: " << str2);  \
      REQUIRE_UNARY(false);                                                                        \
    };                                                                                             \
  } while (0)

#define REQUIRE_STRNEQ(str1, str2)                                                                    \
  do                                                                                                  \
  {                                                                                                   \
    if (strcmp(str1, str2) == 0)                                                                      \
    {                                                                                                 \
      MESSAGE("Expected non-equality of these values: \n\t str1: " << str1 << "\n\t str2: " << str2); \
      REQUIRE_UNARY(false);                                                                           \
    };                                                                                                \
  } while (0)

namespace quill
{
namespace testing
{
/**
 * Object that captures an output stream (stdout/stderr).
 */
class CapturedStream
{
public:
  // The ctor redirects the stream to a temporary file.
  explicit CapturedStream(int fd);

  ~CapturedStream();

  std::string GetCapturedString();

private:
  static std::string _read_entire_file(FILE* file);

  static size_t _get_file_size(FILE* file);

  static FILE* _fopen(char const* path, char const* mode);

  static int _fclose(FILE* fp);

private:
  int fd_; // A stream to capture.
  int uncaptured_fd_;
  std::string filename_; // Name of the temporary file holding the stderr output.
};

QUILL_MAYBE_UNUSED static CapturedStream* g_captured_stderr = nullptr;
QUILL_MAYBE_UNUSED static CapturedStream* g_captured_stdout = nullptr;

// Starts capturing an output stream (stdout/stderr).
void CaptureStream(int fd, const char* stream_name, CapturedStream** stream);

// Stops capturing the output stream and returns the captured string.
std::string GetCapturedStream(CapturedStream** captured_stream);

// Starts capturing stdout.
void CaptureStdout();

// Starts capturing stderr.
void CaptureStderr();

// Stops capturing stdout and returns the captured string.
std::string GetCapturedStdout();

// Stops capturing stderr and returns the captured string.
std::string GetCapturedStderr();
} // namespace testing
} // namespace quill