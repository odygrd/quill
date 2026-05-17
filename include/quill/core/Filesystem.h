/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"

#if !defined(QUILL_HAS_FILESYSTEM) && !defined(QUILL_HAS_EXPERIMENTAL_FILESYSTEM)
  #if defined(__cpp_lib_filesystem)
    #define QUILL_HAS_FILESYSTEM 1
  #elif defined(__cpp_lib_experimental_filesystem)
    #define QUILL_HAS_EXPERIMENTAL_FILESYSTEM 1
  #elif !defined(__has_include)
    #define QUILL_HAS_EXPERIMENTAL_FILESYSTEM 1
  #elif __has_include(<filesystem>)
    #define QUILL_HAS_FILESYSTEM 1
  #elif __has_include(<experimental/filesystem>)
    #define QUILL_HAS_EXPERIMENTAL_FILESYSTEM 1
  #endif

  // std::filesystem does not work on MinGW GCC 8: https://sourceforge.net/p/mingw-w64/bugs/737/
  #if defined(__MINGW32__) && defined(__GNUC__) && __GNUC__ == 8
    #undef QUILL_HAS_FILESYSTEM
    #undef QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  #endif

  // no filesystem support before GCC 8: https://en.cppreference.com/w/cpp/compiler_support
  #if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
    #undef QUILL_HAS_FILESYSTEM
  // #undef QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  #endif

  // no filesystem support before Clang 7: https://en.cppreference.com/w/cpp/compiler_support
  #if defined(__clang_major__) && __clang_major__ < 7
    #undef QUILL_HAS_FILESYSTEM
    #undef QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  #endif
#endif

#ifndef QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  #define QUILL_HAS_EXPERIMENTAL_FILESYSTEM 0
#endif

#ifndef QUILL_HAS_FILESYSTEM
  #define QUILL_HAS_FILESYSTEM 0
#endif

#if QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  #include <experimental/filesystem>
  #include <system_error>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

namespace fs = std::experimental::filesystem;

QUILL_END_EXPORT

QUILL_END_NAMESPACE

#elif QUILL_HAS_FILESYSTEM
  #include <filesystem>
  #include <system_error>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

namespace fs = std::filesystem;

QUILL_END_EXPORT

QUILL_END_NAMESPACE
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
QUILL_NODISCARD inline fs::path normalize_file_sink_path(fs::path filename, bool create_directories = true)
{
  if ((filename == "stdout") || (filename == "stderr") || (filename == "/dev/null"))
  {
    return filename;
  }

  std::error_code ec;
  fs::path parent_path;

  if (!filename.parent_path().empty())
  {
    parent_path = filename.parent_path();

    // The call to fs::status is necessary due to a known issue in GCC versions 8.3.0 to 9.4.0.
    // In these versions, fs::create_directories(path, ec) internally uses
    // fs::symlink_status(path, ec) instead of fs::status(path, ec) for checking the path.
    // This causes a problem because fs::symlink_status does not follow the symlink to the
    // target directory. As a result, it fails the is_directory() check but still indicates
    // that the path exists, leading to a not_a_directory exception being set in the error code.
    auto const st = fs::status(parent_path, ec);
    if (!is_directory(st))
    {
      if (!create_directories)
      {
        QUILL_THROW(QuillError{std::string{"cannot create canonical path for path "} +
                               parent_path.string() + std::string{" - error: directory does not exist"}});
      }

      fs::create_directories(parent_path, ec);
      if (ec)
      {
        QUILL_THROW(QuillError{std::string{"cannot create directories for path "} +
                               parent_path.string() + std::string{" - error: "} + ec.message()});
      }
    }
  }
  else
  {
    // Use the non-throwing overload: when the working directory has been deleted or is
    // inaccessible, the throwing overload raises fs::filesystem_error, which callers that treat
    // normalization as best-effort (e.g. SinkManager::get_sink) only guard against as QuillError.
    parent_path = fs::current_path(ec);

    if (ec)
    {
      QUILL_THROW(QuillError{std::string{"cannot resolve the current working directory for path "} +
                             filename.string() + std::string{" - error: "} + ec.message()});
    }
  }

  fs::path const canonical_path = fs::canonical(parent_path, ec);

  if (ec)
  {
    // fs::canonical can fail on certain filesystems (e.g. Windows RAM disks) where
    // GetFinalPathNameByHandleW is not supported. Fall back to fs::absolute with
    // lexically_normal to resolve . and .. portably (fs::absolute alone does not
    // normalize on POSIX).
    ec.clear();
    fs::path const absolute_path = fs::absolute(parent_path, ec);

    if (ec)
    {
      QUILL_THROW(QuillError{std::string{"cannot resolve path "} + parent_path.string() +
                             std::string{" - error: "} + ec.message()});
    }

    return absolute_path.lexically_normal() / filename.filename();
  }

  return canonical_path / filename.filename();
}
} // namespace detail

QUILL_END_NAMESPACE
