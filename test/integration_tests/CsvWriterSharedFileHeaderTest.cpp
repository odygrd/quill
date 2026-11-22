#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/CsvWriter.h"
#include "quill/Frontend.h"
#include "quill/core/FrontendOptions.h"

#include <string>
#include <vector>

using namespace quill;

struct SharedFileHeaderSchema
{
  static constexpr char const* header = "writer,id";
  static constexpr char const* format = "{},{}";
};

/***/
TEST_CASE("csv_writer_shared_file_header_is_written_once")
{
  static constexpr char const* filename_both = "csv_writer_shared_file_header_both.csv";
  static constexpr char const* filename_second = "csv_writer_shared_file_header_second.csv";
  static constexpr char const* filename_refused = "csv_writer_shared_file_header_refused.csv";
  static constexpr char const* filename_rotating = "csv_writer_shared_file_header_rotating.csv";
  static constexpr char const* filename_rotating_1 = "csv_writer_shared_file_header_rotating.1.csv";
  static constexpr char const* filename_rotating_second =
    "csv_writer_shared_file_header_rotating_second.csv";

  testing::remove_file(filename_both);
  testing::remove_file(filename_second);
  testing::remove_file(filename_refused);
  testing::remove_file(filename_rotating);
  testing::remove_file(filename_rotating_1);
  testing::remove_file(filename_rotating_second);

  Backend::start();

  {
    // Both writers ask for the header. The file sink that the first one creates writes it, and
    // the second one finds it in the file
    CsvWriter<SharedFileHeaderSchema, FrontendOptions> first_writer{filename_both};
    CsvWriter<SharedFileHeaderSchema, FrontendOptions> second_writer{filename_both};

    first_writer.append_row("first", 1);
    first_writer.flush();
    second_writer.append_row("second", 2);

    first_writer.close();
    second_writer.close();
  }

  {
    // The writer that writes the header creates the file sink. The other one shares it
    FileSinkConfig sink_config;
    sink_config.set_open_mode('w');

    CsvWriter<SharedFileHeaderSchema, FrontendOptions> first_writer{filename_second, sink_config, true};
    CsvWriter<SharedFileHeaderSchema, FrontendOptions> second_writer{filename_second, sink_config, false};

    first_writer.append_row("first", 1);
    first_writer.flush();
    second_writer.append_row("second", 2);

    first_writer.close();
    second_writer.close();
  }

#if !defined(QUILL_NO_EXCEPTIONS)
  {
    // The other way around, the sink that the first writer created writes no header: not to this
    // file, and not to one that it opens later, e.g. when this one is deleted. The writer that
    // asks for the header is refused instead of logging it once
    FileSinkConfig sink_config;
    sink_config.set_open_mode('w');

    using Writer = CsvWriter<SharedFileHeaderSchema, FrontendOptions>;
    Writer first_writer{filename_refused, sink_config, false};
    REQUIRE_THROWS_AS(Writer(filename_refused, sink_config, true), QuillError);

    first_writer.close();
  }
#endif

  {
    // The files that a rotating sink rotates to start with the header when the writer that asks
    // for it creates the sink
    RotatingFileSinkConfig sink_config;
    sink_config.set_open_mode('w');
    sink_config.set_rotation_max_file_size(512);

    CsvWriter<SharedFileHeaderSchema, FrontendOptions> first_writer{filename_rotating, sink_config, true};
    CsvWriter<SharedFileHeaderSchema, FrontendOptions> second_writer{filename_rotating, sink_config, false};

    first_writer.append_row(std::string(300, 'a'), 1);
    first_writer.flush();
    second_writer.append_row(std::string(300, 'b'), 2);

    first_writer.close();
    second_writer.close();
  }

#if !defined(QUILL_NO_EXCEPTIONS)
  {
    // The other way around, the sink would write no header to the files it rotates to
    RotatingFileSinkConfig sink_config;
    sink_config.set_open_mode('w');
    sink_config.set_rotation_max_file_size(512);

    using Writer = CsvWriter<SharedFileHeaderSchema, FrontendOptions>;
    Writer first_writer{filename_rotating_second, sink_config, false};
    REQUIRE_THROWS_AS(Writer(filename_rotating_second, sink_config, true), QuillError);

    // Also when the first writer has written a row to the file by then
    first_writer.append_row("first", 1);
    first_writer.flush();
    REQUIRE_THROWS_AS(Writer(filename_rotating_second, sink_config, true), QuillError);

    // And through the constructor that is given only the name: it gets the rotating sink too
    REQUIRE_THROWS_AS(Writer{filename_rotating_second}, QuillError);

    first_writer.close();
  }
#endif

  // Wait until the backend thread stops for test stability
  Backend::stop();

  std::vector<std::string> const expected{"writer,id", "first,1", "second,2"};
  REQUIRE(testing::file_contents(filename_both) == expected);
  REQUIRE(testing::file_contents(filename_second) == expected);

  std::vector<std::string> const rotated_contents = testing::file_contents(filename_rotating_1);
  REQUIRE_EQ(rotated_contents.size(), 2);
  REQUIRE_EQ(rotated_contents[0], "writer,id");
  REQUIRE_EQ(rotated_contents[1], std::string(300, 'a') + ",1");

  std::vector<std::string> const rotating_contents = testing::file_contents(filename_rotating);
  REQUIRE_EQ(rotating_contents.size(), 2);
  REQUIRE_EQ(rotating_contents[0], "writer,id");
  REQUIRE_EQ(rotating_contents[1], std::string(300, 'b') + ",2");

  testing::remove_file(filename_both);
  testing::remove_file(filename_second);
  testing::remove_file(filename_refused);
  testing::remove_file(filename_rotating);
  testing::remove_file(filename_rotating_1);
  testing::remove_file(filename_rotating_second);
}
