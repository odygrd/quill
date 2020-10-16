/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/detail/events/LogRecordMetadata.h"
#include "quill/detail/misc/Attributes.h"

namespace quill
{

/**
 * Base filter class.
 * Filters can be added to Handlers
 */
class FilterBase
{
public:
  /**
   * Constructor
   */
  FilterBase() = default;

  /**
   * Destructor
   */
  virtual ~FilterBase() = default;

  /**
   * Filters a log record
   * @param metadata log record
   * @param formatted_record formatted log record
   * @return true if the log record should be written to the file, false otherwise
   */
  QUILL_NODISCARD virtual bool filter(char const* thread_id, std::chrono::nanoseconds log_record_timestamp,
                                      detail::LogRecordMetadata const& metadata,
                                      fmt::memory_buffer const& formatted_record) noexcept = 0;
};
} // namespace quill