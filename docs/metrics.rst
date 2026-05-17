.. title:: Metrics

Metrics
=======

Use this page to publish metrics with Quill and export them through either a custom sink or the built-in ``PrometheusSink``.

Quill's metric API gives you a **low-latency transport** for publishing application metric samples
to an external metrics backend (Prometheus, StatsD, OpenTelemetry, or any in-process collector)
through the same asynchronous pipeline used for logs. Quill does not aggregate or expose metrics
on its own — your sink hands samples off to the metrics backend, which is responsible for
storage, aggregation, and exposition.

- **Register once, publish cheaply** — metric names and labels live in a stable ``MetricMetadata`` object; the hot path enqueues only a pointer and a ``double`` sample.
- **Same backend worker as logs** — no dedicated metrics thread, no extra context-switch cost, no duplicate formatting path on hot threads.
- **Backend-agnostic transport** — the metric API itself transports only ``(timestamp, MetricMetadata*, Logger*, double)``. Sinks decide where samples go.
- **Ready-made Prometheus exporter** — the built-in :cpp:class:`PrometheusSink` handles counters, gauges, histograms, and summaries, including an optional HTTP exposer.

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

One MetricMetadata per Label Combination
----------------------------------------

Labels on ``MetricMetadata`` are fixed at registration time. Each distinct combination of label
values is a separate time series and needs its own ``MetricMetadata`` pointer. This is intentional:
the hot path stays a pointer plus a ``double``, with no per-sample label lookup or string work.

If you have a known, bounded set of label values, register one ``MetricMetadata`` per combination
up front and dispatch to the right pointer on the hot path:

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

Avoid creating ``MetricMetadata`` on the hot path. ``MetricMetadata`` objects live in a global
manager for the entire program duration and are not freed; creating them per-request would leak
memory and serialize on the manager's lock.

If your label values are not known up front (for example, a label that takes user-provided
strings), drive the registration of new series from a setup or admin path rather than from the
request path.

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

Histogram Bucket Helpers
~~~~~~~~~~~~~~~~~~~~~~~~

Prometheus histograms need a strictly increasing list of bucket boundaries. Quill ships small
helpers so you don't have to spell those out by hand. Quill has no opinion on the units, since
some users measure in nanoseconds, others in milliseconds, seconds, bytes, dollars, etc.

- ``linear_buckets(start, width, count)`` — evenly-spaced boundaries.

  Each bucket is the previous one **plus** ``width``.

  Example: ``linear_buckets(0.0, 5.0, 5)`` → ``{0, 5, 10, 15, 20}``.

  ``start`` may be negative, which is useful when the metric can go below zero (for example,
  ``linear_buckets(-5.0, 1.0, 11)`` → ``{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5}``). ``width`` must
  be > 0.

- ``exponential_buckets(start, factor, count)`` — geometrically-spaced boundaries.

  Each bucket is the previous one **multiplied by** ``factor``.

  Example: ``exponential_buckets(1.0, 2.0, 5)`` → ``{1, 2, 4, 8, 16}``.
  Example: ``exponential_buckets(0.001, 10.0, 4)`` → ``{0.001, 0.01, 0.1, 1}``.

  Use this when the metric spans several orders of magnitude (latencies, sizes). ``start`` must
  be > 0 and ``factor`` must be > 1. For signed quantities, construct the bucket vector by hand —
  a strictly-increasing list of doubles is all Prometheus requires.

Quick rule of thumb: pick **linear** when each step should differ by the same amount, and
**exponential** when each step should differ by the same factor.

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
