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
  struct SinkInfo
  {
    explicit SinkInfo() = default;
    SinkInfo(std::string sid, std::weak_ptr<Sink> sptr)
      : sink_id(static_cast<std::string&&>(sid)), sink_ptr(static_cast<std::weak_ptr<Sink>&&>(sptr)) {};

    std::string sink_id;
    std::weak_ptr<Sink> sink_ptr;
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
    // Normalize before taking the lock to avoid blocking filesystem calls under the spinlock
    std::string normalized_sink_name;
    QUILL_TRY
    {
      normalized_sink_name = detail::normalize_file_sink_path(fs::path{sink_name}, false).string();
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

    std::string const sink_id = _normalized_sink_name<TSink>(sink_name);

    LockGuard const lock{_spinlock};

    std::shared_ptr<Sink> sink = _find_sink(sink_id);

    if (sink)
    {
      QUILL_THROW(QuillError{"Sink with name \"" + sink_name +
                             "\" already exists. "
                             "Use create_or_get_sink() if you want to retrieve the existing sink, "
                             "or choose a different name."});
    }

    if constexpr (std::disjunction_v<std::is_same<FileSink, TSink>, std::is_base_of<FileSink, TSink>>)
    {
      sink = std::make_shared<TSink>(sink_id, static_cast<Args&&>(args)...);
    }
    else
    {
      sink = std::make_shared<TSink>(static_cast<Args&&>(args)...);
    }

    _insert_sink(sink_id, sink);

    return sink;
  }

  /**
   * @brief Creates a new sink or returns an existing one with the given name.
   * @note If a sink with the specified name already exists, the existing sink is returned
   * and the provided constructor arguments are ignored.
   */
  template <typename TSink, typename... Args>
  std::shared_ptr<Sink> create_or_get_sink(std::string const& sink_name, Args&&... args)
  {
    static_assert(std::is_base_of_v<Sink, TSink>, "TSink must derive from Sink");

    std::string const sink_id = _normalized_sink_name<TSink>(sink_name);

    // The sinks are used by the backend thread, so after their creation we want to avoid mutating their member variables.
    LockGuard const lock{_spinlock};

    std::shared_ptr<Sink> sink = _find_sink(sink_id);

    if (!sink)
    {
      if constexpr (std::disjunction_v<std::is_same<FileSink, TSink>, std::is_base_of<FileSink, TSink>>)
      {
        sink = std::make_shared<TSink>(sink_id, static_cast<Args&&>(args)...);
      }
      else
      {
        sink = std::make_shared<TSink>(static_cast<Args&&>(args)...);
      }

      _insert_sink(sink_id, sink);
    }

    return sink;
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

  /***/
  void _insert_sink(std::string const& sink_name, std::shared_ptr<Sink> const& sink)
  {
    auto search_it =
      std::lower_bound(_sinks.begin(), _sinks.end(), sink_name,
                       [](SinkInfo const& elem, std::string const& b) { return elem.sink_id < b; });

    if (search_it != _sinks.end() && search_it->sink_id == sink_name && search_it->sink_ptr.expired())
    {
      search_it->sink_ptr = sink;
      return;
    }

    _sinks.insert(search_it, SinkInfo{sink_name, sink});
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

    return sink;
  }

private:
  std::vector<SinkInfo> _sinks;
  mutable Spinlock _spinlock;
};
} // namespace detail

QUILL_END_NAMESPACE
