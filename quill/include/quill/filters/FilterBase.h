/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/MacroMetadata.h"
#include "quill/detail/misc/Attributes.h"
#include <string>

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
   * @param filter_name unique filter name
   */
  explicit FilterBase(std::string filter_name) : _filter_name(std::move(filter_name)){};

  /**
   * Destructor
   */
  virtual ~FilterBase() = default;

  /**
   * Filters a log message
   * @param metadata log message
   * @param formatted_record formatted log message
   * @return true if the log message should be written to the file, false otherwise
   */
  QUILL_NODISCARD virtual bool filter(char const* thread_id, std::chrono::nanoseconds log_message_timestamp,
                                      MacroMetadata const& metadata,
                                      fmt::memory_buffer const& formatted_record) noexcept = 0;

  /**
   * Gets the name of the filter. Only useful if an existing filter is needed to be looked up
   * @return the name of the filter
   */
  QUILL_NODISCARD virtual std::string const& get_filter_name() const noexcept
  {
    return _filter_name;
  }

private:
  std::string _filter_name;
};
} // namespace quill