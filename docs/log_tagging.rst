.. title:: Log Tagging

Log Tagging
===========

Use this page to add static or dynamic tags to log messages for categorization, filtering, and
monitoring.

Compile-Time Tags
-----------------

The ``_TAGS`` macros store their tags in the call site's static metadata. This avoids copying tag
text with every record and is the preferred path when the tags are known at compile time.

To include these tags in the output, add the ``%(tags)`` placeholder to
:cpp:class:`PatternFormatterOptions`.

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

For tags selected at runtime, the macro-free API accepts a :cpp:struct:`Tags` object built
from ``char const*``, ``std::string``, or ``std::string_view`` values. The values are copied into
the ``Tags`` object, so a string view does not need to be null-terminated and its source only needs
to remain valid while the ``Tags`` constructor runs.

.. code-block:: cpp

   std::string execution_tag = current_execution_tag();
   quill::info(logger, quill::Tags{execution_tag}, "Task resumed");

Macro-free logging uses the hybrid runtime-metadata path, which copies the combined tags into the
frontend queue before the call returns. The
``QUILL_LOG_RUNTIME_METADATA_DEEP`` and ``QUILL_LOG_RUNTIME_METADATA_HYBRID`` macros can likewise
carry dynamic null-terminated tag strings. ``QUILL_LOG_RUNTIME_METADATA_SHALLOW`` only stores the
tag pointer, so that string must remain valid and immutable until the backend consumes the record.

The regular ``LOG_<LEVEL>_TAGS`` macros remain the lowest-latency choice for static tags; runtime
``std::string`` and ``std::string_view`` values cannot be stored in their compile-time metadata.

Backend Tag Processing
----------------------

Set :cpp:member:`PatternFormatterOptions::process_tags` to transform the combined tag text
on the backend before it is inserted into ``%(tags)``. The callback receives a valid
null-terminated string and returns the text to display. See :doc:`Formatters <formatters>` for an
example.

Tag Processing in Sinks
-----------------------

Tags can be accessed at the Sink level and used for additional log processing or filtering. This enables more log handling based on tag content. For example:

.. literalinclude:: snippets/quill_docs_example_tags_with_custom_sink.cpp
   :language: cpp
   :linenos:
