.. title:: Mapped Diagnostic Context

Mapped Diagnostic Context (MDC)
===============================

Use this page to attach request-scoped or task-scoped context to log lines, such as request ids, user ids, tenant ids, shard ids, or session ids.

MDC stands for mapped diagnostic context. In practice, it is a small thread-local key-value map that Quill appends to later log statements.

This is useful when you want to capture context while the relevant variables are in scope, then keep seeing that
context on later log messages from the same frontend thread even after those locals are no longer directly available at
the logging call site.

MDC lets you set that context once and have it appear on subsequent log statements from the same frontend thread until
you replace, erase, or clear it.

Quill's MDC is specifically designed so the steady-state hot path stays cheap: MDC updates are
sent to the backend as a single control event per change, and later log statements do **not**
re-serialize or re-enqueue all MDC fields on the frontend hot path. The backend keeps the current
MDC per frontend thread and reuses it for subsequent log lines from that thread.

Displaying MDC
--------------

Add ``%(mdc)`` to the pattern formatter. When no MDC is set, ``%(mdc)`` expands to an empty string.
By default, when fields are present, it renders a leading space and a bracketed block such as
`` [request_id: 42, user: alice]``.

For that reason, custom patterns will usually want ``%(message)%(mdc)`` rather than ``%(message) [%(mdc)]``.

The default pattern formatter already uses ``%(message)%(mdc)``.

Basic Example
-------------

.. literalinclude:: snippets/quill_docs_example_mdc.cpp
   :language: cpp
   :linenos:

A fuller standalone example with a custom pattern and multiple loggers sharing the same thread-local MDC lives at
`examples/mdc_logging.cpp <https://github.com/odygrd/quill/blob/master/examples/mdc_logging.cpp>`_.

This produces output similar to:

.. code-block:: shell

   12:34:56.123456789 app request started [request_id: 42, user: alice]
   12:34:56.123456790 db querying database [request_id: 42, user: alice]
   12:34:56.123456791 app user removed from context [request_id: 42]
   12:34:56.123456792 db request finished

Updating the Context
--------------------

- ``logger->set_mdc("request_id", 42, "user", "alice");`` sets or replaces one or more fields.
- ``logger->erase_mdc("user", "tenant");`` removes one or more fields.
- ``logger->clear_mdc();`` clears the whole MDC for the current thread.

Hot-Path Cost
-------------

MDC is designed so the steady-state logging path stays cheap.

- ``set_mdc()``, ``erase_mdc()``, and ``clear_mdc()`` are queued as control events, in the same style as other backend-bound logger operations.
- Later log statements do not re-encode and re-enqueue all MDC fields on the frontend hot path. The current MDC is only sent when you update it.
- The backend keeps the current MDC state for each frontend thread and reuses it for subsequent log messages from that thread.

Updating Values
---------------

MDC stores its own copy of the current values. If your local variable changes later, call ``set_mdc()`` again to
replace the value stored in the diagnostic context.

For example:

.. code-block:: cpp

   int request_id = 42;
   logger->set_mdc("request_id", request_id);

   ++request_id;
   LOG_INFO(logger, "still prints request_id: 42");

   logger->set_mdc("request_id", request_id);
   LOG_INFO(logger, "now prints request_id: 43");

This is the usual MDC model: changing the original variable does not update the stored context
automatically.

MDC Is Per Thread
-----------------

Although the API is exposed on ``Logger*``, the state is attached to the current frontend thread, not to an individual
logger instance.

- If the same thread logs through multiple loggers, they all see the same MDC.
- A different thread starts with empty MDC until it sets its own fields.
- Clearing MDC on one thread does not affect any other thread.

Customizing the Rendered Block
------------------------------

``BackendOptions::mdc_format_pattern`` controls how ``%(mdc)`` is rendered globally:

.. code-block:: cpp

   #include "quill/Backend.h"

   quill::BackendOptions backend_options;
   backend_options.mdc_format_pattern = " <{} = {} | >";
   quill::Backend::start(backend_options);

With that configuration, ``%(mdc)`` renders as:

.. code-block:: text

   <request_id = 42 | user = alice>

The format string uses exactly two ``{}`` placeholders. The text around them defines the rendered prefix, key/value
separator, field separator, and suffix.

Invalid patterns are rejected. ``Backend::start()`` and ``ManualBackendWorker::init()`` validate
``mdc_format_pattern`` eagerly and throw ``quill::QuillError`` if the pattern is malformed.

- ``" [{}: {}, ]"`` renders `` [key: value, key: value]``
- ``" [{}: {},]"`` renders `` [key: value,key: value]``
- ``" <{} = {} | >"`` renders `` <key = value | key = value>``
