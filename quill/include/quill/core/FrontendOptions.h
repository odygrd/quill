/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"

#include <cstdint>

QUILL_BEGIN_NAMESPACE

struct FrontendOptions
{
  /**
   * Each frontend thread has its own queue, which can be configured with various options:
   * - UnboundedBlocking: Starts with initial_queue_capacity and reallocates up to 2GB, then blocks.
   * - UnboundedDropping: Starts with initial_queue_capacity and reallocates up to 2GB, then drops log messages.
   * - UnboundedUnlimited: Starts with initial_queue_capacity and reallocates up to 2GB; subsequent queues are reallocated as needed. Never blocks or drops.
   * - BoundedBlocking: Starts with initial_queue_capacity and never reallocates; blocks when the limit is reached.
   * - BoundedDropping: Starts with initial_queue_capacity and never reallocates; drops log messages when the limit is reached.
   *
   * By default, the library uses an UnboundedBlocking queue, which starts with initial_queue_capacity.
   */
  static constexpr quill::QueueType queue_type = quill::QueueType::UnboundedBlocking;

  /**
   * Initial capacity of the queue. Used for UnboundedBlocking, UnboundedDropping, and
   * UnboundedUnlimited. Also serves as the capacity for BoundedBlocking and BoundedDropping.
   */
  static constexpr uint32_t initial_queue_capacity = 131'072;

  /**
   * Interval for retrying when using BoundedBlocking or UnboundedBlocking.
   * Applicable only when using BoundedBlocking or UnboundedBlocking.
   */
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;

  /**
   * Enables huge pages on the frontend queues to reduce TLB misses. Available only for Linux.
   */
  static constexpr bool huge_pages_enabled = false;
};

QUILL_END_NAMESPACE