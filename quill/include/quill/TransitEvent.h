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
    : header(other.header),
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
      structured_kvs = std::move(other.structured_kvs);
      thread_id = other.thread_id;
      thread_name = other.thread_name;
      header = other.header;
      formatted_msg = std::move(other.formatted_msg);
      log_level_override = other.log_level_override;
      flush_flag = other.flush_flag;
    }

    return *this;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT LogLevel log_level() const noexcept
  {
    return log_level_override ? *log_level_override : header.metadata_and_format_fn().first.level();
  }

  /**
   * @return  The log level of this logging event as a string
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::string_view log_level_as_str() const noexcept
  {
    return loglevel_to_string(log_level());
  }

  /**
   * @return The metadata of this logging event
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT MacroMetadata metadata() const noexcept
  {
    return header.metadata_and_format_fn().first;
  }

  /**
   * Need to take a copy of thread_id and thread_name here as the thread that logged can terminate
   * before we flush the backtrace.
   */
  detail::Header header;
  char const* thread_id;
  char const* thread_name;
  transit_event_fmt_buffer_t formatted_msg; /** buffer for message **/
  std::vector<std::pair<std::string, std::string>> structured_kvs;
  std::optional<LogLevel> log_level_override{std::nullopt};
  std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
};
} // namespace quill