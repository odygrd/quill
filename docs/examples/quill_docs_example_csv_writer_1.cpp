#include "quill/Backend.h"
#include "quill/CsvWriter.h"
#include "quill/LogMacros.h"
#include "quill/core/FrontendOptions.h"
#include "quill/sinks/ConsoleSink.h"

struct OrderCsvSchema
{
  static constexpr char const* header = "order_id,symbol,quantity,price,side";
  static constexpr char const* format = "{},{},{},{:.2f},{}";
};

int main()
{
  quill::Backend::start();

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

  LOG_INFO(logger, "CSV writing example");

  // Pass the existing ConsoleSink to the CsvWritter, this will output the csv to console
  quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writer{
    "orders.csv", quill::Frontend::get_sink("sink_id_1")};

  csv_writer.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
  csv_writer.append_row(132121123, "META", 300, 478.32321, "SELL");
  csv_writer.append_row(13212123, "AAPL", 120, 210.42321, "BUY");
}