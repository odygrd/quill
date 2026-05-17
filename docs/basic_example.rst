.. title:: Basic Example

Basic Example
=============

A minimal example using the full ``Backend`` / ``Frontend`` API with a console sink.

.. literalinclude:: snippets/quill_docs_example_basic.cpp
   :language: cpp
   :linenos:

**Key points:**

- A ``Sink`` and a ``Logger`` are each identified by a unique name so they can be retrieved later.
- Each ``Logger`` owns a ``PatternFormatter`` (controls layout) and one or more ``Sink`` objects (control output destinations).
- ``Backend::start()`` must be called before log messages can be processed. The backend thread stops automatically when the application exits normally.
- A macro-free logging interface is also available (see :doc:`Macro-Free Mode <macro_free_mode>`), but the ``LOG_*`` macros are recommended for lowest latency because static metadata is resolved at compile time and arguments are not evaluated when the level is disabled.

See :doc:`Quick Start <quick_start>` for the simple-setup path, or :doc:`Guides <guides>` for sinks, formatters, and advanced configuration.
