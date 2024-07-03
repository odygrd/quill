.. title:: Wide Strings

Wide Strings
============

The library does not provide support for writing Unicode characters to log files; only ASCII characters are supported. However, on Windows, wide strings compatible with ASCII encoding are supported.

It is possible to pass wide characters, wide strings, or wide string views to the logger, but this functionality is specific to Windows.

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
    #include "quill/sinks/ConsoleSink.h"
    #include "quill/std/WideString.h"

    #include <string>
    #include <string_view>
    #include <utility>

    int main()
    {
      quill::Backend::start();

      auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
      quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

      std::wstring ws = L"example";
      std::wstring_view wsv{L"string_view"};
      LOG_INFO(logger, "This is a log info example {} {}", ws, wsv);
    }