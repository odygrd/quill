/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

namespace quill
{
/** Forward declarations **/
class FileSink;
class Sink;

namespace detail
{
class SinkManager
{
private:
  struct SinkInfo
  {
    explicit SinkInfo() = default;
    SinkInfo(std::string sid, std::weak_ptr<Sink> sptr)
      : sink_id(static_cast<std::string&&>(sid)), sink_ptr(static_cast<std::weak_ptr<Sink>&&>(sptr)){};

    std::string sink_id;
    std::weak_ptr<Sink> sink_ptr;
  };

public:
  SinkManager(SinkManager const&) = delete;
  SinkManager& operator=(SinkManager const&) = delete;

  /***/
  static SinkManager& instance() noexcept
  {
    static SinkManager instance;
    return instance;
  }

  /***/
  QUILL_NODISCARD std::shared_ptr<Sink> get_sink(std::string const& sink_name) const
  {
    // The sinks are used by the backend thread, so after their creation we want to avoid mutating their member variables.
    std::lock_guard<std::mutex> const lock{_mutex};

    std::shared_ptr<Sink> sink = _find_sink(sink_name);

    if (QUILL_UNLIKELY(!sink))
    {
      QUILL_THROW(QuillError{"Sink with name \"" + sink_name + "\" does not exist"});
    }

    return sink;
  }

  /***/
  template <typename TSink, typename... Args>
  std::shared_ptr<Sink> create_or_get_sink(std::string const& sink_name, Args&&... args)
  {
    // The sinks are used by the backend thread, so after their creation we want to avoid mutating their member variables.
    std::lock_guard<std::mutex> const lock{_mutex};

    std::shared_ptr<Sink> sink = _find_sink(sink_name);

    if (!sink)
    {
      if constexpr (std::disjunction_v<std::is_same<FileSink, TSink>, std::is_base_of<FileSink, TSink>>)
      {
        sink = std::make_shared<TSink>(sink_name, static_cast<Args&&>(args)...);
      }
      else
      {
        sink = std::make_shared<TSink>(static_cast<Args&&>(args)...);
      }

      _insert_sink(sink_name, sink);
    }

    return sink;
  }

  /***/
  uint32_t cleanup_unused_sinks()
  {
    // this needs to take a lock each time. The backend logging thread should be carefully call
    // it only when needed
    std::lock_guard<std::mutex> const lock{_mutex};
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
  SinkManager() = default;
  ~SinkManager() = default;

  /***/
  void _insert_sink(std::string const& sink_name, std::shared_ptr<Sink> const& sink)
  {
    auto search_it =
      std::lower_bound(_sinks.begin(), _sinks.end(), sink_name,
                       [](SinkInfo const& elem, std::string const& b) { return elem.sink_id < b; });

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
  mutable std::mutex _mutex;
};
} // namespace detail
} // namespace quill