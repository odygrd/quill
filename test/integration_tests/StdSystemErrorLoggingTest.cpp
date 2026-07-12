#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/SystemError.h"
#include "quill/std/Vector.h"

#include <cstdio>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace quill;

namespace
{
class TemporaryErrorCategory final : public std::error_category
{
public:
  char const* name() const noexcept override { return "temporary_error_category"; }

  std::string message(int value) const override
  {
    return "temporary error message " + std::to_string(value);
  }
};

class StatefulErrorCategory final : public std::error_category
{
public:
  StatefulErrorCategory(std::string first_name, std::string later_name, std::string first_message,
                        std::string later_message)
    : _first_name(std::move(first_name)),
      _later_name(std::move(later_name)),
      _first_message(std::move(first_message)),
      _later_message(std::move(later_message))
  {
  }

  char const* name() const noexcept override
  {
    return ((_name_calls++ == 0u) ? _first_name : _later_name).c_str();
  }

  std::string message(int) const override
  {
    return (_message_calls++ == 0u) ? _first_message : _later_message;
  }

  size_t name_calls() const noexcept { return _name_calls; }
  size_t message_calls() const noexcept { return _message_calls; }

private:
  std::string _first_name;
  std::string _later_name;
  std::string _first_message;
  std::string _later_message;
  mutable size_t _name_calls{0};
  mutable size_t _message_calls{0};
};

void log_error_code_with_temporary_category(Logger* logger)
{
  TemporaryErrorCategory category;
  std::error_code const error_code_value{42, category};

  LOG_INFO(logger, "custom_error_code {}", error_code_value);
  LOG_INFO(logger, "custom_error_code_message {:s}", error_code_value);
}
} // namespace

/***/
TEST_CASE("std_system_error_logging")
{
  static constexpr char const* filename = "std_system_error_logging.log";
  static std::string const logger_name = "std_system_error_logger";

  // Regression: reserve and encode must use one exact snapshot. Before the fix the second
  // virtual calls wrote a much larger payload than was reserved, corrupting the next record.
  StatefulErrorCategory codec_category{"first_category", "later_category_name_is_longer",
                                       std::string(21, 'a'), std::string(520, 'b')};
  std::error_code const codec_error{7, codec_category};
  detail::SizeCacheVector size_cache;
  size_t const reserved_size = Codec<std::error_code>::compute_encoded_size(size_cache, codec_error);

  static constexpr size_t guard_size = 1024;
  static constexpr std::byte guard_value{0x5a};
  std::vector<std::byte> encoded_storage(reserved_size + guard_size, guard_value);
  std::byte* encode_pos = encoded_storage.data();
  uint32_t cache_index{0};
  Codec<std::error_code>::encode(encode_pos, size_cache, cache_index, codec_error);

  REQUIRE_EQ(codec_category.name_calls(), 1u);
  REQUIRE_EQ(codec_category.message_calls(), 1u);
  REQUIRE_EQ(static_cast<size_t>(encode_pos - encoded_storage.data()), reserved_size);
  REQUIRE_EQ(cache_index, size_cache.size());

  std::byte* decode_pos = encoded_storage.data();
  detail::DecodedErrorCode const decoded_error = Codec<std::error_code>::decode_arg(decode_pos);
  REQUIRE_EQ(decoded_error.category_name(), "first_category");
  REQUIRE_EQ(decoded_error.message(), std::string(21, 'a'));
  REQUIRE_EQ(static_cast<size_t>(decode_pos - encoded_storage.data()), reserved_size);

  bool guard_intact{true};
  for (size_t i = reserved_size; i < encoded_storage.size(); ++i)
  {
    guard_intact = guard_intact && (encoded_storage[i] == guard_value);
  }
  REQUIRE(guard_intact);

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(BackendOptions{});

  Frontend::preallocate();

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  std::error_code const error_code_value = std::make_error_code(std::errc::permission_denied);
  std::error_code const default_error_code{};

  LOG_INFO(logger, "error_code {}", error_code_value);
  LOG_INFO(logger, "error_code_message {:s}", error_code_value);
  LOG_INFO(logger, "error_code_debug {:?}", error_code_value);
  LOG_INFO(logger, "default_error_code {}", default_error_code);
  LOG_INFO(logger, "temp_error_code {}", std::make_error_code(std::errc::io_error));
  log_error_code_with_temporary_category(logger);

  StatefulErrorCategory message_category{"message_category", "changed_message_category",
                                         std::string(8, 'a'), std::string(64, 'b')};
  std::error_code const stateful_message_error_code{8, message_category};
  LOG_INFO(logger, "stateful_message {:s}", stateful_message_error_code);
  REQUIRE_EQ(message_category.name_calls(), 1u);
  REQUIRE_EQ(message_category.message_calls(), 1u);

  StatefulErrorCategory name_category{"first_name", "a_much_longer_second_category_name",
                                      "first message", "changed message"};
  std::error_code const stateful_name_error_code{9, name_category};
  LOG_INFO(logger, "stateful_name {}", stateful_name_error_code);
  REQUIRE_EQ(name_category.name_calls(), 1u);
  REQUIRE_EQ(name_category.message_calls(), 1u);

  LOG_INFO(logger, "after_stateful_errors still intact {}", 12345);

  // container elements consume two size-cache entries each; the sequencing must stay consistent
  // between compute_encoded_size() and encode()
  std::vector<std::error_code> const error_code_vector{
    std::make_error_code(std::errc::io_error), std::make_error_code(std::errc::permission_denied)};
  LOG_INFO(logger, "error_code_vector {}", error_code_vector);

  manual_backend_worker->poll();
  Frontend::remove_logger(logger);
  manual_backend_worker->poll();
  manual_backend_worker->shutdown();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  std::string const error_code_display =
    std::string{error_code_value.category().name()} + ":" + std::to_string(error_code_value.value());
  std::error_code const temp_error_code = std::make_error_code(std::errc::io_error);
  std::string const temp_error_code_display =
    std::string{temp_error_code.category().name()} + ":" + std::to_string(temp_error_code.value());
  std::string const default_error_code_display = std::string{default_error_code.category().name()} +
    ":" + std::to_string(default_error_code.value());

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " error_code " + error_code_display));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " error_code_message " + error_code_value.message()));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " error_code_debug " + fmtquill::format("{:?}", error_code_display)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " default_error_code " + default_error_code_display));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " temp_error_code " + temp_error_code_display));
  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " custom_error_code temporary_error_category:42"));
  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " custom_error_code_message temporary error message 42"));

  REQUIRE(quill::testing::file_contains(file_contents,
                                        logger_name + " stateful_message " + std::string(8, 'a')));
  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " stateful_name first_name:9"));

  // The statement after the stateful categories must retain intact queue framing.
  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " after_stateful_errors still intact 12345"));

  std::error_code const io_error_code = std::make_error_code(std::errc::io_error);
  std::string const error_code_vector_display = "[\"" + std::string{io_error_code.category().name()} +
    ":" + std::to_string(io_error_code.value()) + "\", \"" + error_code_display + "\"]";
  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " error_code_vector " + error_code_vector_display));

  testing::remove_file(filename);
}
