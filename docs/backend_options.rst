.. title:: Backend Options

Backend Options
===============

The backend options allow you to customize the behavior of the backend thread and are applied at runtime. To utilize these options, you need to create an object and pass it when starting the backend thread. Most options come with carefully tuned default values for general use, but they can be adjusted to meet the specific performance and latency requirements of your application. Refer to :cpp:struct:`BackendOptions` for detailed information.

Example Usage
-------------

For example, to pin the backend worker thread to a specific CPU, you can use the following code:

.. literalinclude:: examples/quill_docs_example_backend_options.cpp
   :language: cpp
   :linenos:

Character Sanitization
-----------------------

By default, Quill filters log messages to ensure they contain only printable characters before writing them to sinks. This safety feature converts non-printable characters (including non-ASCII characters like Chinese, Japanese, or other Unicode text) to their hexadecimal representation (e.g., ``\xE4\xB8\xAD`` for Chinese characters).

The default printable character range is limited to ASCII characters from space (``' '``) to tilde (``'~'``), plus newline (``'\n'``).

**Disabling Character Sanitization for UTF-8 Logging**

If you need to log UTF-8 or other non-ASCII text (such as Chinese, etc.), you can disable character sanitization:

.. code-block:: cpp

   quill::BackendOptions backend_options;
   backend_options.check_printable_char = {};  // Disable sanitization
   quill::Backend::start(backend_options);

**Custom Character Filtering**

You can also define custom printable character rules:

.. code-block:: cpp

   quill::BackendOptions backend_options;
   backend_options.check_printable_char = [](char c) {
       // Allow ASCII printable + newline + common UTF-8 byte ranges
       return (c >= ' ' && c <= '~') || (c == '\n') || (static_cast<unsigned char>(c) >= 128);
   };
   quill::Backend::start(backend_options);
