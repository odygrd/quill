.. title:: Wide Strings

Wide Strings
============

On Windows, log ``wchar_t*``, ``std::wstring`` and ``std::wstring_view`` as UTF-8. On other platforms, pass
UTF-8 directly. To preserve non-ASCII bytes, disable or customize :cpp:member:`BackendOptions::check_printable_char`
as described in :ref:`backend_options:character sanitization`. Windows terminals also need the
:ref:`UTF-8 console code page <sink_types:consolesink>`.

How It Works
------------

The caller copies the full wide buffer to its queue. The worker converts it to UTF-8 before ordinary
narrow-string formatting and output. This avoids frontend transcoding, fmt's wide formatting context
and UTF-16 output files.

Include the matching ``quill/std`` headers for containers of wide strings. Nested containers such as
``std::vector<std::vector<std::wstring>>`` are unsupported for wide strings.

Wide String Logging Example
---------------------------

.. literalinclude:: snippets/quill_docs_wide_strings.cpp
   :language: cpp
   :linenos:
