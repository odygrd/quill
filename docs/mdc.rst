.. title:: Mapped Diagnostic Context

Mapped Diagnostic Context (MDC)
===============================

Use mapped diagnostic context (MDC) to attach request, user, tenant, shard or session IDs to a thread's
subsequent log lines. Capture fields while the variables are in scope. They persist until replaced,
erased or cleared.

Displaying MDC
--------------

The default pattern uses ``%(message)%(mdc)``. With fields set, ``%(mdc)`` supplies its own leading space
and brackets, for example ``" [request_id: 42, user: alice]"``. With no fields it expands to an empty string.
Omit extra brackets around it in custom patterns.

Basic Example
-------------

.. literalinclude:: snippets/quill_docs_example_mdc.cpp
   :language: cpp
   :linenos:

`examples/mdc_logging.cpp <https://github.com/odygrd/quill/blob/master/examples/mdc_logging.cpp>`_ shows a
custom pattern and several loggers sharing context. The basic example produces output like this.

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

Each ``set_mdc()``, ``erase_mdc()`` or ``clear_mdc()`` queues one control event. The backend keeps the current
fields per producer thread, so later log calls do not re-encode them.

Updating Values
---------------

MDC copies its values. After changing a source variable, call ``set_mdc()`` again to update the stored value.

.. code-block:: cpp

   int request_id = 42;
   logger->set_mdc("request_id", request_id);

   ++request_id;
   LOG_INFO(logger, "still prints request_id: 42");

   logger->set_mdc("request_id", request_id);
   LOG_INFO(logger, "now prints request_id: 43");

MDC Is Per Thread
-----------------

All loggers on a thread see its MDC. Other threads start with empty context and retain their own fields
when one thread clears its map.

Customizing the Rendered Block
------------------------------

``BackendOptions::mdc_format_pattern`` sets the global rendering of ``%(mdc)``.

.. code-block:: cpp

   #include "quill/Backend.h"

   quill::BackendOptions backend_options;
   backend_options.mdc_format_pattern = " <{} = {} | >";
   quill::Backend::start(backend_options);

This pattern produces the following block.

.. code-block:: text

   <request_id = 42 | user = alice>

Use exactly two ``{}`` placeholders. The surrounding text sets the prefix, key/value separator, field
separator and suffix. ``Backend::start()`` and ``ManualBackendWorker::init()`` validate the pattern at
startup and throw ``quill::QuillError`` if malformed.

- ``" [{}: {}, ]"`` renders ``" [key: value, key: value]"``
- ``" [{}: {},]"`` renders ``" [key: value,key: value]"``
- ``" <{} = {} | >"`` renders ``" <key = value | key = value>"``
