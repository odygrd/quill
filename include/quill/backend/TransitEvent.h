/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/FormatBuffer.h"
#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

QUILL_BEGIN_NAMESPACE

namespace detail
{
/** Forward declaration */
class LoggerBase;

/***/
struct TransitEvent
{
  /***/
  TransitEvent() { formatted_msg.reserve(32); }

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
      named_args(std::move(other.named_args)),
      dynamic_log_level(other.dynamic_log_level),
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
      named_args = std::move(other.named_args);
      dynamic_log_level = other.dynamic_log_level;
      flush_flag = other.flush_flag;
    }

    return *this;
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT LogLevel log_level() const noexcept
  {
    return (macro_metadata->log_level() != LogLevel::Dynamic) ? macro_metadata->log_level() : dynamic_log_level;
  }

  uint64_t timestamp{0};
  MacroMetadata const* macro_metadata{nullptr};
  detail::LoggerBase* logger_base{nullptr};
  detail::FormatArgsDecoder format_args_decoder{nullptr};
  std::string_view thread_id;
  std::string_view thread_name;
  FormatBuffer formatted_msg; /** buffer for message **/
  std::unique_ptr<std::vector<std::pair<std::string, std::string>>> named_args; /** A unique ptr to save space as named args feature is not always used */
  LogLevel dynamic_log_level{LogLevel::None};
  std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
};
} // namespace detail

QUILL_END_NAMESPACE