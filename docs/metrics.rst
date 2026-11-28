.. title:: Metrics

.. meta::
   :description: Publish low-latency C++ application metrics through Quill's asynchronous backend to Prometheus, StatsD, OpenTelemetry, or custom collectors.

Metrics
=======

Publish application samples to Prometheus or another metrics system through Quill's logging worker.
The sink hands samples to the collector for storage, aggregation and exposition. Metrics need no extra
Quill thread, context switch or caller-side formatting.

Registering Metrics
-------------------

Register the series at setup and cache the returned pointer.

.. code-block:: cpp

   quill::MetricMetadata const* requests_total = quill::Frontend::create_metric(
     "requests_total_post_500", "requests_total", {{"method", "POST"}, {"status", "500"}});

   quill::MetricMetadata const* request_latency = quill::Frontend::create_metric(
     "request_latency_post", "request_latency_seconds", {{"method", "POST"}});

Pass a unique lookup key (``metric_key``), metric name and static labels. ``Frontend::create_or_get_metric()``
returns existing metadata unchanged for a known key. The pointer is stable for the program's lifetime and
can be stored in a member or global.

One MetricMetadata per Label Combination
----------------------------------------

Register each distinct set of label values as a separate series. For a bounded set, choose the cached pointer
at publish time to avoid per-sample label lookup and string work.

.. code-block:: cpp

   // For an endpoint that can answer GET/POST and return 200/500, register all four series
   // during setup and keep the pointers where the hot path can reach them.
   std::unordered_map<std::pair<std::string, std::string>, quill::MetricMetadata const*, ...> by_method_status;

   for (auto const& method : {"GET", "POST"})
   {
     for (auto const& status : {"200", "500"})
     {
       std::string key = std::string{"requests_total_"} + method + "_" + status;
       by_method_status[{method, status}] = quill::Frontend::create_metric(
         key, "requests_total", {{"method", method}, {"status", status}});
     }
   }

   // Hot path: pick the right pre-registered metadata pointer, then publish.
   METRIC(metrics_logger, by_method_status[{method, status}], 1.0);

Registration takes the global manager's lock and retains metadata for the program's lifetime. Avoid registering
per request. For runtime labels, see `Dynamic Labels`_.

Publishing Samples
------------------

Bind a logger to a sink implementing ``write_metric()``.

.. code-block:: cpp

   // CustomMetricSink derives from quill::Sink and implements write_metric().
   auto metric_sink = quill::Frontend::create_or_get_sink<CustomMetricSink>("metric_sink");
   quill::Logger* metrics_logger =
     quill::Frontend::create_or_get_logger("metrics", std::move(metric_sink));

   // Using the macro, for symmetry with LOG_*.
   METRIC(metrics_logger, requests_total, 1.0);

   // Or the function form, if you prefer.
   metrics_logger->publish_metric(request_latency, 0.0023);

``METRIC(...)`` and ``Logger::publish_metric()`` enqueue a fixed-size timestamp, metadata pointer, logger pointer
and ``double`` value. The sink receives the sample, metadata, timestamp, thread information, process ID and
logger name on the backend.

Dynamic Labels
--------------

``Logger::publish_dynamic_metric(name, labels, value)`` and ``DYNAMIC_METRIC(...)`` copy the name and labels
on every call. Pass labels inline as ``{{key, value}, ...}`` without allocating a label container, or use a
``std::vector<MetricLabel>``. Prefer cached metadata on latency-sensitive paths.

.. code-block:: cpp

   DYNAMIC_METRIC(metrics_logger, "requests_total",
                  {{"method", method}, {"status", status}}, 1.0);

Caller strings can change or be destroyed after return. Keep the number of distinct names and label sets
bounded because the backend retains their metadata for the program's lifetime.

Both APIs reach ``Sink::write_metric()``. A custom sink needs no Prometheus registration. For ``PrometheusSink``,
register the type by pointer or use ``register_*_family()`` without an initial pointer. Matching names and
labels share a series. Unregistering it removes all aliases and stops samples until its pointer is registered again.

Writing a Metric Sink
---------------------

Override ``Sink::write_metric()`` to forward samples to StatsD, Graphite, OpenTelemetry or an in-process collector.
Log-only sinks can inherit the no-op. Metric-only sinks can implement ``write_log()`` as a no-op. Implement
both methods for a mixed sink.

Using PrometheusSink
--------------------

Create a :cpp:class:`PrometheusSink` and register each series as a counter, gauge, histogram or summary: Quill
transports only ``double`` samples, and the Prometheus type is chosen at this registration.
Configure its optional HTTP exposer when the application should serve a scrape endpoint.

.. code-block:: cpp

   auto prom_sink = quill::Frontend::create_or_get_sink<quill::PrometheusSink>("prometheus_sink", exposer_config);

   prom_sink->register_counter(requests_total, "Total number of handled requests");
   prom_sink->register_histogram(request_latency, "Request latency in seconds",
                                 {0.005, 0.01, 0.05, 0.1, 0.5, 1.0, 5.0});

Histogram Bucket Helpers
~~~~~~~~~~~~~~~~~~~~~~~~

Provide strictly increasing boundaries in the units of your metric, such as nanoseconds, milliseconds,
seconds, bytes or dollars.

- ``linear_buckets(start, width, count)`` adds a positive width at each step. For example,
  ``linear_buckets(0.0, 5.0, 5)`` gives ``{0, 5, 10, 15, 20}``. For a negative start,
  ``linear_buckets(-5.0, 1.0, 11)`` gives ``{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5}``.
- ``exponential_buckets(start, factor, count)`` multiplies each step. Use a positive start and a factor
  greater than one for ranges spanning orders of magnitude, such as sizes and latencies.
  ``exponential_buckets(1.0, 2.0, 5)`` gives ``{1, 2, 4, 8, 16}`` and ``exponential_buckets(0.001, 10.0, 4)``
  gives ``{0.001, 0.01, 0.1, 1}``. Supply your own increasing vector of doubles for signed, non-linear ranges.

.. code-block:: cpp

   prom_sink->register_histogram(request_latency, "Request latency in seconds",
                                 quill::PrometheusSink::exponential_buckets(0.001, 2.0, 10));

Full examples
-------------

Custom metric sink:

.. literalinclude:: snippets/quill_docs_example_metric_publishing.cpp
   :language: cpp
   :linenos:

Prometheus integration:

.. literalinclude:: ../examples/metric_publishing_prometheus.cpp
   :language: cpp
   :linenos:
