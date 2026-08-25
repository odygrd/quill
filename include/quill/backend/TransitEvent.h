/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"

#include "quill/bundled/fmt/format.h"

#include <atomic>
#include <charconv>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

QUILL_BEGIN_NAMESPACE

namespace detail
{
/** Forward declaration */
class LoggerBase;

/***/
struct TransitEvent
{
  using FormatBuffer = fmtquill::basic_memory_buffer<char, 88>;

  /***/
  TransitEvent() = default;

  /***/
  ~TransitEvent() = default;

  /**
   * Copy constructor and assignment operator are deleted to prevent accidental copying.
   */
  TransitEvent(TransitEvent const& other) = delete;
  TransitEvent& operator=(TransitEvent const& other) = delete;

  /***/
  TransitEvent(TransitEvent&& other) noexcept
    : timestamp(other.timestamp),
      macro_metadata(other.macro_metadata),
      logger_base(other.logger_base),
      formatted_msg(std::move(other.formatted_msg)),
      extra_data(std::move(other.extra_data)),
      event_payload(std::move(other.event_payload))
  {
    other.event_payload = std::monostate{};

    // Update macro_metadata pointer if it was pointing to runtime_metadata
    if (extra_data && extra_data->runtime_metadata.has_runtime_metadata)
    {
      // Point to our own runtime_metadata instead of the moved-from object's
      macro_metadata = &extra_data->runtime_metadata.macro_metadata;
    }
  }

  /***/
  TransitEvent& operator=(TransitEvent&& other) noexcept
  {
    if (this != &other)
    {
      timestamp = other.timestamp;
      macro_metadata = other.macro_metadata;
      logger_base = other.logger_base;
      formatted_msg = std::move(other.formatted_msg);
      extra_data = std::move(other.extra_data);
      event_payload = std::move(other.event_payload);
      other.event_payload = std::monostate{};

      if (extra_data && extra_data->runtime_metadata.has_runtime_metadata)
      {
        // Point to our own runtime_metadata instead of the moved-from object's
        macro_metadata = &extra_data->runtime_metadata.macro_metadata;
      }
    }

    return *this;
  }

  /***/
  void copy_to(TransitEvent& other) const
  {
    other.timestamp = timestamp;
    other.macro_metadata = macro_metadata;
    other.logger_base = logger_base;
    other.event_payload = event_payload;

    // manually copy the fmt::buffer
    other.formatted_msg->clear();
    other.formatted_msg->reserve(formatted_msg->size());
    other.formatted_msg->append(*formatted_msg);

    if (extra_data)
    {
      other.extra_data = std::make_unique<ExtraData>(*extra_data);

      if (other.extra_data->runtime_metadata.has_runtime_metadata)
      {
        // then point macro_metadata to the correct object
        other.macro_metadata = &other.extra_data->runtime_metadata.macro_metadata;
      }
    }
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT LogLevel log_level() const noexcept
  {
    return macro_metadata->log_level();
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::vector<std::pair<std::string, std::string>>* get_named_args() const noexcept
  {
    // TransitEvents are pooled, so `extra_data` may have been allocated by a previous log on
    // this slot
    if (!extra_data || !macro_metadata || !macro_metadata->has_named_args())
    {
      return nullptr;
    }

    return &extra_data->named_args;
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::string_view mdc() const noexcept
  {
    return extra_data ? extra_data->mdc : std::string_view{};
  }

  /***/
  QUILL_ATTRIBUTE_HOT void ensure_extra_data()
  {
    if (extra_data)
    {
      return;
    }

    extra_data = std::make_unique<ExtraData>();
  }

  QUILL_ATTRIBUTE_HOT void set_flush_flag(std::atomic<bool>* flush_flag_ptr) noexcept
  {
    event_payload = flush_flag_ptr;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::atomic<bool>* flush_flag() const noexcept
  {
    QUILL_ASSERT(std::holds_alternative<std::atomic<bool>*>(event_payload),
                 "Attempted to read a flush flag from a TransitEvent with a non-flush payload");
    std::atomic<bool>* const flush_flag_ptr = std::get<std::atomic<bool>*>(event_payload);
    QUILL_ASSERT(flush_flag_ptr != nullptr,
                 "Attempted to read a null flush flag from a TransitEvent");
    return flush_flag_ptr;
  }

  QUILL_ATTRIBUTE_HOT void set_metric_value(double value) noexcept { event_payload = value; }

  QUILL_ATTRIBUTE_HOT void reset_payload() noexcept { event_payload = std::monostate{}; }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool has_metric_value() const noexcept
  {
    return std::holds_alternative<double>(event_payload);
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT double metric_value() const noexcept
  {
    QUILL_ASSERT(has_metric_value(),
                 "Attempted to read a metric value from a TransitEvent without one");
    return std::get<double>(event_payload);
  }

  struct RuntimeMetadata
  {
    RuntimeMetadata() = default;

    RuntimeMetadata(char const* file, uint32_t line, char const* function, char const* in_tags,
                    char const* in_fmt, LogLevel log_level)
    {
      set(file, line, function, in_tags, in_fmt, log_level);
    }

    QUILL_ATTRIBUTE_HOT void set(char const* file, uint32_t line, char const* function,
                                 char const* in_tags, char const* in_fmt, LogLevel log_level)
    {
      if ((macro_metadata.event() == MacroMetadata::Event::Log) && (macro_metadata.log_level() == log_level) &&
          (fmt == _safe_string(in_fmt)) && (function_name == _safe_string(function)) &&
          (tags == _safe_string(in_tags)) && _source_location_matches(file, line))
      {
        has_runtime_metadata = true;
        return;
      }

      fmt.assign(_safe_string(in_fmt));
      line_number = line;
      _set_source_location(file, line);
      function_name.assign(_safe_string(function));
      tags.assign(_safe_string(in_tags));

      macro_metadata =
        MacroMetadata(source_location.data(), function_name.data(), fmt.data(),
                      tags.empty() ? nullptr : tags.data(), log_level, MacroMetadata::Event::Log);
      has_runtime_metadata = true;
    }

    RuntimeMetadata(RuntimeMetadata const& other)
      : fmt(other.fmt),
        source_location(other.source_location),
        function_name(other.function_name),
        tags(other.tags),
        macro_metadata(source_location.data(), function_name.data(), fmt.data(),
                       tags.empty() ? nullptr : tags.data(), other.macro_metadata.log_level(),
                       MacroMetadata::Event::Log),
        line_number(other.line_number),
        has_runtime_metadata(other.has_runtime_metadata)
    {
      // Recreate macro_metadata to point to our own strings
    }

    RuntimeMetadata& operator=(RuntimeMetadata const& other)
    {
      if (this != &other)
      {
        fmt = other.fmt;
        source_location = other.source_location;
        function_name = other.function_name;
        tags = other.tags;
        line_number = other.line_number;
        has_runtime_metadata = other.has_runtime_metadata;

        // Recreate macro_metadata to point to our own strings
        macro_metadata = MacroMetadata(source_location.data(), function_name.data(), fmt.data(),
                                       tags.empty() ? nullptr : tags.data(),
                                       other.macro_metadata.log_level(), MacroMetadata::Event::Log);
      }
      return *this;
    }

    RuntimeMetadata(RuntimeMetadata&& other) noexcept = delete;
    RuntimeMetadata& operator=(RuntimeMetadata&& other) noexcept = delete;

    std::string fmt;
    std::string source_location;
    std::string function_name;
    std::string tags;
    MacroMetadata macro_metadata;
    uint32_t line_number{0};
    bool has_runtime_metadata{false};

  private:
    QUILL_NODISCARD static char const* _safe_string(char const* value) noexcept
    {
      return value ? value : "";
    }

    QUILL_NODISCARD bool _source_location_matches(char const* file, uint32_t line) const noexcept
    {
      if (!file || (file[0] == '\0' && line == 0))
      {
        return source_location.empty();
      }

      if (line_number != line)
      {
        return false;
      }

      std::string_view file_path{file};
      constexpr size_t max_file_path_length{(std::numeric_limits<uint16_t>::max)()};
      if (QUILL_UNLIKELY(file_path.size() > max_file_path_length))
      {
        file_path.remove_prefix(file_path.size() - max_file_path_length);
      }

      return macro_metadata.full_path() == file_path;
    }

    void _set_source_location(char const* file, uint32_t line)
    {
      if (!file || (file[0] == '\0' && line == 0))
      {
        source_location.clear();
        return;
      }

      std::string_view file_path{file};
      constexpr size_t max_file_path_length{(std::numeric_limits<uint16_t>::max)()};

      // MacroMetadata stores source-location offsets as uint16_t. Keep the path suffix so the
      // filename and line offsets remain representable while preserving the most useful part.
      if (QUILL_UNLIKELY(file_path.size() > max_file_path_length))
      {
        file_path.remove_prefix(file_path.size() - max_file_path_length);
      }

      source_location.assign(file_path);
      source_location.push_back(':');

      char line_buffer[std::numeric_limits<uint32_t>::digits10 + 1];
      auto const result = std::to_chars(line_buffer, line_buffer + sizeof(line_buffer), line);
      source_location.append(line_buffer, result.ptr);
    }
  };

  struct ExtraData
  {
    // Additional fields that are used for some features as a separate structure to keep
    // TransitEvent size smaller for the main scenarios
    std::vector<std::pair<std::string, std::string>> named_args;
    std::string mdc;
    RuntimeMetadata runtime_metadata;
  };

  uint64_t timestamp{0};
  MacroMetadata const* macro_metadata{nullptr};
  LoggerBase* logger_base{nullptr};
  std::unique_ptr<FormatBuffer> formatted_msg{std::make_unique<FormatBuffer>()}; /** buffer for message **/
  std::unique_ptr<ExtraData> extra_data; /** A unique ptr to save space as these fields not always used */
  std::variant<std::monostate, std::atomic<bool>*, double> event_payload{
    std::monostate{}}; /** This is only used in the case of Event::Flush or Event::Metric **/
};
} // namespace detail

QUILL_END_NAMESPACE
