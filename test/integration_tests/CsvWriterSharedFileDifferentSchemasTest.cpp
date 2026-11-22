#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/CsvWriter.h"
#include "quill/Frontend.h"
#include "quill/core/FrontendOptions.h"

#include <cstddef>
#include <string>
#include <vector>

using namespace quill;

struct SharedFileOrderSchema
{
  static constexpr char const* header = "kind,id,price";
  static constexpr char const* format = "order,{},{:.2f}";
};

struct SharedFileCancelSchema
{
  static constexpr char const* header = "kind,id,price";
  static constexpr char const* format = "cancel,{},";
};

/***/
TEST_CASE("csv_writer_shared_file_different_schemas")
{
  static constexpr char const* filename = "csv_writer_shared_file_different_schemas.csv";

  // The writer appends by default, so the file of a run that did not finish is removed first
  testing::remove_file(filename);

  Backend::start();

  quill::CsvWriter<SharedFileOrderSchema, quill::FrontendOptions> order_writer{filename};

  quill::CsvWriter<SharedFileCancelSchema, quill::FrontendOptions> cancel_writer{filename,
                                                                                 []()
                                                                                 {
                                                                                   FileSinkConfig cfg;
                                                                                   cfg.set_open_mode('a');
                                                                                   return cfg;
                                                                                 }(),
                                                                                 false};

  // Each writer owns a logger, even though their schema types differ
  size_t csv_logger_count{0};
  for (auto* logger : Frontend::get_all_loggers())
  {
    if (logger->get_logger_name().rfind("__csv__", 0) == 0)
    {
      ++csv_logger_count;
    }
  }
  REQUIRE_EQ(csv_logger_count, 2u);

  order_writer.append_row(1, 101.5);
  order_writer.close();

  // Closing one writer must not remove the logger of the other
  cancel_writer.append_row(1);
  cancel_writer.flush();

  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 3);
  REQUIRE_EQ(file_contents[0], "kind,id,price");
  REQUIRE_EQ(file_contents[1], "order,1,101.50");
  REQUIRE_EQ(file_contents[2], "cancel,1,");

  testing::remove_file(filename);
}
