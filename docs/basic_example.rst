.. title:: Basic Example

Basic Example
=============

.. literalinclude:: examples/quill_docs_example_basic.cpp
   :language: cpp
   :linenos:

In the example above, a console `Sink` is created and passed to a `Logger` with its name set to 'root'.

Each `Sink` and `Logger` must be assigned a unique name during creation to facilitate retrieval later.

Each :cpp:class:`Logger` contains a :cpp:class:`PatternFormatter` object responsible for formatting the message.

Moreover, each :cpp:class:`Logger` contains single or multiple :cpp:class:`Sink` objects that deliver the log message to their output source.

A single backend thread checks for new log messages periodically.

Starting the backend thread is the user's responsibility. The backend thread will automatically stop at the end of `main`, printing every message, as long as the application terminates gracefully.

The use of macros is unavoidable to achieve better runtime performance. The static information of a log (such as format string, log level, location) is created at compile time and passed along with the type of each argument to a decoding function. A template instantiation per log statement is created.