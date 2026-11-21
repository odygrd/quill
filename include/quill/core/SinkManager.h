/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Filesystem.h"
#include "quill/core/QuillError.h"
#include "quill/core/Spinlock.h"
#include "quill/core/ThreadPrimitives.h"

#ifndef QUILL_MODULE
  #include <algorithm>
  #include <cstdint>
  #include <memory>
  #include <string>
  #include <type_traits>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/** Forward declarations **/
class FileSink;
class Sink;

QUILL_END_EXPORT

namespace detail
{
class SinkManager
{
private:
  using FileSinkIdFunction = std::string (*)(fs::path const&);

  // Windows file keys are UTF-8. Opaque sink names may use any encoding.
  struct SinkId
  {
    bool operator<(SinkId const& other) const noexcept
    {
      return (is_windows_file == other.is_windows_file) ? (value < other.value)
                                                        : (is_windows_file < other.is_windows_file);
    }

    bool operator==(SinkId const& other) const noexcept
    {
      return (is_windows_file == other.is_windows_file) && (value == other.value);
    }

    bool operator!=(SinkId const& other) const noexcept { return !(*this == other); }

    std::string value;
    bool is_windows_file{false};
  };

  struct SinkInfo
  {
    SinkInfo(SinkId sid, std::string name, std::weak_ptr<Sink> sptr, FileSinkIdFunction id_function)
      : sink_id(static_cast<SinkId&&>(sid)),
        sink_name(static_cast<std::string&&>(name)),
        sink_ptr(static_cast<std::weak_ptr<Sink>&&>(sptr)),
        file_sink_id(id_function)
    {
    }

    SinkId sink_id;
    std::string sink_name;
    std::weak_ptr<Sink> sink_ptr;
    FileSinkIdFunction file_sink_id{nullptr};
  };

public:
  SinkManager(SinkManager const&) = delete;
  SinkManager& operator=(SinkManager const&) = delete;

  /***/
  QUILL_EXPORT static inline SinkManager& instance() noexcept
  {
    static SinkManager instance;
    return instance;
  }

  /***/
  QUILL_NODISCARD std::shared_ptr<Sink> get_sink(std::string const& sink_name) const
  {
    // Keep a file sink alive while using its normalization code, including across DLL boundaries.
    std::shared_ptr<Sink> file_sink;
    FileSinkIdFunction file_sink_id{nullptr};
    {
      LockGuard const lock{_spinlock};

      std::shared_ptr<Sink> sink = _find_sink({sink_name, false}, sink_name);
      if (sink)
      {
        return sink;
      }

      for (auto const& entry : _sinks)
      {
        if (!entry.file_sink_id)
        {
          continue;
        }

        file_sink = entry.sink_ptr.lock();
        if (file_sink)
        {
          file_sink_id = entry.file_sink_id;
          break;
        }
      }
    }

    // Only a file sink can be registered under a different spelling of the requested name.
    // Normalize outside the lock to avoid blocking filesystem calls under the spinlock, and
    // without throwing: a name that is not a usable path simply does not match a file sink.
    std::shared_ptr<Sink> sink;
    if (file_sink_id)
    {
      std::string error;
      fs::path const requested_path{sink_name};
      if (_use_separate_file_ids() && requested_path.is_absolute())
      {
        // Preserve exact-name lookups of a Windows file sink while its path cannot be
        // normalized, e.g. because its directory is temporarily inaccessible
        std::string const name = detail::file_sink_path_string(requested_path);
        LockGuard const lock{_spinlock};
        sink = _find_sink_by_name(name, true);
        if (sink)
        {
          return sink;
        }
      }

      fs::path const path = detail::try_normalize_file_sink_path(requested_path, false, error);
      if (error.empty() && !path.empty())
      {
        SinkId const id{file_sink_id(path), _use_separate_file_ids()};
        LockGuard const lock{_spinlock};
        sink = id.is_windows_file ? _find_sink_by_id(id) : _find_sink(id, id.value);
      }
    }

    if (QUILL_UNLIKELY(!sink))
    {
      QUILL_THROW(QuillError{"Sink with name \"" + sink_name + "\" does not exist"});
    }

    return sink;
  }

  /**
   * @brief Creates a new sink with the given name.
   * @throws QuillError if a sink with the same name already exists.
   */
  template <typename TSink, typename... Args>
  std::shared_ptr<TSink> create_sink(std::string const& sink_name, Args&&... args)
  {
    static_assert(std::is_base_of_v<Sink, TSink>, "TSink must derive from Sink");

    fs::path const path = _normalized_file_path<TSink>(sink_name);
    SinkId const name_id =
      _make_sink_id<TSink>(path.empty() ? sink_name : detail::file_sink_path_string(path));
    bool id_only{false};
    SinkId const sink_id = _make_sink_id<TSink>(_sink_id<TSink>(path, name_id.value, id_only));

    (void)_reserve_sink_id_for_creation(sink_name, sink_id, name_id, id_only, false);
    return _create_reserved_sink<TSink>(sink_id, name_id, path, static_cast<Args&&>(args)...);
  }

  /**
   * @brief Creates a new sink or returns an existing one with the given name.
   * @note If a compatible sink with the specified name already exists, it is returned and the
   *       provided constructor arguments are ignored.
   * @throws QuillError if an incompatible sink exists and RTTI is enabled.
   * @warning Without RTTI the type mismatch cannot be diagnosed; callers must keep sink names and
   *          requested types compatible.
   */
  template <typename TSink, typename... Args>
  std::shared_ptr<TSink> create_or_get_sink(std::string const& sink_name, Args&&... args)
  {
    static_assert(std::is_base_of_v<Sink, TSink>, "TSink must derive from Sink");

    fs::path const path = _normalized_file_path<TSink>(sink_name);
    SinkId const name_id =
      _make_sink_id<TSink>(path.empty() ? sink_name : detail::file_sink_path_string(path));
    bool id_only{false};
    SinkId const sink_id = _make_sink_id<TSink>(_sink_id<TSink>(path, name_id.value, id_only));

    std::shared_ptr<Sink> sink = _reserve_sink_id_for_creation(sink_name, sink_id, name_id, id_only, true);

#if QUILL_USE_RTTI
    if (QUILL_UNLIKELY(sink && (dynamic_cast<TSink*>(sink.get()) == nullptr)))
    {
      // Callers cast the returned sink to the requested type, so returning a sink of an
      // incompatible type would be undefined behaviour
      QUILL_THROW(QuillError{"A sink named \"" + sink_name + "\" already exists with an incompatible type"});
    }
#endif

    if (sink)
    {
      return _downcast_sink<TSink>(static_cast<std::shared_ptr<Sink>&&>(sink));
    }

    return _create_reserved_sink<TSink>(sink_id, name_id, path, static_cast<Args&&>(args)...);
  }

  /***/
  uint32_t cleanup_unused_sinks()
  {
    // this needs to take a lock each time. The backend logging thread should be carefully call
    // it only when needed
    LockGuard const lock{_spinlock};

    uint32_t cnt{0};
    for (auto it = _sinks.begin(); it != _sinks.end();)
    {
      if (it->sink_ptr.expired())
      {
        it = _sinks.erase(it);
        ++cnt;
      }
      else
      {
        ++it;
      }
    }

    return cnt;
  }

private:
  static constexpr bool _use_separate_file_ids() noexcept
  {
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
  }

  template <typename TSink>
  static SinkId _make_sink_id(std::string id)
  {
    return {static_cast<std::string&&>(id),
            _use_separate_file_ids() && _file_sink_id_function<TSink>() != nullptr};
  }

  template <typename TSink>
  static std::string _sink_id(fs::path const& path, std::string const& normalized_name)
  {
    auto const file_sink_id = _file_sink_id_function<TSink>();
    return file_sink_id ? file_sink_id(path) : normalized_name;
  }

  template <typename TSink, typename TFileSink = FileSink>
  static FileSinkIdFunction _file_sink_id_function()
  {
    if constexpr (std::disjunction_v<std::is_same<FileSink, TSink>, std::is_base_of<FileSink, TSink>>)
    {
#if defined(_WIN32)
      return &TFileSink::_file_sink_id;
#else
      return &_canonical_file_sink_id;
#endif
    }
    else
    {
      return nullptr;
    }
  }

  static std::string _canonical_file_sink_id(fs::path const& path)
  {
    // Resolve existing file aliases for lookup without changing the configured I/O path.
    if (!path.is_absolute())
    {
      return path.string();
    }

    std::error_code ec;
    fs::path const canonical_path = fs::canonical(path, ec);
    return ec ? path.string() : canonical_path.string();
  }

  /**
   * The id of a sink, and whether it was resolved from an existing file. Both come from the same
   * filesystem lookup: a separate existence check could see a file another caller created after
   * this id was computed for the missing file, and then miss that caller's sink.
   * On Windows the id does not depend on whether the file exists, so it is never resolved.
   */
  template <typename TSink>
  static std::string _sink_id(fs::path const& path, std::string const& normalized_name, bool& resolved)
  {
    resolved = false;

#if !defined(_WIN32)
    if constexpr (std::disjunction_v<std::is_same<FileSink, TSink>, std::is_base_of<FileSink, TSink>>)
    {
      if (path.is_absolute())
      {
        std::error_code ec;
        fs::path const canonical_path = fs::canonical(path, ec);
        if (!ec)
        {
          resolved = true;
          return canonical_path.string();
        }
      }

      return normalized_name;
    }
#endif

    return _sink_id<TSink>(path, normalized_name);
  }

  template <typename TSink>
  static fs::path _normalized_file_path(std::string const& sink_name)
  {
    if constexpr (std::disjunction_v<std::is_same<FileSink, TSink>, std::is_base_of<FileSink, TSink>>)
    {
      return detail::normalize_file_sink_path(fs::path{sink_name});
    }
    else
    {
      return {};
    }
  }

  SinkManager() = default;
  ~SinkManager() = default;

  /**
   * A file sink id depends on whether its file exists: a symlink resolves to its target only once
   * the target is created. Reserve both the id and the configured name, so the same path cannot
   * get a second sink while the first is constructed, and publish under the id recomputed after
   * the file is opened.
   */
  QUILL_NODISCARD std::shared_ptr<Sink> _reserve_sink_id_for_creation(std::string const& sink_name,
                                                                      SinkId const& sink_id, SinkId const& name_id,
                                                                      bool id_only, bool return_existing)
  {
    while (true)
    {
      QUILL_TRY
      {
        LockGuard const lock{_spinlock};

        // An existing file is found by its id only: the configured name may belong to the sink of
        // another file, e.g. one a symlink pointed to before it was retargeted or replaced. A path
        // that does not resolve yet also matches the configured name of a sink created through it.
        std::shared_ptr<Sink> sink =
          id_only ? _find_sink_by_id(sink_id) : _find_sink(sink_id, name_id.value);

        if (sink)
        {
          if (return_existing)
          {
            return sink;
          }

          QUILL_THROW(
            QuillError{"Sink with name \"" + sink_name +
                       "\" already exists. "
                       "Use create_or_get_sink() if you want to retrieve the existing sink, "
                       "or choose a different name."});
        }

        // Reserve the id, then construct outside the lock because file sinks can run user callbacks.
        if (_try_mark_sink_pending(sink_id, name_id))
        {
          return nullptr;
        }
      }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH_ALL()
      {
        // Release the lock before an uncaught rethrow can invoke terminate().
        throw;
      }
#endif

      detail::sleep_for_ns(100);
    }
  }

  /** True when Sink can be downcast to TSink with static_cast, i.e. not through a virtual base */
  template <typename TSink, typename = void>
  struct is_static_downcastable : std::false_type
  {
  };

  template <typename TSink>
  struct is_static_downcastable<TSink, std::void_t<decltype(static_cast<TSink*>(std::declval<Sink*>()))>>
    : std::true_type
  {
  };

  /** Returns an existing sink as TSink. Its type was checked by the caller when RTTI is available */
  template <typename TSink>
  QUILL_NODISCARD static std::shared_ptr<TSink> _downcast_sink(std::shared_ptr<Sink> sink)
  {
    if constexpr (is_static_downcastable<TSink>::value)
    {
      return std::static_pointer_cast<TSink>(static_cast<std::shared_ptr<Sink>&&>(sink));
    }
    else
    {
#if QUILL_USE_RTTI
      // A sink that inherits Sink virtually can only be downcast with dynamic_cast
      return std::dynamic_pointer_cast<TSink>(static_cast<std::shared_ptr<Sink>&&>(sink));
#else
      static_assert(
        is_static_downcastable<TSink>::value,
        "create_or_get_sink() needs RTTI for a sink that inherits quill::Sink virtually");
      return nullptr;
#endif
    }
  }

  template <typename TSink, typename... Args>
  std::shared_ptr<TSink> _create_reserved_sink(SinkId const& reserved_id, SinkId const& name_id,
                                               fs::path const& path, Args&&... args)
  {
    std::shared_ptr<TSink> sink;
    SinkId published_id;
    QUILL_TRY
    {
      sink = _create_sink_instance<TSink>(path, static_cast<Args&&>(args)...);

      // Opening the file may have created a symlink's target, so recompute the id now.
      published_id = _make_sink_id<TSink>(_sink_id<TSink>(path, name_id.value));
    }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      _remove_pending_sink(reserved_id, name_id);
      throw;
    }
#endif

    _publish_created_sink(reserved_id, published_id, name_id, sink, _file_sink_id_function<TSink>());
    return sink;
  }

  template <typename TSink, typename... Args>
  static std::shared_ptr<TSink> _create_sink_instance(QUILL_MAYBE_UNUSED fs::path const& path, Args&&... args)
  {
    if constexpr (std::disjunction_v<std::is_same<FileSink, TSink>, std::is_base_of<FileSink, TSink>>)
    {
      if constexpr (std::is_constructible_v<TSink, fs::path const&, Args...>)
      {
        return std::make_shared<TSink>(path, static_cast<Args&&>(args)...);
      }
      else
      {
        // Preserve support for custom file sinks with narrow-string constructors. Pass an lvalue,
        // since such a constructor may take std::string const& and delete the rvalue overload.
        std::string const name = path.string();
        return std::make_shared<TSink>(name, static_cast<Args&&>(args)...);
      }
    }
    else
    {
      return std::make_shared<TSink>(static_cast<Args&&>(args)...);
    }
  }

  void _publish_created_sink(SinkId const& reserved_id, SinkId const& sink_id, SinkId const& name_id,
                             std::shared_ptr<Sink> const& sink, FileSinkIdFunction file_sink_id)
  {
    QUILL_TRY
    {
      LockGuard const lock{_spinlock};
      _insert_sink(sink_id, name_id.value, sink, file_sink_id);
      _erase_pending_sink(reserved_id);
      _erase_pending_sink(name_id);
    }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      _remove_pending_sink(reserved_id, name_id);
      throw;
    }
#endif
  }

  void _remove_pending_sink(SinkId const& reserved_id, SinkId const& name_id) noexcept
  {
    LockGuard const lock{_spinlock};
    _erase_pending_sink(reserved_id);
    _erase_pending_sink(name_id);
  }

  /***/
  void _insert_sink(SinkId const& sink_id, std::string const& sink_name,
                    std::shared_ptr<Sink> const& sink, FileSinkIdFunction file_sink_id)
  {
    auto search_it =
      std::lower_bound(_sinks.begin(), _sinks.end(), sink_id,
                       [](SinkInfo const& elem, SinkId const& b) { return elem.sink_id < b; });

    if (search_it != _sinks.end() && search_it->sink_id == sink_id && search_it->sink_ptr.expired())
    {
      search_it->sink_ptr = sink;
      search_it->sink_name = sink_name;
      search_it->file_sink_id = file_sink_id;
      return;
    }

    _sinks.insert(search_it, SinkInfo{sink_id, sink_name, sink, file_sink_id});
  }

  QUILL_NODISCARD bool _is_sink_pending(SinkId const& key) const noexcept
  {
    auto const search_it = std::lower_bound(_pending_sink_ids.begin(), _pending_sink_ids.end(), key);
    return (search_it != _pending_sink_ids.end()) && (*search_it == key);
  }

  QUILL_NODISCARD bool _try_mark_sink_pending(SinkId const& sink_id, SinkId const& name_id)
  {
    if (_is_sink_pending(sink_id) || _is_sink_pending(name_id))
    {
      return false;
    }

    _pending_sink_ids.insert(
      std::lower_bound(_pending_sink_ids.begin(), _pending_sink_ids.end(), sink_id), sink_id);

    if (name_id != sink_id)
    {
      QUILL_TRY
      {
        _pending_sink_ids.insert(
          std::lower_bound(_pending_sink_ids.begin(), _pending_sink_ids.end(), name_id), name_id);
      }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH_ALL()
      {
        _erase_pending_sink(sink_id);
        throw;
      }
#endif
    }

    return true;
  }

  void _erase_pending_sink(SinkId const& id) noexcept
  {
    auto search_it = std::lower_bound(_pending_sink_ids.begin(), _pending_sink_ids.end(), id);

    if (search_it != _pending_sink_ids.end() && *search_it == id)
    {
      _pending_sink_ids.erase(search_it);
    }
  }

  /***/
  QUILL_NODISCARD std::shared_ptr<Sink> _find_sink_by_id(SinkId const& sink_id) const noexcept
  {
    auto search_it =
      std::lower_bound(_sinks.begin(), _sinks.end(), sink_id,
                       [](SinkInfo const& elem, SinkId const& b) { return elem.sink_id < b; });

    if (search_it != std::end(_sinks) && search_it->sink_id == sink_id)
    {
      return search_it->sink_ptr.lock();
    }

    return nullptr;
  }

  /***/
  QUILL_NODISCARD std::shared_ptr<Sink> _find_sink(SinkId const& id, std::string const& name) const noexcept
  {
    std::shared_ptr<Sink> sink = _find_sink_by_id(id);

    return sink ? sink : _find_sink_by_name(name, id.is_windows_file);
  }

  QUILL_NODISCARD std::shared_ptr<Sink> _find_sink_by_name(std::string const& name, bool is_windows_file) const noexcept
  {
    for (auto const& entry : _sinks)
    {
      if (entry.sink_id.is_windows_file == is_windows_file && entry.sink_name == name)
      {
        if (auto sink = entry.sink_ptr.lock())
        {
          return sink;
        }
      }
    }

    return nullptr;
  }

private:
  std::vector<SinkInfo> _sinks;
  std::vector<SinkId> _pending_sink_ids;
  mutable Spinlock _spinlock;
};
} // namespace detail

QUILL_END_NAMESPACE
