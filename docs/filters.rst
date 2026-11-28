.. title:: Filters

Filters
=======

Attach one or more :cpp:class:`Filter` objects to a :cpp:class:`Sink` to route messages by level, content or
custom criteria. By default, each sink receives all messages its logger accepts.

The backend evaluates each filter before writing. A ``false`` result discards the record for that sink.
All filters must return ``true`` for the sink to receive it.

Filtering Logs with the Built-In Filter
---------------------------------------

.. literalinclude:: snippets/quill_docs_example_filter.cpp
   :language: cpp
   :linenos:

Creating a Custom Log Filter
----------------------------

.. literalinclude:: ../examples/user_defined_filter.cpp
   :language: cpp
   :linenos:
