.. title:: Metrics

Metrics
=======

Use this page to publish metrics with Quill and export them through either a custom sink or the built-in ``PrometheusSink``.

Metrics go through the same backend worker as logs. You register ``MetricMetadata`` once during
setup, keep the returned pointer, and then publish ``double`` samples through a logger. The sink
attached to that logger receives the samples on the backend thread through ``Sink::write_metric()``.

Registering Metrics
-------------------

Create metric metadata during setup and store the returned pointer where the hot path can reuse it:

.. code-block:: cpp

   quill::MetricMetadata const* requests_total = quill::Frontend::create_metric(
     "requests_total_post_500", "requests_total", {{"method", "POST"}, {"status", "500"}});

   quill::MetricMetadata const* request_latency = quill::Frontend::create_metric(
     "request_latency_post", "request_latency_seconds", {{"method", "POST"}});

The first argument is a unique ``metric_key`` used for lookup. The second is the metric name. The third is the set of static labels for that series.

If you want create-or-reuse semantics, use ``Frontend::create_or_get_metric()`` instead. It looks
up by ``metric_key`` and returns the existing metadata pointer unchanged if one already exists.

The returned ``MetricMetadata*`` is a stable pointer that lives for the entire program duration,
so you can safely cache it in globals, class members, or anywhere else the hot path can reach.

Publishing Samples
------------------

Metrics are published through a logger that is bound to a sink implementing ``write_metric()``:

.. code-block:: cpp

   // CustomMetricSink derives from quill::Sink and implements write_metric().
   auto metric_sink = quill::Frontend::create_or_get_sink<CustomMetricSink>("metric_sink");
   quill::Logger* metrics_logger =
     quill::Frontend::create_or_get_logger("metrics", std::move(metric_sink));

   // Using the macro, for symmetry with LOG_*.
   METRIC(metrics_logger, requests_total, 1.0);

   // Or the function form, if you prefer.
   metrics_logger->publish_metric(request_latency, 0.0023);

Use ``METRIC(...)`` if you want a macro that mirrors the ``LOG_*`` APIs, or call ``Logger::publish_metric()`` directly.

The publish call only queues the sample. The sink receives it on the backend worker thread,
together with the metric metadata, timestamp, thread information, process id, logger name, and
the sample value.

Writing a Metric Sink
---------------------

Metrics are delivered on the backend worker through the virtual method ``Sink::write_metric()``.
The default implementation is a no-op, so existing log sinks do not need any changes.

- Log-only sinks keep the default no-op ``write_metric()`` and ignore metric events.
- Metric-only sinks implement ``write_metric()`` and can leave ``write_log()`` as a no-op.
- Mixed sinks implement both methods and handle logs and metrics in the same sink.

This lets you export metrics to Prometheus, StatsD, Graphite, OpenTelemetry, or any other
backend.

Using PrometheusSink
--------------------

``PrometheusSink`` is the built-in sink for Prometheus. After creating the sink, register each
metric on it as a counter, gauge, histogram, or summary:

.. code-block:: cpp

   auto prom_sink = std::static_pointer_cast<quill::PrometheusSink>(
     quill::Frontend::create_or_get_sink<quill::PrometheusSink>("prometheus_sink", exposer_config));

   prom_sink->register_counter(requests_total, "Total number of handled requests");
   prom_sink->register_histogram(request_latency, "Request latency in seconds",
                                 {0.005, 0.01, 0.05, 0.1, 0.5, 1.0, 5.0});

Quill itself only transports ``double`` samples. The Prometheus type is chosen when you register
the metric on ``PrometheusSink``.

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
