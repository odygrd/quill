/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Common.h"
#include "quill/core/MacroMetadata.h"

#include <string>
#include <vector>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

struct MetricLabel
{
  MetricLabel() = default;
  MetricLabel(std::string k, std::string v) : key(std::move(k)), value(std::move(v)) {}

  std::string key;
  std::string value;
};

/**
 * MetricMetadata extends MacroMetadata so the existing header-encoding path can carry it in the
 * `macro_metadata` slot on the SPSC queue. It is marked `final` because MacroMetadata has no
 * virtual destructor: any future owner of `unique_ptr<MacroMetadata>` pointing at a
 * MetricMetadata would slice or leak. MetricManager stores `unique_ptr<MetricMetadata>` directly,
 * so there is no slicing today.
 */
class MetricMetadata final : public MacroMetadata
{
public:
  MetricMetadata(std::string metric_key, std::string metric_name, std::vector<MetricLabel> labels = {})
    : MacroMetadata("", "", "", nullptr, LogLevel::None, MacroMetadata::Event::Metric),
      _metric_key(std::move(metric_key)),
      _metric_name(std::move(metric_name)),
      _labels(std::move(labels))
  {
  }

  QUILL_NODISCARD std::string const& metric_key() const noexcept { return _metric_key; }

  QUILL_NODISCARD std::string const& metric_name() const noexcept { return _metric_name; }

  QUILL_NODISCARD std::vector<MetricLabel> const& labels() const noexcept { return _labels; }

private:
  std::string _metric_key;
  std::string _metric_name;
  std::vector<MetricLabel> _labels;
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
