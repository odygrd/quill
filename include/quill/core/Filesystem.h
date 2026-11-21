/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"

#ifndef QUILL_MODULE
  #include <filesystem>
  #include <string>
  #include <system_error>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

namespace fs = std::filesystem;

QUILL_END_EXPORT

namespace detail
{
#if defined(_WIN32)
/** True for an extended-length path ("\\?\..."), which Win32 passes to the filesystem as is. */
QUILL_NODISCARD inline bool is_extended_windows_path(fs::path const& path) noexcept
{
  return path.native().compare(0, 4, L"\\\\?\\") == 0;
}

/** Spells an absolute drive or UNC path in extended-length form. */
QUILL_NODISCARD inline fs::path to_extended_windows_path(fs::path const& path)
{
  std::wstring const& native = path.native();
  return fs::path{(native.compare(0, 2, L"\\\\") == 0) ? L"\\\\?\\UNC\\" + native.substr(2) : L"\\\\?\\" + native};
}

/** Spells an extended-length drive or UNC path in ordinary form. Other paths are unchanged. */
QUILL_NODISCARD inline fs::path to_ordinary_windows_path(fs::path const& path)
{
  std::wstring const& native = path.native();
  if (native.compare(0, 8, L"\\\\?\\UNC\\") == 0)
  {
    return fs::path{L"\\\\" + native.substr(8)};
  }

  bool const is_extended_drive_path = is_extended_windows_path(path) && (native.size() > 6) &&
    (native[5] == L':') && (native[6] == L'\\');
  return is_extended_drive_path ? fs::path{native.substr(4)} : path;
}
#endif

/**
 * The final component of a file sink path as the OS opens it. Ordinary Win32 paths ignore
 * trailing periods and spaces on it, while extended-length paths address those names literally.
 */
QUILL_NODISCARD inline fs::path file_sink_leaf(fs::path const& filename)
{
#if defined(_WIN32)
  if (!is_extended_windows_path(filename))
  {
    std::wstring const name = filename.filename().native();
    size_t const last = name.find_last_not_of(L". ");
    if ((last != std::wstring::npos) && ((last + 1) < name.size()))
    {
      return fs::path{name.substr(0, last + 1)};
    }
  }
#endif

  return filename.filename();
}

/** Converts a path to text without narrowing through the Windows ANSI code page. */
QUILL_NODISCARD inline std::string file_sink_path_string(fs::path const& path)
{
#if defined(_WIN32)
  auto const utf8 = path.u8string();
  return std::string{reinterpret_cast<char const*>(utf8.data()), utf8.size()};
#else
  return path.string();
#endif
}

/**
 * Resolves a file sink path to an absolute path with a canonical parent directory.
 * On failure returns an empty path and stores the reason in `error` instead of throwing.
 */
QUILL_NODISCARD inline fs::path try_normalize_file_sink_path(fs::path const& filename,
                                                             bool create_directories, std::string& error)
{
  if ((filename == "stdout") || (filename == "stderr") || (filename == "/dev/null"))
  {
    return filename;
  }

  fs::path const leaf = file_sink_leaf(filename);
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
        error = "cannot create canonical path for path " + file_sink_path_string(parent_path) +
          " - error: directory does not exist";
        return fs::path{};
      }

      fs::create_directories(parent_path, ec);
      if (ec)
      {
        error = "cannot create directories for path " + file_sink_path_string(parent_path) +
          " - error: " + ec.message();
        return fs::path{};
      }
    }
  }
  else
  {
    // Use the non-throwing overload: the throwing one raises fs::filesystem_error when the
    // working directory has been deleted or is inaccessible.
    parent_path = fs::current_path(ec);

    if (ec)
    {
      error = "cannot resolve the current working directory for path " +
        file_sink_path_string(filename) + " - error: " + ec.message();
      return fs::path{};
    }
  }

  fs::path const canonical_path = fs::canonical(parent_path, ec);

  if (ec)
  {
#if defined(_WIN32)
    // Some Windows standard libraries treat the extended prefix as a relative path.
    // A valid extended input is already absolute and must keep its spelling.
    if (is_extended_windows_path(filename))
    {
      return filename;
    }
#endif

    // fs::canonical can fail on certain filesystems (e.g. Windows RAM disks) where
    // GetFinalPathNameByHandleW is not supported. Fall back to fs::absolute with
    // lexically_normal to resolve . and .. portably (fs::absolute alone does not
    // normalize on POSIX).
    ec.clear();
    fs::path const absolute_path = fs::absolute(parent_path, ec);

    if (ec)
    {
      error = "cannot resolve path " + file_sink_path_string(parent_path) + " - error: " + ec.message();
      return fs::path{};
    }

    return absolute_path.lexically_normal() / leaf;
  }

#if defined(_WIN32)
  // canonical() may remove the extended prefix even when native I/O still requires it.
  if (is_extended_windows_path(filename) && !is_extended_windows_path(canonical_path))
  {
    return to_extended_windows_path(canonical_path) / leaf;
  }
#endif

  return canonical_path / leaf;
}

/**
 * Resolves a file sink path to an absolute path with a canonical parent directory.
 * @throws QuillError if the path cannot be resolved
 */
QUILL_NODISCARD inline fs::path normalize_file_sink_path(fs::path const& filename, bool create_directories = true)
{
  std::string error;
  fs::path normalized = try_normalize_file_sink_path(filename, create_directories, error);

  if (QUILL_UNLIKELY(!error.empty()))
  {
    QUILL_THROW(QuillError{error});
  }

  return normalized;
}
} // namespace detail

QUILL_END_NAMESPACE
