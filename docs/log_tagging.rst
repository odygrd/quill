.. title:: Log Tagging

Log Tagging
===========

In addition to creating multiple `Logger` instances, each with a unique name, you can add static compile-time tags to your log messages. These tags are embedded directly into the log message format at compile time, providing zero runtime overhead for categorization. This enhances your ability to search, monitor, categorize, and understand events in your software.

These static tags are included as hashtag-style keywords within your log messages, making it easier to filter and categorize logs based on these predefined tags.

To include tags in your log statements, use the `_TAGS` macros. You will also need to include the `%(tags)` placeholder in :cpp:class:`PatternFormatterOptions` for proper display of these tags.

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

Tag Processing in Sinks
-----------------------

Tags can be accessed at the Sink level and used for additional log processing or filtering. This enables more log handling based on tag content. For example:

.. literalinclude:: examples/quill_docs_example_tags_with_custom_sink.cpp
   :language: cpp
   :linenos: