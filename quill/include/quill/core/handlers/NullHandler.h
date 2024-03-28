/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/handlers/Handler.h"

namespace quill
{
class NullHandler : public Handler
{
public:
  NullHandler() { set_pattern(""); }

  ~NullHandler() override = default;

    QUILL_ATTRIBUTE_HOT void write(FormatBuffer const &, TransitEvent const &) override
  {
  }

    QUILL_ATTRIBUTE_HOT void flush() override {}
};
} // namespace quill
