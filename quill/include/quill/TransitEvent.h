/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/LogLevel.h"
#include "quill/detail/Serialize.h"
#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Common.h"

#include <atomic>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace quill
{

struct TransitEvent
{
  TransitEvent() { formatted_msg.reserve(64); }

  ~TransitEvent() = default;

  TransitEvent(TransitEvent const& other) = delete;
  TransitEvent& operator=(TransitEvent const& other) = delete;

  TransitEvent(TransitEvent&& other) noexcept
    : timestamp(other.timestamp),
      macro_metadata(other.macro_metadata),
      logger_details(other.logger_details),
      format_fn(other.format_fn),
      thread_id(other.thread_id),
      thread_name(other.thread_name),
      formatted_msg(std::move(other.formatted_msg)),
      structured_kvs(std::move(other.structured_kvs)),
      log_level_override(other.log_level_override),
      flush_flag(other.flush_flag)
  {
  }

  TransitEvent& operator=(TransitEvent&& other) noexcept
  {
    if (this != &other)
    {
      timestamp = other.timestamp;
      macro_metadata = other.macro_metadata;
      logger_details = other.logger_details;
      format_fn = other.format_fn;
      structured_kvs = std::move(other.structured_kvs);
      thread_id = other.thread_id;
      thread_name = other.thread_name;
      formatted_msg = std::move(other.formatted_msg);
      log_level_override = other.log_level_override;
      flush_flag = other.flush_flag;
    }

    return *this;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT LogLevel log_level() const noexcept
  {
    return log_level_override ? *log_level_override : macro_metadata->log_level();
  }

  /**
   * Need to take a copy of thread_id and thread_name here as the thread that logged can terminate
   * before we flush the backtrace.
   */
  uint64_t timestamp;
  MacroMetadata const* macro_metadata{nullptr};
  detail::LoggerDetails const* logger_details{nullptr};
  void* format_fn{nullptr};
  char const* thread_id;
  char const* thread_name;
  transit_event_fmt_buffer_t formatted_msg; /** buffer for message **/
  std::vector<std::pair<std::string, transit_event_fmt_buffer_t>> structured_kvs;
  std::optional<LogLevel> log_level_override{std::nullopt};
  std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
};
} // namespace quill