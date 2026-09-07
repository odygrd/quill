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

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

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
      : sink_id(static_cast<std::string&&>(sid)), sink_name(static_cast<std::string&&>(name)),
        sink_ptr(static_cast<std::weak_ptr<Sink>&&>(sptr)), file_sink_id(id_function) {};

    std::string sink_id;
    std::string sink_name;
    std::weak_ptr<Sink> sink_ptr;
    FileSinkIdFunction file_sink_id{nullptr};
  };

public:
  SinkManager(SinkManager const&) = delete;
  SinkManager& operator=(SinkManager const&) = delete;

  /***/
  QUILL_EXPORT static SinkManager& instance() noexcept
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

    // Normalize before taking the lock to avoid blocking filesystem calls under the spinlock
    std::string normalized_sink_name;
    QUILL_TRY
    {
      normalized_sink_name = detail::normalize_file_sink_path(fs::path{sink_name}, false).string();
      if (file_sink_id)
      {
        normalized_sink_name = file_sink_id(normalized_sink_name);
      }
    }
    QUILL_CATCH(QuillError const&)
    {
      // Fallback normalization is best-effort for file-path lookups only.
    }

    // The sinks are used by the backend thread, so after their creation we want to avoid mutating their member variables.
    LockGuard const lock{_spinlock};

    std::shared_ptr<Sink> sink = _find_sink(sink_name);

    if (!sink && !normalized_sink_name.empty() && normalized_sink_name != sink_name)
    {
      sink = _find_sink(normalized_sink_name);
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
  std::shared_ptr<Sink> create_sink(std::string const& sink_name, Args&&... args)
  {
    static_assert(std::is_base_of_v<Sink, TSink>, "TSink must derive from Sink");

    std::string const normalized_name = _normalized_sink_name<TSink>(sink_name);
    std::string const sink_id = _sink_id<TSink>(normalized_name);

    (void)_reserve_sink_id_for_creation(sink_name, sink_id, false);
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
  std::shared_ptr<Sink> create_or_get_sink(std::string const& sink_name, Args&&... args)
  {
    static_assert(std::is_base_of_v<Sink, TSink>, "TSink must derive from Sink");

    std::string const normalized_name = _normalized_sink_name<TSink>(sink_name);
    std::string const sink_id = _sink_id<TSink>(normalized_name);

    std::shared_ptr<Sink> sink = _reserve_sink_id_for_creation(sink_name, sink_id, true);

#if QUILL_USE_RTTI
    if (QUILL_UNLIKELY(sink && (dynamic_cast<TSink*>(sink.get()) == nullptr)))
    {
      // Callers cast the returned sink to the requested type, so returning a sink of an
      // incompatible type would be undefined behaviour
      QUILL_THROW(QuillError{"A sink named \"" + sink_name + "\" already exists with an incompatible type"});
    }
#endif

    return sink ? sink : _create_reserved_sink<TSink>(sink_id, normalized_name, static_cast<Args&&>(args)...);
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

  QUILL_NODISCARD std::shared_ptr<Sink> _reserve_sink_id_for_creation(std::string const& sink_name,
                                                                      std::string const& sink_id, bool return_existing)
  {
    while (true)
    {
      {
        LockGuard const lock{_spinlock};

        std::shared_ptr<Sink> sink = _find_sink(sink_id);
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
        if (_try_mark_sink_pending(sink_id))
        {
          return nullptr;
        }
      }

      detail::sleep_for_ns(100);
    }
  }

  template <typename TSink, typename... Args>
  std::shared_ptr<Sink> _create_reserved_sink(std::string const& sink_id, std::string const& sink_name, Args&&... args)
  {
    std::shared_ptr<Sink> sink;
    QUILL_TRY { sink = _create_sink_instance<TSink>(sink_name, static_cast<Args&&>(args)...); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      _remove_pending_sink(sink_id);
      throw;
    }
#endif

    _publish_created_sink(sink_id, sink_name, sink, _file_sink_id_function<TSink>());
    return sink;
  }

  template <typename TSink, typename... Args>
  static std::shared_ptr<Sink> _create_sink_instance(std::string const& sink_id, Args&&... args)
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

  void _publish_created_sink(std::string const& sink_id, std::string const& sink_name,
                             std::shared_ptr<Sink> const& sink, FileSinkIdFunction file_sink_id)
  {
    LockGuard const lock{_spinlock};

    QUILL_TRY
    {
      _insert_sink(sink_id, sink_name, sink, file_sink_id);
      _erase_pending_sink(sink_id);
    }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      _erase_pending_sink(sink_id);
      throw;
    }
#endif
  }

  void _remove_pending_sink(std::string const& sink_id) noexcept
  {
    LockGuard const lock{_spinlock};
    _erase_pending_sink(sink_id);
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

  QUILL_NODISCARD bool _try_mark_sink_pending(std::string const& sink_name)
  {
    auto search_it = std::lower_bound(_pending_sink_ids.begin(), _pending_sink_ids.end(), sink_name);

    if (search_it != _pending_sink_ids.end() && *search_it == sink_name)
    {
      return false;
    }

    _pending_sink_ids.insert(search_it, sink_name);
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
  QUILL_NODISCARD std::shared_ptr<Sink> _find_sink(std::string const& target) const noexcept
  {
    std::shared_ptr<Sink> sink;

    auto search_it =
      std::lower_bound(_sinks.begin(), _sinks.end(), target,
                       [](SinkInfo const& elem, std::string const& b) { return elem.sink_id < b; });

    if (search_it != std::end(_sinks) && search_it->sink_id == target)
    {
      sink = search_it->sink_ptr.lock();
    }

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
