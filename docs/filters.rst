.. title:: Filters

Filters
=======

Filters are used to selectively control which log statements are sent to specific ``Sinks`` based on defined criteria.

Each :cpp:class:`Sink` can be associated with one or multiple :cpp:class:`Filter` objects. These filters allow customization of log statement handling, such as filtering by log level or other criteria.

By default, a logger sends all log messages to its ``Sinks``. Filters provide a way to intercept and selectively process log records before they are outputted.

A filter is implemented as a callable object that evaluates each log statement on the backend thread and returns a boolean value. When the filter returns ``true``, the log statement is forwarded to the ``Sink``; when ``false``, the log statement is discarded.

Filtering Logs with the Built-In Filter
---------------------------------------

.. literalinclude:: examples/quill_docs_example_filter_1.cpp
   :language: cpp
   :linenos:

Creating a Custom Log Filter
----------------------------

.. literalinclude:: ../examples/user_defined_filter.cpp
   :language: cpp
   :linenos:
