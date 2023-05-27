/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/handlers/Handler.h"

namespace quill
{
class NullHandler : public Handler
{
public:
  NullHandler() { set_pattern(""); }

  ~NullHandler() override = default;

  QUILL_ATTRIBUTE_HOT void write(fmt_buffer_t const& formatted_log_message, quill::TransitEvent const& log_event) override
  {
  }
  QUILL_ATTRIBUTE_HOT void flush() noexcept override {}
};
} // namespace quill
