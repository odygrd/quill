.. title:: Wide Strings

Wide Strings
============

Use this page if you need to log ``wchar_t`` strings on Windows. On other platforms, log UTF-8 strings directly after disabling character sanitization (see :doc:`Backend Options <backend_options>`).

On Windows, Quill supports ``wchar_t*``, ``std::wstring``, and ``std::wstring_view`` log arguments. The wide-string data is copied on the frontend and converted to UTF-8 on the backend before it is formatted and written to sinks. Quill does not use fmt's wide-character formatting context and does not write UTF-16 log files.

By default, Quill's character sanitizer allows only printable ASCII bytes, plus newline, tab, and carriage return. Non-ASCII UTF-8 bytes are converted to hexadecimal escapes (for example, Chinese text may appear as ``\xE4\xB8\xAD``). To write real UTF-8 Unicode text to file sinks, disable or customize :cpp:member:`BackendOptions::check_printable_char` (see :doc:`backend_options`).

.. note::

   Disable or customize ``BackendOptions::check_printable_char`` when you want non-ASCII wide-string output to appear as UTF-8 text in the sink. If the default sanitizer remains enabled, Unicode text is still logged safely, but non-ASCII bytes are written as hexadecimal escapes.

How It Works
------------

1. **Buffer Transfer**: The entire wide string buffer is copied from the caller thread to the queue.
2. **Backend Processing**: The backend thread reads the wide string and performs a UTF-8 encoding, converting it to a plain string.
3. **Formatting and Logging**: The plain string is then formatted with the log statement and written to the log file.

This method ensures that there is no overhead on the hot path since the encoding is handled in the backend thread.

Additionally, there is support for logging STL containers consisting of wide strings by including the relevant header files under ``quill/std``. Note that chaining STL types, such as ``std::vector<std::vector<std::wstring>>``, is not supported for wide strings.

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
