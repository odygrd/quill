/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/bundled/fmt/format.h"
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
  TransitEvent()
  { data = std::make_unique<TransitEventData>(); }

  /***/
  ~TransitEvent() = default;

  /***/
  TransitEvent(TransitEvent const& other) = delete;
  TransitEvent& operator=(TransitEvent const& other) = delete;

  /***/
  TransitEvent(TransitEvent&& other) noexcept
    : timestamp(other.timestamp), data(std::move(other.data))
  {
  }

  /***/
  TransitEvent& operator=(TransitEvent&& other) noexcept
  {
    if (this != &other)
    {
      timestamp = other.timestamp;
      data = std::move(other.data);
    }

    return *this;
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT LogLevel log_level() const noexcept
  {
    return (data->macro_metadata->log_level() != LogLevel::Dynamic) ? data->macro_metadata->log_level()
                                                                    : data->dynamic_log_level;
  }

  struct TransitEventData
  {
    MacroMetadata const* macro_metadata{nullptr};
    detail::LoggerBase* logger_base{nullptr};
    detail::FormatArgsDecoder format_args_decoder{nullptr};
    std::string_view thread_id;
    std::string_view thread_name;
    fmtquill::basic_memory_buffer<char, 64> formatted_msg; /** buffer for message **/
    std::unique_ptr<std::vector<std::pair<std::string, std::string>>> named_args; /** A unique ptr to save space as named args feature is not always used */
    LogLevel dynamic_log_level{LogLevel::None};
    std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
  };

  uint64_t timestamp{0};
  std::unique_ptr<TransitEventData> data;
};
} // namespace detail

QUILL_END_NAMESPACE