.. title:: User's API

User's API
==========

Reference for the main public Quill API surface.

Use :doc:`Guides <guides>` and :doc:`Recipes <recipes>` for usage patterns and
examples. Use this page when you want the main public options, aliases, classes,
and sink types.

If you generate Doxygen for a downstream project, you can link ``quill::`` API
types through the published tag file:

.. code-block:: ini

   TAGFILES = https://quillcpp.readthedocs.io/en/latest/quill.tag=https://quillcpp.readthedocs.io/en/latest/

Backend Options
---------------

.. doxygenstruct:: BackendOptions
   :members:

Backend Class
-------------

.. doxygenclass:: quill::Backend
   :members:

BackendTscClock Class
---------------------

.. doxygenclass:: BackendTscClock
   :members:

SignalHandler Options
---------------------

.. doxygenstruct:: SignalHandlerOptions
   :members:

Frontend Options
----------------

.. doxygenstruct:: FrontendOptions
   :members:

Frontend Class
--------------

.. doxygenclass:: FrontendImpl
   :members:

Frontend Alias
--------------

.. doxygentypedef:: Frontend

MetricLabel Struct
------------------

.. doxygenstruct:: MetricLabel
   :members:

MetricMetadata Class
--------------------

.. doxygenclass:: MetricMetadata
   :members:

Log Levels
----------

.. doxygenenum:: LogLevel

LoggerImpl Class
----------------

.. doxygenclass:: LoggerImpl
   :members:

Logger Alias
------------

.. doxygentypedef:: Logger

PatternFormatter Class
----------------------

.. doxygenclass:: PatternFormatter
   :members:

PatternFormatterOptions Class
-----------------------------

.. doxygenclass:: PatternFormatterOptions
   :members:

Sink Class
----------

.. doxygenclass:: Sink
   :members:

Filter Class
------------

.. doxygenclass:: Filter
   :members:

FileSinkConfig Class
--------------------

.. doxygenclass:: FileSinkConfig
   :members:

FileSink Class
--------------------

.. doxygenclass:: FileSink
   :members:

File Event Types
----------------

.. doxygentypedef:: FileEventNotifierHandle

.. doxygenstruct:: FileEventNotifier
   :members:

RotatingSink Class
----------------------------

.. doxygenclass:: RotatingSink
   :members:

RotatingFileSinkConfig Class
----------------------------

.. doxygenclass:: RotatingFileSinkConfig
   :members:

RotatingFileSink Alias
----------------------

.. doxygentypedef:: RotatingFileSink

JsonFileSink Class
------------------

.. doxygenclass:: JsonFileSink
   :members:

RotatingJsonFileSink Alias
--------------------------

.. doxygentypedef:: RotatingJsonFileSink

JsonConsoleSink Class
---------------------

.. doxygenclass:: JsonConsoleSink
   :members:

StreamSink Class
----------------

.. doxygenclass:: StreamSink
   :members:

ConsoleSinkConfig Class
-----------------------

.. doxygenclass:: ConsoleSinkConfig
   :members:

ConsoleSink Class
-----------------

.. doxygenclass:: ConsoleSink
   :members:

AndroidSinkConfig Class
-----------------------

.. doxygenclass:: AndroidSinkConfig
   :members:

AndroidSink Class
-----------------

.. doxygenclass:: AndroidSink
   :members:

NullSink Class
--------------

.. doxygenclass:: NullSink
   :members:

SyslogSink Class
----------------

.. doxygenclass:: SyslogSink
   :members:

SyslogSinkConfig Class
----------------------

.. doxygenclass:: SyslogSinkConfig
   :members:

SystemdSink Class
-----------------

.. doxygenclass:: SystemdSink
   :members:

SystemdSinkConfig Class
-----------------------

.. doxygenclass:: SystemdSinkConfig
   :members:

PrometheusSink Class
--------------------

Optional sink for exporting Quill metric samples through ``prometheus-cpp``.

.. doxygenclass:: PrometheusSink
   :members: PrometheusSink, registry, exposer, exposer_scrape_endpoint, exposer_listening_ports, has_metric, register_counter, register_gauge, register_histogram, register_summary, unregister_metric

PrometheusSink::ExposerConfiguration Struct
-------------------------------------------

.. doxygenstruct:: PrometheusSink::ExposerConfiguration
   :members:

PrometheusSink::Options Struct
------------------------------

.. doxygenstruct:: PrometheusSink::Options
   :members:

PrometheusSink::SummaryQuantile Struct
--------------------------------------

.. doxygenstruct:: PrometheusSink::SummaryQuantile
   :members:

PrometheusSink::GaugeUpdateMode Enum
------------------------------------

.. doxygenenum:: PrometheusSink::GaugeUpdateMode

ManualBackendWorker Class
-------------------------

.. doxygenclass:: ManualBackendWorker
   :members:

CsvWriter Class
---------------

.. doxygenclass:: CsvWriter
   :members:
