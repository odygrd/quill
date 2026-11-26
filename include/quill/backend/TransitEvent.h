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

#ifndef QUILL_MODULE
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
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/** Forward declaration */
class LoggerBase;

/***/
struct TransitEvent
{
  struct RuntimeMetadata;

  /***/
  TransitEvent() = default;

  /***/
  ~TransitEvent() = default;

  /**
   * Copy constructor and assignment operator are deleted to prevent accidental copying.
   */
  TransitEvent(TransitEvent const& other) = delete;
  TransitEvent& operator=(TransitEvent const& other) = delete;

  /**
   * Moves keep macro_metadata valid: runtime metadata lives in the heap-allocated ExtraData,
   * whose address does not change when the unique_ptr is moved.
   */
  TransitEvent(TransitEvent&& other) noexcept = default;
  TransitEvent& operator=(TransitEvent&& other) noexcept = default;

  /**
   * Copies the event to `other`, except for its formatted message: `other` refers to the message
   * of this event, which is released with this event. A copy that is kept for longer needs a
   * message of its own, see BacktraceStorage::store().
   */
  void copy_to(TransitEvent& other) const
  {
    other.timestamp = timestamp;
    other.macro_metadata = macro_metadata;
    other.logger_base = logger_base;
    other.formatted_msg = formatted_msg;
    other.event_payload = event_payload;

    if (extra_data)
    {
      other.extra_data = std::make_unique<ExtraData>(*extra_data);

      if (extra_data->runtime_metadata && (macro_metadata == &extra_data->runtime_metadata->macro_metadata))
      {
        // then point macro_metadata to the correct object
        other.macro_metadata = &other.extra_data->runtime_metadata->macro_metadata;
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

  /** @return the runtime metadata of this event, allocated for the first event of this slot that has one */
  QUILL_ATTRIBUTE_HOT RuntimeMetadata& ensure_runtime_metadata()
  {
    ensure_extra_data();

    if (!extra_data->runtime_metadata)
    {
      extra_data->runtime_metadata = std::make_unique<RuntimeMetadata>();
    }

    return *extra_data->runtime_metadata;
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
      // Not skipped when the slot already holds the same metadata: an application interleaves
      // many log statements, so a slot rarely gets the same one twice in a row
      fmt.assign(_safe_string(in_fmt));
      _set_source_location(file, line);
      function_name.assign(_safe_string(function));
      tags.assign(_safe_string(in_tags));

      // The lengths of the strings are known, so the metadata does not measure them again
      macro_metadata =
        MacroMetadata(std::string_view{source_location}, function_name.data(), std::string_view{fmt},
                      tags.empty() ? nullptr : tags.data(), log_level, MacroMetadata::Event::Log);
    }

    RuntimeMetadata(RuntimeMetadata const& other)
      : fmt(other.fmt),
        source_location(other.source_location),
        function_name(other.function_name),
        tags(other.tags),
        macro_metadata(std::string_view{source_location}, function_name.data(),
                       std::string_view{fmt}, tags.empty() ? nullptr : tags.data(),
                       other.macro_metadata.log_level(), MacroMetadata::Event::Log)
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

        // Recreate macro_metadata to point to our own strings
        macro_metadata = MacroMetadata(std::string_view{source_location}, function_name.data(),
                                       std::string_view{fmt}, tags.empty() ? nullptr : tags.data(),
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

  private:
    QUILL_NODISCARD static char const* _safe_string(char const* value) noexcept
    {
      return value ? value : "";
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
    ExtraData() = default;

    ExtraData(ExtraData const& other)
      : named_args(other.named_args),
        mdc(other.mdc),
        runtime_metadata(other.runtime_metadata ? std::make_unique<RuntimeMetadata>(*other.runtime_metadata) : nullptr)
    {
    }

    ExtraData& operator=(ExtraData const& other) = delete;

    // Additional fields that are used for some features as a separate structure to keep
    // TransitEvent size smaller for the main scenarios
    std::vector<std::pair<std::string, std::string>> named_args;
    std::string mdc;

    /**
     * Allocated by ensure_runtime_metadata(), so the extra data of the other events, e.g. with
     * named args, takes 64 bytes instead of several cache lines
     */
    std::unique_ptr<RuntimeMetadata> runtime_metadata;
  };

  uint64_t timestamp{0};
  MacroMetadata const* macro_metadata{nullptr};
  LoggerBase* logger_base{nullptr};

  /**
   * The formatted message. An event does not own it: it is stored in the MessageArena of the
   * TransitEventBuffer or BacktraceStorage that holds the event, and released with the event.
   */
  std::string_view formatted_msg;
  std::unique_ptr<ExtraData> extra_data; /** A unique ptr to save space as these fields not always used */
  std::variant<std::monostate, std::atomic<bool>*, double> event_payload{
    std::monostate{}}; /** This is only used in the case of Event::Flush or Event::Metric **/
};
} // namespace detail

QUILL_END_NAMESPACE
