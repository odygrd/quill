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

.. code:: cpp

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/sinks/FileSink.h"
    #include "quill/std/WideString.h"

    #include <string>
    #include <string_view>
    #include <utility>

    int main()
    {
      quill::BackendOptions backend_options;
      backend_options.check_printable_char = {}; // Preserve UTF-8 output for non-ASCII text.
      quill::Backend::start(backend_options);

      auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("wide_strings.log");
      quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));

      std::wstring ws = L"example";
      std::wstring_view wsv{L"string_view"};
      std::wstring unicode_ws = L"\u4E2D\u6587 \u03A9";
      LOG_INFO(logger, "This is a log info example {} {} {}", ws, wsv, unicode_ws);
    }
