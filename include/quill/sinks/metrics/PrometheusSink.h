/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Metric.h"
#include "quill/core/Spinlock.h"
#include "quill/sinks/Sink.h"

#include <prometheus/check_names.h>
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/family.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/labels.h>
#include <prometheus/metric_type.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>

#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/**
 * @brief Optional sink that forwards Quill metric samples to a prometheus-cpp registry.
 *
 * Only translation units including this header require prometheus-cpp headers and linking against
 * `prometheus-cpp::core` and `prometheus-cpp::pull`. Quill itself does not depend on
 * prometheus-cpp.
 */
class PrometheusSink final : public Sink
{
public:
  using Registry = prometheus::Registry;
  using Exposer = prometheus::Exposer;
  using Labels = prometheus::Labels;
  using HistogramBuckets = std::vector<double>;

  struct ExposerConfiguration
  {
    std::string bind_address{"127.0.0.1:8080"};
    size_t num_threads{1};
    std::vector<std::string> server_options;
    std::string uri{"/metrics"};
    CivetCallbacks const* callbacks{nullptr};
  };

  struct Options
  {
    Registry::InsertBehavior insert_behavior{Registry::InsertBehavior::Merge};
    std::optional<ExposerConfiguration> exposer;
  };

  struct SummaryQuantile
  {
    double quantile;
    double error;
  };

  using SummaryQuantiles = std::vector<SummaryQuantile>;

  enum class GaugeUpdateMode
  {
    Add,
    Set
  };

  explicit PrometheusSink(Registry::InsertBehavior insert_behavior = Registry::InsertBehavior::Merge)
    : _registry(insert_behavior)
  {
  }

  explicit PrometheusSink(ExposerConfiguration const& exposer_config,
                          Registry::InsertBehavior insert_behavior = Registry::InsertBehavior::Merge)
    : _registry(insert_behavior)
  {
    if (exposer_config.uri.empty())
    {
      QUILL_THROW(QuillError{"Prometheus exposer uri must not be empty"});
    }

    if (!exposer_config.server_options.empty())
    {
      _exposer = std::make_unique<Exposer>(exposer_config.server_options, exposer_config.callbacks);
    }
    else
    {
      if (exposer_config.bind_address.empty())
      {
        QUILL_THROW(QuillError{"Prometheus exposer bind_address must not be empty"});
      }

      _exposer = std::make_unique<Exposer>(exposer_config.bind_address, exposer_config.num_threads,
                                           exposer_config.callbacks);
    }

    if (std::optional<std::string> endpoint_bind_address = _resolve_scrape_bind_address(exposer_config))
    {
      if (_bind_address_uses_ephemeral_port(*endpoint_bind_address))
      {
        std::vector<int> const listening_ports = _exposer->GetListeningPorts();

        if (!listening_ports.empty() && listening_ports.front() > 0)
        {
          _replace_bind_address_port(*endpoint_bind_address, listening_ports.front());
        }
      }

      _exposer_scrape_endpoint = std::string{"http://"} + *endpoint_bind_address + exposer_config.uri;
    }

    _exposer->RegisterCollectable(_registry_collectable, exposer_config.uri);
  }

  QUILL_NODISCARD Registry& registry() noexcept { return _registry; }

  QUILL_NODISCARD Registry const& registry() const noexcept { return _registry; }

  QUILL_NODISCARD Exposer* exposer() noexcept { return _exposer.get(); }

  QUILL_NODISCARD Exposer const* exposer() const noexcept { return _exposer.get(); }

  QUILL_NODISCARD std::string const& exposer_scrape_endpoint() const noexcept
  {
    return _exposer_scrape_endpoint;
  }

  QUILL_NODISCARD std::vector<int> exposer_listening_ports() const
  {
    if (_exposer == nullptr)
    {
      return {};
    }

    return _exposer->GetListeningPorts();
  }

  QUILL_NODISCARD bool has_metric(std::string const& metric_key) const
  {
    detail::LockGuard const lock{_spinlock};
    return _metric_keys.find(metric_key) != _metric_keys.end();
  }

  void register_counter(MetricMetadata const* metric_metadata, std::string help, Labels constant_labels = {})
  {
    _register_metric<counter_family_t>(
      metric_metadata, prometheus::MetricType::Counter, std::move(help), std::move(constant_labels),
      [](counter_family_t* family, Labels const& metric_labels, FamilyKey const& family_key)
      { return RegisteredCounter{family_key, &family->Add(metric_labels)}; });
  }

  void register_gauge(MetricMetadata const* metric_metadata, std::string help,
                      GaugeUpdateMode update_mode = GaugeUpdateMode::Set, Labels constant_labels = {})
  {
    _register_metric<gauge_family_t>(
      metric_metadata, prometheus::MetricType::Gauge, std::move(help), std::move(constant_labels),
      [update_mode](gauge_family_t* family, Labels const& metric_labels, FamilyKey const& family_key)
      { return RegisteredGauge{family_key, &family->Add(metric_labels), update_mode}; });
  }

  void register_histogram(MetricMetadata const* metric_metadata, std::string help,
                          HistogramBuckets bucket_boundaries, Labels constant_labels = {})
  {
    _register_metric<histogram_family_t>(
      metric_metadata, prometheus::MetricType::Histogram, std::move(help), std::move(constant_labels),
      [buckets = std::move(bucket_boundaries)](
        histogram_family_t* family, Labels const& metric_labels, FamilyKey const& family_key) mutable
      { return RegisteredHistogram{family_key, &family->Add(metric_labels, std::move(buckets))}; });
  }

  void register_summary(MetricMetadata const* metric_metadata, std::string help, SummaryQuantiles quantiles,
                        std::chrono::milliseconds max_age = std::chrono::seconds{60},
                        int age_buckets = 5, Labels constant_labels = {})
  {
    _register_metric<summary_family_t>(
      metric_metadata, prometheus::MetricType::Summary, std::move(help), std::move(constant_labels),
      [quantiles = _to_prometheus_quantiles(quantiles), max_age, age_buckets](
        summary_family_t* family, Labels const& metric_labels, FamilyKey const& family_key)
      {
        return RegisteredSummary{family_key, &family->Add(metric_labels, quantiles, max_age, age_buckets)};
      });
  }

  bool unregister_metric(MetricMetadata const* metric_metadata)
  {
    MetricMetadata const& validated_metric = _validate_metric_metadata(metric_metadata);

    detail::LockGuard const lock{_spinlock};
    return _unregister_metric(&validated_metric);
  }

  bool unregister_metric(std::string const& metric_key)
  {
    detail::LockGuard const lock{_spinlock};

    auto metric_key_it = _metric_keys.find(metric_key);
    if (metric_key_it == _metric_keys.end())
    {
      return false;
    }

    auto metric_it = _metrics.find(metric_key_it->second);
    QUILL_ASSERT(metric_it != _metrics.end(),
                 "PrometheusSink _metric_keys is out of sync with _metrics");

    _erase_metric(metric_key_it, metric_it);
    return true;
  }

  void write_log(MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                 std::string const&, std::string_view, LogLevel, std::string_view, std::string_view,
                 std::vector<std::pair<std::string, std::string>> const*, std::string_view, std::string_view) override
  {
    // Intentionally ignore log events.
    //
    // PrometheusSink only exports metric samples, but a logger may still fan out to both normal
    // log sinks and this metrics sink at the same time. In that setup, log events should continue
    // to flow to the regular log sinks without PrometheusSink asserting or throwing here.
  }

  void write_metric(MetricMetadata const* metric_metadata, uint64_t, std::string_view,
                    std::string_view, std::string const&, std::string_view, double value) override
  {
    QUILL_ASSERT(metric_metadata,
                 "PrometheusSink::write_metric received a null metric metadata pointer");

    detail::LockGuard const lock{_spinlock};

    auto metric_it = _metrics.find(metric_metadata);
    if (metric_it == _metrics.end())
    {
      // Metric registrations can race with queued backend events. Late samples for an
      // unregistered metric are ignored.
      return;
    }

    std::visit([value](auto& registered_metric) { _apply_sample(registered_metric, value); },
               metric_it->second);
  }

  void flush_sink() noexcept override {}

  void run_periodic_tasks() noexcept override {}

private:
  QUILL_NODISCARD static std::optional<std::string> _resolve_scrape_bind_address(ExposerConfiguration const& exposer_config)
  {
    if (exposer_config.server_options.empty())
    {
      if (!_can_form_scrape_endpoint(exposer_config.bind_address))
      {
        return std::nullopt;
      }

      return exposer_config.bind_address;
    }

    return _extract_listening_ports_option(exposer_config.server_options);
  }

  QUILL_NODISCARD static std::optional<std::string> _extract_listening_ports_option(std::vector<std::string> const& server_options)
  {
    for (size_t idx = 0; (idx + 1u) < server_options.size(); idx += 2u)
    {
      if (server_options[idx] != "listening_ports")
      {
        continue;
      }

      std::string const& candidate = server_options[idx + 1u];

      if (candidate.empty() || (candidate.find(',') != std::string::npos) || !_can_form_scrape_endpoint(candidate))
      {
        return std::nullopt;
      }

      return candidate;
    }

    return std::nullopt;
  }

  QUILL_NODISCARD static bool _can_form_scrape_endpoint(std::string const& bind_address) noexcept
  {
    size_t const last_colon = bind_address.rfind(':');

    if ((last_colon == std::string::npos) || (last_colon == 0u) ||
        ((last_colon + 1u) >= bind_address.size()))
    {
      return false;
    }

    for (size_t idx = last_colon + 1u; idx < bind_address.size(); ++idx)
    {
      char const ch = bind_address[idx];

      if ((ch < '0') || (ch > '9'))
      {
        return false;
      }
    }

    return true;
  }

  QUILL_NODISCARD static bool _bind_address_uses_ephemeral_port(std::string const& bind_address) noexcept
  {
    size_t const last_colon = bind_address.rfind(':');

    if (last_colon == std::string::npos || (last_colon + 1u) >= bind_address.size())
    {
      return false;
    }

    bool has_digit = false;

    for (size_t idx = last_colon + 1u; idx < bind_address.size(); ++idx)
    {
      char const ch = bind_address[idx];

      if ((ch < '0') || (ch > '9'))
      {
        return false;
      }

      has_digit = true;

      if (ch != '0')
      {
        return false;
      }
    }

    return has_digit;
  }

  static void _replace_bind_address_port(std::string& bind_address, int port)
  {
    QUILL_ASSERT(port > 0, "port must be positive in PrometheusSink::_replace_bind_address_port()");

    size_t const last_colon = bind_address.rfind(':');
    QUILL_ASSERT(last_colon != std::string::npos,
                 "bind_address must contain a ':' in PrometheusSink::_replace_bind_address_port()");

    bind_address.resize(last_colon + 1u);
    bind_address.append(std::to_string(port));
  }

  using counter_family_t = prometheus::Family<prometheus::Counter>;
  using gauge_family_t = prometheus::Family<prometheus::Gauge>;
  using histogram_family_t = prometheus::Family<prometheus::Histogram>;
  using summary_family_t = prometheus::Family<prometheus::Summary>;

  struct FamilyKey
  {
    prometheus::MetricType type;
    std::string metric_name;
    Labels constant_labels;
  };

  struct RegisteredCounter
  {
    FamilyKey family_key;
    prometheus::Counter* metric;
  };

  struct RegisteredGauge
  {
    FamilyKey family_key;
    prometheus::Gauge* metric;
    GaugeUpdateMode update_mode;
  };

  struct RegisteredHistogram
  {
    FamilyKey family_key;
    prometheus::Histogram* metric;
  };

  struct RegisteredSummary
  {
    FamilyKey family_key;
    prometheus::Summary* metric;
  };

  using family_variant_t =
    std::variant<counter_family_t*, gauge_family_t*, histogram_family_t*, summary_family_t*>;

  using metric_variant_t =
    std::variant<RegisteredCounter, RegisteredGauge, RegisteredHistogram, RegisteredSummary>;

  struct FamilyHandle
  {
    std::string help;
    size_t metric_count{0};
    family_variant_t family;
  };

private:
  bool _unregister_metric(MetricMetadata const* metric_metadata)
  {
    auto metric_it = _metrics.find(metric_metadata);
    if (metric_it == _metrics.end())
    {
      return false;
    }

    auto metric_key_it = _metric_keys.find(metric_metadata->metric_key());
    QUILL_ASSERT(metric_key_it != _metric_keys.end(),
                 "PrometheusSink _metrics is out of sync with _metric_keys");

    _erase_metric(metric_key_it, metric_it);
    return true;
  }

  void _erase_metric(std::unordered_map<std::string, MetricMetadata const*>::iterator metric_key_it,
                     std::unordered_map<MetricMetadata const*, metric_variant_t>::iterator metric_it)
  {
    FamilyKey const family_key = std::visit(
      [](auto const& registered_metric) { return registered_metric.family_key; }, metric_it->second);

    auto family_it = _families.find(family_key);
    QUILL_ASSERT(family_it != _families.end(), "PrometheusSink family handle is missing");

    std::visit([&family_handle = family_it->second](auto const& registered_metric)
               { _remove_metric_from_family(family_handle, registered_metric); }, metric_it->second);

    if (family_it->second.metric_count > 0)
    {
      --family_it->second.metric_count;
    }

    if (family_it->second.metric_count == 0)
    {
      _families.erase(family_it);
    }

    _metric_keys.erase(metric_key_it);
    _metrics.erase(metric_it);
  }

  friend bool operator<(FamilyKey const& lhs, FamilyKey const& rhs) noexcept
  {
    return std::tie(lhs.type, lhs.metric_name, lhs.constant_labels) <
      std::tie(rhs.type, rhs.metric_name, rhs.constant_labels);
  }

  static MetricMetadata const& _validate_metric_metadata(MetricMetadata const* metric_metadata)
  {
    if (QUILL_UNLIKELY(metric_metadata == nullptr))
    {
      QUILL_THROW(QuillError{"PrometheusSink requires a valid MetricMetadata pointer"});
    }

    if (QUILL_UNLIKELY(metric_metadata->event() != MacroMetadata::Event::Metric))
    {
      QUILL_THROW(QuillError{"PrometheusSink requires MetricMetadata created for Event::Metric"});
    }

    return *metric_metadata;
  }

  void _ensure_metric_not_registered(std::string const& metric_key) const
  {
    if (_metric_keys.find(metric_key) != _metric_keys.end())
    {
      QUILL_THROW(QuillError{"PrometheusSink metric \"" + metric_key + "\" is already registered"});
    }
  }

  template <typename TFamily, typename TAdder>
  void _register_metric(MetricMetadata const* metric_metadata, prometheus::MetricType metric_type,
                        std::string help, Labels constant_labels, TAdder adder)
  {
    MetricMetadata const& validated_metric = _validate_metric_metadata(metric_metadata);
    Labels const metric_labels = _make_metric_labels(validated_metric);
    _validate_prometheus_metadata(metric_type, validated_metric.metric_name(), constant_labels, metric_labels);

    detail::LockGuard const lock{_spinlock};
    _ensure_metric_not_registered(validated_metric.metric_key());

    FamilyKey const family_key{metric_type, validated_metric.metric_name(), std::move(constant_labels)};
    FamilyHandle& family_handle = _get_or_create_family<TFamily>(family_key, std::move(help));

    auto* family = std::get<TFamily*>(family_handle.family);

    if (family->Has(metric_labels))
    {
      QUILL_THROW(QuillError{"PrometheusSink metric \"" + validated_metric.metric_key() +
                             "\" aliases an existing time series"});
    }

    _metrics.emplace(metric_metadata, adder(family, metric_labels, family_key));
    _metric_keys.emplace(validated_metric.metric_key(), metric_metadata);
    ++family_handle.metric_count;
  }

  static Labels _make_metric_labels(MetricMetadata const& metric_metadata)
  {
    Labels labels;

    for (MetricLabel const& label : metric_metadata.labels())
    {
      auto const insert_result = labels.emplace(label.key, label.value);

      if (!insert_result.second)
      {
        QUILL_THROW(QuillError{"PrometheusSink metric \"" + metric_metadata.metric_key() +
                               "\" contains duplicate label key \"" + label.key + "\""});
      }
    }

    return labels;
  }

  static void _validate_prometheus_metadata(prometheus::MetricType metric_type, std::string const& metric_name,
                                            Labels const& constant_labels, Labels const& metric_labels)
  {
    if (!prometheus::CheckMetricName(metric_name))
    {
      QUILL_THROW(QuillError{"Invalid Prometheus metric name \"" + metric_name + "\""});
    }

    for (auto const& label : constant_labels)
    {
      if (!prometheus::CheckLabelName(label.first, metric_type))
      {
        QUILL_THROW(QuillError{"Invalid Prometheus constant label name \"" + label.first + "\""});
      }
    }

    for (auto const& label : metric_labels)
    {
      if (!prometheus::CheckLabelName(label.first, metric_type))
      {
        QUILL_THROW(QuillError{"Invalid Prometheus metric label name \"" + label.first + "\""});
      }

      if (constant_labels.find(label.first) != constant_labels.end())
      {
        QUILL_THROW(QuillError{"Prometheus label \"" + label.first +
                               "\" is defined both as a constant label and as a metric label"});
      }
    }
  }

  static prometheus::Summary::Quantiles _to_prometheus_quantiles(SummaryQuantiles const& quantiles)
  {
    prometheus::Summary::Quantiles prometheus_quantiles;
    prometheus_quantiles.reserve(quantiles.size());

    for (SummaryQuantile const& quantile : quantiles)
    {
      prometheus_quantiles.emplace_back(
        prometheus::detail::CKMSQuantiles::Quantile{quantile.quantile, quantile.error});
    }

    return prometheus_quantiles;
  }

  template <typename TFamily>
  FamilyHandle& _get_or_create_family(FamilyKey const& family_key, std::string help)
  {
    auto family_it = _families.find(family_key);
    if (family_it != _families.end())
    {
      if (family_it->second.help != help)
      {
        QUILL_THROW(QuillError{"Prometheus family \"" + family_key.metric_name +
                               "\" is already registered with different help text"});
      }

      return family_it->second;
    }

    FamilyHandle family_handle;
    family_handle.help = std::move(help);

    if constexpr (std::is_same_v<TFamily, counter_family_t>)
    {
      family_handle.family = &prometheus::BuildCounter()
                                .Name(family_key.metric_name)
                                .Help(family_handle.help)
                                .Labels(family_key.constant_labels)
                                .Register(_registry);
    }
    else if constexpr (std::is_same_v<TFamily, gauge_family_t>)
    {
      family_handle.family = &prometheus::BuildGauge()
                                .Name(family_key.metric_name)
                                .Help(family_handle.help)
                                .Labels(family_key.constant_labels)
                                .Register(_registry);
    }
    else if constexpr (std::is_same_v<TFamily, histogram_family_t>)
    {
      family_handle.family = &prometheus::BuildHistogram()
                                .Name(family_key.metric_name)
                                .Help(family_handle.help)
                                .Labels(family_key.constant_labels)
                                .Register(_registry);
    }
    else
    {
      static_assert(std::is_same_v<TFamily, summary_family_t>, "Unsupported Prometheus family type");
      family_handle.family = &prometheus::BuildSummary()
                                .Name(family_key.metric_name)
                                .Help(family_handle.help)
                                .Labels(family_key.constant_labels)
                                .Register(_registry);
    }

    auto const insert_result = _families.emplace(family_key, std::move(family_handle));
    return insert_result.first->second;
  }

  static void _remove_metric_from_family(FamilyHandle& family_handle, RegisteredCounter const& registered_metric)
  {
    std::get<counter_family_t*>(family_handle.family)->Remove(registered_metric.metric);
  }

  static void _remove_metric_from_family(FamilyHandle& family_handle, RegisteredGauge const& registered_metric)
  {
    std::get<gauge_family_t*>(family_handle.family)->Remove(registered_metric.metric);
  }

  static void _remove_metric_from_family(FamilyHandle& family_handle, RegisteredHistogram const& registered_metric)
  {
    std::get<histogram_family_t*>(family_handle.family)->Remove(registered_metric.metric);
  }

  static void _remove_metric_from_family(FamilyHandle& family_handle, RegisteredSummary const& registered_metric)
  {
    std::get<summary_family_t*>(family_handle.family)->Remove(registered_metric.metric);
  }

  static void _apply_sample(RegisteredCounter& registered_metric, double value)
  {
    registered_metric.metric->Increment(value);
  }

  static void _apply_sample(RegisteredGauge& registered_metric, double value)
  {
    if (registered_metric.update_mode == GaugeUpdateMode::Add)
    {
      registered_metric.metric->Increment(value);
    }
    else
    {
      registered_metric.metric->Set(value);
    }
  }

  static void _apply_sample(RegisteredHistogram& registered_metric, double value)
  {
    registered_metric.metric->Observe(value);
  }

  static void _apply_sample(RegisteredSummary& registered_metric, double value)
  {
    registered_metric.metric->Observe(value);
  }

private:
  Registry _registry;
  std::shared_ptr<prometheus::Collectable> _registry_collectable{
    &_registry, [](prometheus::Collectable*) noexcept {}};
  std::unique_ptr<Exposer> _exposer;
  std::string _exposer_scrape_endpoint;
  mutable detail::Spinlock _spinlock;
  std::map<FamilyKey, FamilyHandle> _families;
  std::unordered_map<MetricMetadata const*, metric_variant_t> _metrics;
  std::unordered_map<std::string, MetricMetadata const*> _metric_keys;
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
