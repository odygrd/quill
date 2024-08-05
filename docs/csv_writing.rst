.. title:: CSV Writing

CSV Writing
===========

The library provides functionality for asynchronously writing CSV files. Formatting and I/O operations are managed by the backend thread, allowing for efficient and minimal-overhead CSV file writing on the hot path. This feature can be used alongside regular logging.

The `CsvWriter` class is a utility designed to facilitate asynchronous CSV file writing.

CSV Writing Example
-------------------

.. code:: cpp

    #include "quill/Backend.h"
    #include "quill/core/FrontendOptions.h"
    #include "quill/CsvWriter.h"
    #include "quill/sinks/ConsoleSink.h"
    #include "quill/LogMacros.h"

    struct OrderCsvSchema
    {
      static constexpr char const* header = "order_id,symbol,quantity,price,side";
      static constexpr char const* format = "{},{},{},{:.2f},{}";
    };

    int main()
    {
      quill::BackendOptions backend_options;
      quill::Backend::start(backend_options);

      quill::Logger* logger = quill::Frontend::create_or_get_logger("root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

      LOG_INFO(logger, "CSV writing example");

      quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writter {"orders.csv"};
      csv_writter.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
      csv_writter.append_row(132121123, "META", 300, 478.32321, "SELL");
      csv_writter.append_row(13212123, "AAPL", 120, 210.42321, "BUY");
    }