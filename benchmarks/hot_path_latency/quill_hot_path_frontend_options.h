/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/FrontendOptions.h"

#include <cstddef>

// Every option not set here keeps Quill's default, including the queue capacity of the default
// unbounded queue and the immediate flush check
struct CustomFrontendOptions : quill::FrontendOptions
{
#ifdef QUILL_USE_BOUNDED_DROPPING_QUEUE
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;

  // A bounded queue never grows, so it is sized for the load, as an application would: 512 KiB per
  // thread. Quill's default of 128 KiB holds about six bursts of the vector_largestr payload and
  // drops messages whenever the backend thread is descheduled for longer, which happens on a
  // machine without isolated CPUs. A dropped call skips the copy, so a run that drops messages is
  // not comparable.
  static constexpr size_t initial_queue_capacity = 512u * 1024u;
#endif
};
