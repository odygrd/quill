/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/FormatBuffer.h"
#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace quill
{

namespace detail
{
// Forward declaration
class LoggerBase;
} // namespace detail

struct TransitEvent
{
  /***/
  TransitEvent() { formatted_msg.reserve(64); }

  /***/
  ~TransitEvent() = default;

  /***/
  TransitEvent(TransitEvent const& other) = delete;
  TransitEvent& operator=(TransitEvent const& other) = delete;

  /***/
  TransitEvent(TransitEvent&& other) noexcept
    : timestamp(other.timestamp),
      macro_metadata(other.macro_metadata),
      logger_base(other.logger_base),
      format_args_decoder(other.format_args_decoder),
      thread_id(other.thread_id),
      thread_name(other.thread_name),
      formatted_msg(std::move(other.formatted_msg)),
      structured_kvs(std::move(other.structured_kvs)),
      log_level_override(other.log_level_override),
      flush_flag(other.flush_flag)
  {
  }

  /***/
  TransitEvent& operator=(TransitEvent&& other) noexcept
  {
    if (this != &other)
    {
      timestamp = other.timestamp;
      macro_metadata = other.macro_metadata;
      logger_base = other.logger_base;
      format_args_decoder = other.format_args_decoder;
      thread_id = other.thread_id;
      thread_name = other.thread_name;
      formatted_msg = std::move(other.formatted_msg);
      structured_kvs = std::move(other.structured_kvs);
      log_level_override = other.log_level_override;
      flush_flag = other.flush_flag;
    }

    return *this;
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT LogLevel log_level() const noexcept
  {
    return log_level_override ? *log_level_override : macro_metadata->log_level();
  }

  uint64_t timestamp{0};
  MacroMetadata const* macro_metadata{nullptr};
  detail::LoggerBase const* logger_base{nullptr};
  void* format_args_decoder{nullptr};
  std::string_view thread_id;
  std::string_view thread_name;
  FormatBuffer formatted_msg; /** buffer for message **/
  std::unique_ptr<std::vector<std::pair<std::string, std::string>>> structured_kvs; /** A unique ptr to save space as structured logs feature is not always used */
  std::optional<LogLevel> log_level_override{std::nullopt};
  std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
};
} // namespace quill