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
  using FileSinkIdFunction = std::string (*)(std::string const&);

  struct SinkInfo
  {
    explicit SinkInfo() = default;
    SinkInfo(std::string sid, std::string name, std::weak_ptr<Sink> sptr, FileSinkIdFunction id_function)
      : sink_id(static_cast<std::string&&>(sid)),
        sink_name(static_cast<std::string&&>(name)),
        sink_ptr(static_cast<std::weak_ptr<Sink>&&>(sptr)),
        file_sink_id(id_function)
    {
    }

    std::string sink_id;
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

      std::shared_ptr<Sink> sink = _find_sink(sink_name);
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
      std::string normalized_sink_name =
        detail::try_normalize_file_sink_path(fs::path{sink_name}, false, error).string();

      if (error.empty() && !normalized_sink_name.empty())
      {
        normalized_sink_name = file_sink_id(normalized_sink_name);

        if (normalized_sink_name != sink_name)
        {
          LockGuard const lock{_spinlock};
          sink = _find_sink(normalized_sink_name);
        }
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

    std::string const normalized_name = _normalized_sink_name<TSink>(sink_name);
    bool id_only{false};
    std::string const sink_id = _sink_id<TSink>(normalized_name, id_only);

    (void)_reserve_sink_id_for_creation(sink_name, sink_id, normalized_name, id_only, false);
    return _create_reserved_sink<TSink>(sink_id, normalized_name, static_cast<Args&&>(args)...);
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

    std::string const normalized_name = _normalized_sink_name<TSink>(sink_name);
    bool id_only{false};
    std::string const sink_id = _sink_id<TSink>(normalized_name, id_only);

    std::shared_ptr<Sink> sink =
      _reserve_sink_id_for_creation(sink_name, sink_id, normalized_name, id_only, true);

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

    return _create_reserved_sink<TSink>(sink_id, normalized_name, static_cast<Args&&>(args)...);
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
  template <typename TSink>
  static std::string _sink_id(std::string const& normalized_name)
  {
    auto const file_sink_id = _file_sink_id_function<TSink>();
    return file_sink_id ? file_sink_id(normalized_name) : normalized_name;
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

  static std::string _canonical_file_sink_id(std::string const& normalized_name)
  {
    // Resolve existing file aliases for lookup without changing the configured I/O path.
    fs::path const path{normalized_name};
    if (!path.is_absolute())
    {
      return normalized_name;
    }

    std::error_code ec;
    fs::path const canonical_path = fs::canonical(path, ec);
    return ec ? normalized_name : canonical_path.string();
  }

  /**
   * The id of a sink, and whether it was resolved from an existing file. Both come from the same
   * filesystem lookup: a separate existence check could see a file another caller created after
   * this id was computed for the missing file, and then miss that caller's sink.
   * On Windows the id does not depend on whether the file exists, so it is never resolved.
   */
  template <typename TSink>
  static std::string _sink_id(std::string const& normalized_name, bool& resolved)
  {
    resolved = false;

#if !defined(_WIN32)
    if constexpr (std::disjunction_v<std::is_same<FileSink, TSink>, std::is_base_of<FileSink, TSink>>)
    {
      fs::path const path{normalized_name};
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

    return _sink_id<TSink>(normalized_name);
  }

  template <typename TSink>
  static std::string _normalized_sink_name(std::string const& sink_name)
  {
    if constexpr (std::disjunction_v<std::is_same<FileSink, TSink>, std::is_base_of<FileSink, TSink>>)
    {
      return detail::normalize_file_sink_path(fs::path{sink_name}).string();
    }
    else
    {
      return sink_name;
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
                                                                      std::string const& sink_id,
                                                                      std::string const& normalized_name,
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
        std::shared_ptr<Sink> sink = id_only ? _find_sink_by_id(sink_id) : _find_sink(sink_id);

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
        if (_try_mark_sink_pending(sink_id, normalized_name))
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
  std::shared_ptr<TSink> _create_reserved_sink(std::string const& reserved_id,
                                               std::string const& normalized_name, Args&&... args)
  {
    std::shared_ptr<TSink> sink;
    std::string published_id;
    QUILL_TRY
    {
      sink = _create_sink_instance<TSink>(normalized_name, static_cast<Args&&>(args)...);

      // Opening the file may have created a symlink's target, so recompute the id now.
      published_id = _sink_id<TSink>(normalized_name);
    }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      _remove_pending_sink(reserved_id, normalized_name);
      throw;
    }
#endif

    _publish_created_sink(reserved_id, published_id, normalized_name, sink, _file_sink_id_function<TSink>());
    return sink;
  }

  template <typename TSink, typename... Args>
  static std::shared_ptr<TSink> _create_sink_instance(std::string const& sink_id, Args&&... args)
  {
    if constexpr (std::disjunction_v<std::is_same<FileSink, TSink>, std::is_base_of<FileSink, TSink>>)
    {
      return std::make_shared<TSink>(sink_id, static_cast<Args&&>(args)...);
    }
    else
    {
      return std::make_shared<TSink>(static_cast<Args&&>(args)...);
    }
  }

  void _publish_created_sink(std::string const& reserved_id, std::string const& sink_id,
                             std::string const& normalized_name, std::shared_ptr<Sink> const& sink,
                             FileSinkIdFunction file_sink_id)
  {
    QUILL_TRY
    {
      LockGuard const lock{_spinlock};
      _insert_sink(sink_id, normalized_name, sink, file_sink_id);
      _erase_pending_sink(reserved_id);
      _erase_pending_sink(normalized_name);
    }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      _remove_pending_sink(reserved_id, normalized_name);
      throw;
    }
#endif
  }

  void _remove_pending_sink(std::string const& reserved_id, std::string const& normalized_name) noexcept
  {
    LockGuard const lock{_spinlock};
    _erase_pending_sink(reserved_id);
    _erase_pending_sink(normalized_name);
  }

  /***/
  void _insert_sink(std::string const& sink_name, std::string const& original_name,
                    std::shared_ptr<Sink> const& sink, FileSinkIdFunction file_sink_id)
  {
    auto search_it =
      std::lower_bound(_sinks.begin(), _sinks.end(), sink_name,
                       [](SinkInfo const& elem, std::string const& b) { return elem.sink_id < b; });

    if (search_it != _sinks.end() && search_it->sink_id == sink_name && search_it->sink_ptr.expired())
    {
      search_it->sink_ptr = sink;
      search_it->sink_name = original_name;
      search_it->file_sink_id = file_sink_id;
      return;
    }

    _sinks.insert(search_it, SinkInfo{sink_name, original_name, sink, file_sink_id});
  }

  QUILL_NODISCARD bool _is_sink_pending(std::string const& key) const noexcept
  {
    auto const search_it = std::lower_bound(_pending_sink_ids.begin(), _pending_sink_ids.end(), key);
    return (search_it != _pending_sink_ids.end()) && (*search_it == key);
  }

  QUILL_NODISCARD bool _try_mark_sink_pending(std::string const& sink_id, std::string const& normalized_name)
  {
    if (_is_sink_pending(sink_id) || _is_sink_pending(normalized_name))
    {
      return false;
    }

    _pending_sink_ids.insert(
      std::lower_bound(_pending_sink_ids.begin(), _pending_sink_ids.end(), sink_id), sink_id);

    if (normalized_name != sink_id)
    {
      QUILL_TRY
      {
        _pending_sink_ids.insert(
          std::lower_bound(_pending_sink_ids.begin(), _pending_sink_ids.end(), normalized_name), normalized_name);
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

  void _erase_pending_sink(std::string const& sink_name) noexcept
  {
    auto search_it = std::lower_bound(_pending_sink_ids.begin(), _pending_sink_ids.end(), sink_name);

    if (search_it != _pending_sink_ids.end() && *search_it == sink_name)
    {
      _pending_sink_ids.erase(search_it);
    }
  }

  /***/
  QUILL_NODISCARD std::shared_ptr<Sink> _find_sink_by_id(std::string const& sink_id) const noexcept
  {
    auto search_it =
      std::lower_bound(_sinks.begin(), _sinks.end(), sink_id,
                       [](SinkInfo const& elem, std::string const& b) { return elem.sink_id < b; });

    if (search_it != std::end(_sinks) && search_it->sink_id == sink_id)
    {
      return search_it->sink_ptr.lock();
    }

    return nullptr;
  }

  /***/
  QUILL_NODISCARD std::shared_ptr<Sink> _find_sink(std::string const& target) const noexcept
  {
    std::shared_ptr<Sink> sink = _find_sink_by_id(target);

    // Preserve exact-name lookups when filesystem normalization is temporarily unavailable.
    if (!sink)
    {
      for (auto const& entry : _sinks)
      {
        if (entry.sink_name == target)
        {
          return entry.sink_ptr.lock();
        }
      }
    }

    return sink;
  }

private:
  std::vector<SinkInfo> _sinks;
  std::vector<std::string> _pending_sink_ids;
  mutable Spinlock _spinlock;
};
} // namespace detail

QUILL_END_NAMESPACE
