/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/sinks/LogRecord.h"

#ifndef QUILL_MODULE
  #include <string>
  #include <utility>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/**
 * Base filter class.
 * Filters can be added to Sinks
 */
class Filter
{
public:
  /**
   * Constructor
   * @param filter_name unique filter name
   */
  explicit Filter(std::string filter_name) : _filter_name(std::move(filter_name)) {}

  /**
   * Destructor
   */
  virtual ~Filter() = default;

  /**
   * @brief Filters a log record before a sink writes it. Called on the backend thread.
   * @param record the record, with log_statement formatted with the sink's pattern
   * @return true if the record should be written, false otherwise
   */
  QUILL_NODISCARD virtual bool filter(LogRecord const& record) noexcept = 0;

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

QUILL_END_EXPORT

QUILL_END_NAMESPACE