.. title:: Log Tagging

Log Tagging
===========

Tag messages to categorize, filter or monitor them. Choose compile-time tags for fixed categories or
dynamic tags for values selected at runtime.

Compile-Time Tags
-----------------

``LOG_<LEVEL>_TAGS`` keeps known tags in static call-site metadata, avoiding per-record string copies.
Runtime ``std::string`` and ``std::string_view`` values cannot go in that metadata. Add ``%(tags)`` to
:cpp:class:`PatternFormatterOptions` to display them.

.. literalinclude:: ../examples/tags_logging.cpp
   :language: cpp
   :linenos:

Output:

.. code:: shell

    2024-08-11 01:23:44.463 [46228] tags_logging.cpp:40          INFO      #random Debug with tags
    2024-08-11 01:23:44.463 [46228] tags_logging.cpp:41          INFO      #bar Info with tags
    2024-08-11 01:23:44.463 [46228] tags_logging.cpp:42          WARNING   #foo #bar Warning with tags
    2024-08-11 01:23:44.463 [46228] tags_logging.cpp:43          ERROR     #foo #bar #baz Info with tags
    2024-08-11 01:23:44.463 [46228] tags_logging.cpp:45          INFO      Without tags

Dynamic Tags
------------

For runtime values, pass :cpp:struct:`Tags` to a :doc:`logging function <macro_free_mode>`. Its constructor
copies and combines ``char const*``, ``std::string`` or ``std::string_view`` values. A view needs neither a null
terminator nor a lifetime beyond construction.

.. code-block:: cpp

   std::string execution_tag = current_execution_tag();
   quill::info(logger, quill::Tags{execution_tag}, "Task resumed");

A tagged logging function copies the combined text into the queue before returning. For the copy and
lifetime rules of ``QUILL_LOG_RUNTIME_METADATA_DEEP``, ``HYBRID`` and ``SHALLOW``, see
:ref:`logging_macros:runtime metadata logging macro`.

Backend Tag Processing
----------------------

:cpp:member:`PatternFormatterOptions::process_tags` takes the combined null-terminated string on the backend
and returns display text. See :doc:`formatters` for an example.

Tag Processing in Sinks
-----------------------

A sink can inspect tag content to route, filter or otherwise process a record.

.. literalinclude:: snippets/quill_docs_example_tags_with_custom_sink.cpp
   :language: cpp
   :linenos:
