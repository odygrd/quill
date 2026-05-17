/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Metric.h"
#include "quill/core/QuillError.h"
#include "quill/core/Spinlock.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

QUILL_BEGIN_NAMESPACE

namespace detail
{
class MetricManager
{
public:
  MetricManager(MetricManager const&) = delete;
  MetricManager& operator=(MetricManager const&) = delete;

  /***/
  QUILL_EXPORT static MetricManager& instance() noexcept
  {
    static MetricManager instance;
    return instance;
  }

  /**
   * Creates a new metric.
   *
   * `metric_key` must be unique. `metric_name` must be non-empty but does not need to be unique.
   * Throws if a metric with the same key already exists.
   */
  QUILL_NODISCARD MetricMetadata const* create_metric(std::string const& metric_key, std::string const& metric_name,
                                                      std::vector<MetricLabel> const& labels)
  {
    _validate_key_and_name(metric_key, metric_name);

    LockGuard const lock{_spinlock};

    if (_find_metric(metric_key))
    {
      QUILL_THROW(QuillError{"Metric with key \"" + metric_key +
                             "\" already exists. Use create_or_get_metric() if you want "
                             "to retrieve the existing metric metadata pointer."});
    }

    return _insert_metric(metric_key, metric_name, labels);
  }

  /**
   * Returns the metric with `metric_key`, creating it from the supplied arguments if it does not
   * yet exist.
   *
   * `metric_key` is the unique identifier. `metric_name` must be non-empty but is not required to
   * be unique.
   *
   * If a metric with the same `metric_key` already exists, the existing metadata is returned as-is
   * and `metric_name` and `labels` are ignored. Callers that need strict matching should use
   * `create_metric()` and handle the throw themselves.
   */
  QUILL_NODISCARD MetricMetadata const* create_or_get_metric(std::string const& metric_key,
                                                             std::string const& metric_name,
                                                             std::vector<MetricLabel> const& labels)
  {
    _validate_key_and_name(metric_key, metric_name);

    LockGuard const lock{_spinlock};

    MetricMetadata const* metric_metadata = _find_metric(metric_key);

    if (!metric_metadata)
    {
      metric_metadata = _insert_metric(metric_key, metric_name, labels);
    }

    return metric_metadata;
  }

  /**
   * Returns the metric with `metric_key`. Throws if no such metric exists.
   */
  QUILL_NODISCARD MetricMetadata const* get_metric(std::string const& metric_key)
  {
    LockGuard const lock{_spinlock};

    MetricMetadata const* metric_metadata = _find_metric(metric_key);

    if (!metric_metadata)
    {
      QUILL_THROW(QuillError{"Metric with key \"" + metric_key + "\" does not exist"});
    }

    return metric_metadata;
  }

private:
  MetricManager() = default;
  ~MetricManager() = default;

  /***/
  static void _validate_key_and_name(std::string const& metric_key, std::string const& metric_name)
  {
    if (metric_name.empty())
    {
      QUILL_THROW(QuillError{"Metric name must not be empty"});
    }

    if (metric_key.empty())
    {
      QUILL_THROW(QuillError{"Metric key must not be empty"});
    }
  }

  /***/
  QUILL_NODISCARD MetricMetadata const* _insert_metric(std::string const& metric_key, std::string const& metric_name,
                                                       std::vector<MetricLabel> const& labels)
  {
    auto insert_it = std::lower_bound(_metrics.begin(), _metrics.end(), metric_key,
                                      [](std::unique_ptr<MetricMetadata> const& elem, std::string const& target)
                                      { return elem->metric_key() < target; });

    insert_it = _metrics.insert(insert_it, std::make_unique<MetricMetadata>(metric_key, metric_name, labels));

    return insert_it->get();
  }

  /***/
  QUILL_NODISCARD MetricMetadata const* _find_metric(std::string const& metric_key) const noexcept
  {
    auto search_it = std::lower_bound(_metrics.begin(), _metrics.end(), metric_key,
                                      [](std::unique_ptr<MetricMetadata> const& elem, std::string const& target)
                                      { return elem->metric_key() < target; });

    if (search_it != _metrics.end() && (*search_it)->metric_key() == metric_key)
    {
      return search_it->get();
    }

    return nullptr;
  }

private:
  std::vector<std::unique_ptr<MetricMetadata>> _metrics;
  mutable Spinlock _spinlock;
};
} // namespace detail

QUILL_END_NAMESPACE
