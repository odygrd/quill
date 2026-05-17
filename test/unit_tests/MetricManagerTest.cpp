#include "doctest/doctest.h"

#include "quill/core/Metric.h"
#include "quill/core/MetricManager.h"

TEST_SUITE_BEGIN("MetricManager");

using namespace quill;
using namespace quill::detail;

TEST_CASE("metric_metadata")
{
  static std::vector<MetricLabel> http_requests_labels{{"method", "GET"}, {"status", "200"}};

  static MetricMetadata http_requests_metric{"http_requests_total", "http_requests_total",
                                             http_requests_labels};

  REQUIRE_EQ(std::string_view{http_requests_metric.metric_name()}, std::string_view{"http_requests_total"});
  REQUIRE_EQ(http_requests_metric.labels().size(), 2u);
  REQUIRE_EQ(http_requests_metric.event(), MacroMetadata::Event::Metric);
}

TEST_CASE("create_or_get_metric")
{
  std::vector<MetricLabel> labels{{"method", "GET"}, {"status", "200"}};
  std::vector<MetricLabel> different_labels{{"method", "POST"}};

  MetricMetadata const* metric_metadata_1 = MetricManager::instance().create_or_get_metric(
    "runtime_http_requests_total_01", "runtime_http_requests_total", labels);
  MetricMetadata const* metric_metadata_2 = MetricManager::instance().create_or_get_metric(
    "runtime_http_requests_total_01", "runtime_http_requests_total_renamed", different_labels);
  MetricMetadata const* metric_metadata_3 =
    MetricManager::instance().get_metric("runtime_http_requests_total_01");

  REQUIRE(metric_metadata_1);
  REQUIRE(metric_metadata_2);
  REQUIRE_EQ(metric_metadata_1, metric_metadata_2);
  REQUIRE_EQ(metric_metadata_1, metric_metadata_3);
  REQUIRE_EQ(std::string_view{metric_metadata_1->metric_name()}, std::string_view{"runtime_http_requests_total"});
  REQUIRE_EQ(metric_metadata_1->labels().size(), 2u);
  REQUIRE_EQ(metric_metadata_1->labels()[0].key, "method");
  REQUIRE_EQ(metric_metadata_1->labels()[0].value, "GET");
  REQUIRE_EQ(metric_metadata_1->labels()[1].key, "status");
  REQUIRE_EQ(metric_metadata_1->labels()[1].value, "200");
}

#ifndef QUILL_NO_EXCEPTIONS
TEST_CASE("create_metric_throws_on_duplicate")
{
  std::vector<MetricLabel> labels{{"method", "POST"}, {"status", "500"}};

  MetricMetadata const* metric_metadata = MetricManager::instance().create_or_get_metric(
    "runtime_http_requests_total_duplicate_02", "runtime_http_requests_total_duplicate", labels);

  REQUIRE(metric_metadata);

  REQUIRE_THROWS_AS((
                      [&labels]()
                      {
                        (void)MetricManager::instance().create_metric(
                          "runtime_http_requests_total_duplicate_02",
                          "runtime_http_requests_total_duplicate", labels);
                      })(),
                    quill::QuillError);
}
#endif

TEST_SUITE_END();
