/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/detail/Serialize.h"

namespace quill
{

struct TransitEvent
{
  TransitEvent() = default;
  ~TransitEvent() = default;

  TransitEvent(TransitEvent const& other) = delete;
  TransitEvent& operator=(TransitEvent const& other) = delete;

  TransitEvent(TransitEvent&& other) noexcept
    : structured_keys(std::move(other.structured_keys)),
      structured_values(std::move(other.structured_values)),
      thread_id(std::move(other.thread_id)),
      thread_name(std::move(other.thread_name)),
      header(other.header),
      formatted_msg(std::move(other.formatted_msg)),
      flush_flag(other.flush_flag)
  {
  }

  TransitEvent& operator=(TransitEvent&& other) noexcept
  {
    if (this != &other)
    {
      structured_keys = std::move(other.structured_keys);
      structured_values = std::move(other.structured_values);
      thread_id = std::move(other.thread_id);
      thread_name = std::move(other.thread_name);
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
  std::vector<std::string> structured_keys;
  std::vector<std::string> structured_values;
  std::string thread_id;
  std::string thread_name;
  detail::Header header;
  fmt_buffer_t formatted_msg;             /** buffer for message **/
  uint32_t custom_clock_seq{0};           /**< used only when there is a custom clock as seq num */
  std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
};

class TransitEventComparator
{
public:
  bool operator()(std::pair<uint64_t, TransitEvent*> const& lhs, std::pair<uint64_t, TransitEvent*> const& rhs)
  {
    if (lhs.first == rhs.first)
    {
      // when the timestamps are equal, compare the custom_clock_seq
      return lhs.second->custom_clock_seq > rhs.second->custom_clock_seq;
    }
    return lhs.first > rhs.first;
  }
};
} // namespace quill