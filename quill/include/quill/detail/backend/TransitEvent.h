/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/Serialize.h"
#include "quill/detail/ThreadContext.h"
#include "quill/detail/misc/Common.h"

namespace quill
{
namespace detail
{
struct TransitEvent
{
  TransitEvent(ThreadContext* thread_context, detail::Header header,
               FormatFnMemoryBuffer formatted_msg, std::atomic<bool>* flush_flag)
    : thread_id(thread_context->thread_id()),
      thread_name(thread_context->thread_name()),
      header(header),
      formatted_msg(std::move(formatted_msg)),
      flush_flag(flush_flag)
  {
  }

  TransitEvent(TransitEvent const& other)
    : thread_id(other.thread_id),
      thread_name(other.thread_name),
      header(other.header),
      flush_flag(other.flush_flag)

  {
    formatted_msg.clear();
    formatted_msg.append(other.formatted_msg.begin(), other.formatted_msg.end());
  }

  TransitEvent& operator=(TransitEvent const& other)
  {
    if (this != &other)
    {
      thread_id = other.thread_id;
      thread_name = other.thread_name;
      header = other.header;
      formatted_msg.clear();
      formatted_msg.append(other.formatted_msg.begin(), other.formatted_msg.end());
      flush_flag = other.flush_flag;
    }

    return *this;
  }

  TransitEvent(TransitEvent&& other) noexcept
    : thread_id(std::move(other.thread_id)),
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
      thread_id = std::move(other.thread_id);
      thread_name = std::move(other.thread_name);
      header = other.header;
      formatted_msg = std::move(other.formatted_msg);
      flush_flag = other.flush_flag;
    }

    return *this;
  }

  friend bool operator>(TransitEvent const& lhs, TransitEvent const& rhs)
  {
    return lhs.header.timestamp > rhs.header.timestamp;
  }

  /**
   * Need to take a copy of thread_id and thread_name here as the thread that logged can terminate
   * before we flush the backtrace.
   */
  std::string thread_id;
  std::string thread_name;
  detail::Header header;
  FormatFnMemoryBuffer formatted_msg;     /** buffer for message **/
  std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
};
} // namespace detail
} // namespace quill