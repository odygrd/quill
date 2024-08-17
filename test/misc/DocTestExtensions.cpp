#include "DocTestExtensions.h"

#include <memory>

#ifndef _WIN32
  #include <cstdlib>
  #include <unistd.h>
#else
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <windows.h>

  #include <codecvt>
  #include <io.h>
  #include <locale>
  #include <sys/stat.h>
#endif

namespace quill
{
namespace testing
{

// The ctor redirects the stream to a temporary file.
CapturedStream::CapturedStream(int fd) : fd_(fd), uncaptured_fd_(dup(fd))
{
#if defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(_WIN32)
  char temp_dir_path[MAX_PATH + 1] = {'\0'};  // NOLINT
  char temp_file_path[MAX_PATH + 1] = {'\0'}; // NOLINT

  ::GetTempPathA(sizeof(temp_dir_path), temp_dir_path);
  const UINT success = ::GetTempFileNameA(temp_dir_path, "dtest_redir",
                                          0, // Generate unique file name.
                                          temp_file_path);
  if (success == 0)
  {
    FAIL("Unable to create a temporary file in " << temp_dir_path;);
  }

  const int captured_fd = creat(temp_file_path, _S_IREAD | _S_IWRITE);
  if (captured_fd == -1)
  {
    FAIL("Unable to open_file temporary file " << temp_file_path);
  }
  filename_ = temp_file_path;
#else
  // There's no guarantee that a test has write access to the current
  // directory, so we create the temporary file in the /tmp directory
  // instead.
  // We use /tmp on most systems, and /sdcard on Android.
  // That's because Android doesn't have /tmp.

  #if defined(__ANDROID__)
  char name_template[] = "/data/local/tmp/doctest_captured_stream.XXXXXX";
  #else
  char name_template[] = "/tmp/captured_stream.XXXXXX";
  #endif // __ANDROID__

  const int captured_fd = mkstemp(name_template);

  if (captured_fd == -1)
  {
    FAIL("Failed to create tmp file "
         << name_template << " for test; does the test have access to the /tmp directory?");
  }

  filename_ = name_template;
#endif   // _WIN32

  fflush(nullptr);
  dup2(captured_fd, fd_);
  close(captured_fd);
}

CapturedStream::~CapturedStream() { remove(filename_.c_str()); }

std::string CapturedStream::GetCapturedString()
{
  if (uncaptured_fd_ != -1)
  {
    // Restores the original stream.
    fflush(nullptr);
    dup2(uncaptured_fd_, fd_);
    close(uncaptured_fd_);
    uncaptured_fd_ = -1;
  }

  FILE* const file = _fopen(filename_.c_str(), "r");
  if (file == nullptr)
  {
    // FAIL("Failed to open_file tmp file " << filename_ << " for capturing stream.");
  }

  std::string content = _read_entire_file(file);
  _fclose(file);
  return content;
}

std::string CapturedStream::_read_entire_file(FILE* file)
{
  size_t const file_size = _get_file_size(file);
  std::unique_ptr<char[]> buffer{new char[file_size]};

  size_t bytes_last_read = 0; // # of bytes read in the last fread()
  size_t bytes_read = 0;      // # of bytes read so far

  fseek(file, 0, SEEK_SET);

  do
  {
    // Keeps reading the file until we cannot read further or the
    // pre-determined file size is reached.
    bytes_last_read = fread(buffer.get() + bytes_read, 1, file_size - bytes_read, file);
    bytes_read += bytes_last_read;
  } while (bytes_last_read > 0 && bytes_read < file_size);

  std::string content{buffer.get(), bytes_read};
  return content;
}

size_t CapturedStream::_get_file_size(FILE* file)
{
  fseek(file, 0, SEEK_END);
  return static_cast<size_t>(ftell(file));
}

FILE* CapturedStream::_fopen(char const* path, char const* mode)
{
#if defined(_WIN32)
  struct wchar_codecvt : public std::codecvt<wchar_t, char, std::mbstate_t>
  {
  };
  std::wstring_convert<wchar_codecvt> converter;
  std::wstring wide_path = converter.from_bytes(path);
  std::wstring wide_mode = converter.from_bytes(mode);
  return _wfopen(wide_path.c_str(), wide_mode.c_str());
#else
  return fopen(path, mode);
#endif // _WIN32
}

int CapturedStream::_fclose(FILE* fp) { return fclose(fp); }

// Starts capturing an output stream (stdout/stderr).
void CaptureStream(int fd, const char* stream_name, CapturedStream** stream)
{
  if (*stream != nullptr)
  {
    FAIL("Only one " << stream_name << " capturer can exist at a time.");
  }
  *stream = new CapturedStream(fd);
}

// Stops capturing the output stream and returns the captured string.
std::string GetCapturedStream(CapturedStream** captured_stream)
{
  std::string content = (*captured_stream)->GetCapturedString();

  delete *captured_stream;
  *captured_stream = nullptr;

  return content;
}

#if defined(_MSC_VER) || defined(__BORLANDC__)
// MSVC and C++Builder do not provide a definition of STDERR_FILENO.
const int kStdOutFileno = 1;
const int kStdErrFileno = 2;
#else
const int kStdOutFileno = STDOUT_FILENO;
const int kStdErrFileno = STDERR_FILENO;
#endif // _MSC_VER

void CaptureStdout() { CaptureStream(kStdOutFileno, "stdout", &g_captured_stdout); }

void CaptureStderr() { CaptureStream(kStdErrFileno, "stderr", &g_captured_stderr); }

std::string GetCapturedStdout() { return GetCapturedStream(&g_captured_stdout); }

std::string GetCapturedStderr() { return GetCapturedStream(&g_captured_stderr); }

} // namespace testing
} // namespace quill