.. title:: Quick Start

Quick Start
===========

Quickest Setup
--------------

For the quickest and simplest setup:

.. literalinclude:: examples/quill_docs_quick_start.cpp
   :language: cpp
   :linenos:

Architecture Overview
---------------------

The library is header only and consists of two main components: the frontend and the backend.

The **frontend** captures a copy of the log arguments and metadata from each ``LOG_*`` statement and places them in a thread-local SPSC (Single Producer Single Consumer) queue buffer. Each frontend thread has its own lock-free queue, ensuring no contention between logging threads.

The **backend** runs in a separate thread, spawned by the library, asynchronously consuming messages from all frontend queues, formatting them, and writing them to the configured sinks.

Detailed Setup
--------------

For more detailed control, you need to start the backend thread in your application, then set up one or more output ``Sinks`` and a ``Logger``.

Once the initialization is complete, you only need to include two header files to issue log statements:

- ``#include "quill/LogMacros.h"``
- ``#include "quill/Logger.h"``

These headers have minimal dependencies, keeping compilation times low.

For even faster compilation, consider building the backend initialization as a static library, as shown in:
`Recommended Usage Example <https://github.com/odygrd/quill/tree/master/examples/recommended_usage>`_.

For a quick reference on usage see :doc:`Cheat Sheet <cheat_sheet>`.

Logging to Console
~~~~~~~~~~~~~~~~~~

.. literalinclude:: examples/quill_docs_example_console.cpp
   :language: cpp
   :linenos:

Logging to File
~~~~~~~~~~~~~~~

.. literalinclude:: examples/quill_docs_example_file.cpp
   :language: cpp
   :linenos:
