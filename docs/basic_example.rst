.. title:: Basic Example

Basic Example
=============

Use the full ``Backend`` and ``Frontend`` APIs to create a named logger that writes to the console.

.. literalinclude:: snippets/quill_docs_example_basic.cpp
   :language: cpp
   :linenos:

- Call ``Backend::start()`` to process messages. The worker stops automatically on normal exit.
- Reuse a sink or logger by its unique name. Each logger has a ``PatternFormatter`` for layout and one
  or more sinks for output.
- The example uses the lowest-latency ``LOG_*`` interface. See :doc:`macro_free_mode` for function calls.

For default loggers see :doc:`quick_start`. For sink, formatter and other settings see :doc:`guides`.
