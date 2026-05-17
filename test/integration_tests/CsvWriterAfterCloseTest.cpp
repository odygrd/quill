#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/CsvWriter.h"
#include "quill/core/FrontendOptions.h"

using namespace quill;

struct OrderCsvSchema
{
  static constexpr char const* header = "order_id,symbol,quantity,price,side";
  static constexpr char const* format = "{},{},{},{:.2f},{}";
};

/***/
TEST_CASE("csv_writer_operations_throw_after_close")
{
  static constexpr char const* filename = "csv_writer_operations_throw_after_close.csv";

  Backend::start();

  quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writer{filename};
  csv_writer.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
  csv_writer.close();

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(csv_writer.append_row(13212124, "META", 200, 310.32321, "SELL"), QuillError);
  REQUIRE_THROWS_AS(csv_writer.write_header(), QuillError);
  REQUIRE_THROWS_AS(csv_writer.flush(), QuillError);
#endif

  Backend::stop();

  testing::remove_file(filename);
}
