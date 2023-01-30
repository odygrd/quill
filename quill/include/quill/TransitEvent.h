/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/detail/Serialize.h"

#include <string_view>

namespace quill
{

struct TransitEvent
{
  TransitEvent() = default;
  ~TransitEvent() = default;

  TransitEvent(TransitEvent const& other) = delete;
  TransitEvent& operator=(TransitEvent const& other) = delete;

  TransitEvent(TransitEvent&& other) noexcept
    : header(other.header),
      thread_id(other.thread_id),
      thread_name(other.thread_name),
      formatted_msg(std::move(other.formatted_msg)),
      structured_keys(std::move(other.structured_keys)),
      structured_values(std::move(other.structured_values)),
      flush_flag(other.flush_flag)
  {
  }

  TransitEvent& operator=(TransitEvent&& other) noexcept
  {
    if (this != &other)
    {
      structured_keys = std::move(other.structured_keys);
      structured_values = std::move(other.structured_values);
      thread_id = other.thread_id;
      thread_name = other.thread_name;
      header = other.header;
      formatted_msg = std::move(other.formatted_msg);
      flush_flag = other.flush_flag;
    }

    return *this;
  }

  /**
   * Need to take a copy of thread_id and thread_name here as the thread that logged can terminate
   * before we flush the backtrace.
   */
  detail::Header header;
  char const* thread_id;
  char const* thread_name;
  fmt_buffer_t formatted_msg; /** buffer for message **/
  std::vector<std::string> structured_keys;
  std::vector<std::string> structured_values;
  std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
};
} // namespace quill