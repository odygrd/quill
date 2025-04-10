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
      named_args(std::move(other.named_args)),
      flush_flag(other.flush_flag),
      dynamic_log_level(other.dynamic_log_level)
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
      formatted_msg = std::move(other.formatted_msg);
      named_args = std::move(other.named_args);
      flush_flag = other.flush_flag;
      dynamic_log_level = other.dynamic_log_level;
    }

    return *this;
  }

  /***/
  void copy_to(TransitEvent& other) const
  {
    other.timestamp = timestamp;
    other.macro_metadata = macro_metadata;
    other.logger_base = logger_base;
    other.flush_flag = flush_flag;
    other.dynamic_log_level = dynamic_log_level;

    // manually copy the fmt::buffer
    other.formatted_msg->reserve(formatted_msg->size());
    other.formatted_msg->append(*formatted_msg);

    if (named_args)
    {
      other.named_args = std::make_unique<std::vector<std::pair<std::string, std::string>>>(*named_args);
    }
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT LogLevel log_level() const noexcept
  {
    if (macro_metadata->log_level() != LogLevel::Dynamic)
    {
      return macro_metadata->log_level();
    }
    else
    {
      return dynamic_log_level;
    }
  }

  uint64_t timestamp{0};
  MacroMetadata const* macro_metadata{nullptr};
  LoggerBase* logger_base{nullptr};
  std::unique_ptr<FormatBuffer> formatted_msg{std::make_unique<FormatBuffer>()}; /** buffer for message **/
  std::unique_ptr<std::vector<std::pair<std::string, std::string>>> named_args; /** A unique ptr to save space as named args feature is not always used */
  std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
  LogLevel dynamic_log_level{LogLevel::None};
};
} // namespace detail

QUILL_END_NAMESPACE