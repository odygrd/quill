/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Macros.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/Utilities.h"
#include "quill/detail/spsc_queue/BoundedSPSCRawQueue.h"
#include <atomic>
#include <cassert>
#include <cstdint>
#include <type_traits>

namespace quill
{
namespace detail
{
/**
 * A bounded queue that can store objects of different types.
 * Because we don't know the type of the object when we pop() we should pop() a base class
 * object and then call a virtual method on it.
 *
 * This queue is meant to be used when you want to store variable sized objects.
 *
 * Usage :
 * Push the derived type
 * Pop a handle to the base type.
 * Then either
 * 1) cast the base type to the derived type based on a named tag or
 * 2) call a virtual method of the base type
 *
 * @see BoundedSPSCQueueTest.cpp for examples
 * @tparam TBaseObject A base class type
 */
template <typename TBaseObject>
class BoundedSPSCEventQueue : private BoundedSPSCRawQueue
{
public:
  using base_t = BoundedSPSCRawQueue;
  using value_type = TBaseObject;

  /**
   * A handle to a consumed object.
   */
  class Handle
  {
  public:
    /**
     * Creates a default invalid handle.
     */
    Handle() = default;

    /**
     * Move constructor
     */
    Handle(Handle&& other) noexcept
      : _data(other._data), _buffer_ref(other._buffer_ref), _read_size(other._read_size)
    {
      // invalidate other
      other._buffer_ref = nullptr;
      other._data = nullptr;
      other._read_size = 0;
    }

    /**
     * Move assignment operator
     */
    Handle& operator=(Handle&& other) noexcept
    {
      std::swap(_data, other._data);
      std::swap(_buffer_ref, other._buffer_ref);
      std::swap(_read_size, other._read_size);
      return *this;
    }

    /**
     * Destructor
     * Destructs the reference object and also increments the read position in the buffer
     */
    ~Handle()
    {
      if (is_valid())
      {
        _destroy();
        _buffer_ref->finish_read(_read_size);
      }
    }

    /**
     * Deleted
     */
    Handle(Handle const&) = delete;
    Handle& operator=(Handle const&) = delete;

    /**
     * Get the object memory.
     */
    QUILL_NODISCARD value_type* data() const noexcept { return _data; }

    /**
     * Checks the validity of this handle.
     * @return True if valid, otherwise false.
     */
    QUILL_NODISCARD bool is_valid() const noexcept { return static_cast<bool>(_buffer_ref); }

    /**
     * Release will release the handle without incrementing the tail.
     * This can be used if you want to observe a value without removing it from the queue
     * Calling queue.pop() again after release will return a Handle to the same object
     */
    void release() noexcept { _buffer_ref = nullptr; }

  private:
    friend class BoundedSPSCEventQueue;

    /**
     * Private constructor
     * Only this constructor creates a valid handle
     */
    Handle(value_type* data, BoundedSPSCRawQueue* queue_ref, size_t read_size) noexcept
      : _data(data), _buffer_ref(queue_ref), _read_size(read_size)
    {
    }

    /**
     * Do not run a destructor for a trivially destructible object
     */
    template <typename UBaseObject = TBaseObject>
    typename std::enable_if<std::is_trivially_destructible<UBaseObject>::value>::type _destroy()
    {
    }

    /**
     * Run a destructor for a trivially destructible object
     */
    template <typename UBaseObject = TBaseObject>
    typename std::enable_if<!std::is_trivially_destructible<UBaseObject>::value>::type _destroy()
    {
      _data->~value_type();
    }

  private:
    value_type* _data{nullptr}; /**< The data */
    BoundedSPSCRawQueue* _buffer_ref{nullptr};
    size_t _read_size{0};
  };

public:
  using handle_t = Handle;

  /**
   * Circular Buffer class Constructor
   * @throws on system error
   */
  BoundedSPSCEventQueue() = default;

  /**
   * Destructor
   */
  ~BoundedSPSCEventQueue() = default;

  /**
   * Deleted
   */
  BoundedSPSCEventQueue(BoundedSPSCEventQueue const&) = delete;
  BoundedSPSCEventQueue& operator=(BoundedSPSCEventQueue const&) = delete;

  /**
   * Add a new object to the queue
   * @param args constructor arguments of the object we want to insert
   * @return true if we emplaced false otherwise
   */
  template <typename TInsertedObject, typename... Args>
  QUILL_NODISCARD_ALWAYS_INLINE_HOT bool try_emplace(Args&&... args) noexcept;

  /**
   * Return a handle containing the consumed data of the requested size
   * @return a handle to the object in the queue or an invalid handle object if the queue is empty
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT Handle try_pop() noexcept;

  /**
   * @return True when the queue is empty, false if there is still data to read
   */
  QUILL_NODISCARD bool empty() const noexcept
  {
    return reinterpret_cast<base_t const*>(this)->empty();
  }

  QUILL_NODISCARD size_t capacity() const noexcept { return base_t::capacity(); }

private:
  /**
   * Returns the remaining bytes until the end of the cache line. For non trivial objects because
   * the consumer calls the destructor we align them on cache line boundaries to avoid false
   * sharing with the producer
   * @param start_pos starting memory position
   * @param obj_size object size
   * @return the distance from the next cache line
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT static size_t _distance_from_next_cache_line(unsigned char* start_pos,
                                                                                 size_t obj_size) noexcept;
};

/***/
template <typename TBaseObject>
template <typename TInsertedObject, typename... Args>
bool BoundedSPSCEventQueue<TBaseObject>::try_emplace(Args&&... args) noexcept
{
  static_assert(sizeof(TInsertedObject) < QUILL_QUEUE_CAPACITY,
                "The size of the object is greater than the queue capacity. Increase "
                "QUILL_QUEUE_CAPACITY to the next power of two.");

  // We will align all non trivial objects to cache line boundaries to avoid false sharing when we
  // call the destructor.
  // We calculate the remaining bytes until the end of this cache line and add them to the size
  size_t const obj_size = sizeof(TInsertedObject) +
    _distance_from_next_cache_line(this->_producer_pos.load(std::memory_order_relaxed), sizeof(TInsertedObject));

  // We want to know if we have enough space in the buffer to store the object
  unsigned char* write_buffer = this->prepare_write(obj_size);

  if (QUILL_UNLIKELY(write_buffer == nullptr))
  {
    // not enough space to produce
    return false;
  }

  // emplace construct the Message there
  new (write_buffer) TInsertedObject{std::forward<Args>(args)...};

  // update the buffer with the write
  this->commit_write(obj_size);

  return true;
}

/***/
template <typename TBaseObject>
typename BoundedSPSCEventQueue<TBaseObject>::Handle BoundedSPSCEventQueue<TBaseObject>::try_pop() noexcept
{
  // we have been asked to consume but we don't know yet how much to consume
  // e.g object T might be a base class

  auto const buffer_span = this->prepare_read();

  if (buffer_span.second == 0)
  {
    // nothing to consume
    return Handle{};
  }

  // Get the beginning of the new object
  unsigned char* read_buffer = buffer_span.first;

  assert((reinterpret_cast<uintptr_t>(read_buffer) % CACHELINE_SIZE == 0) &&
         "Object should always be cache aligned");

  // Get the new object
  auto object_base = reinterpret_cast<value_type*>(read_buffer);

  // Get the size of the object via the virtual function
  size_t const base_obj_size = object_base->size();

  // The real object size will be until the end of the cache line
  size_t const total_obj_size = base_obj_size + _distance_from_next_cache_line(read_buffer, base_obj_size);

  // Return a Handle to the user for this object
  return Handle(object_base, this, total_obj_size);
}

/***/
template <typename TBaseObject>
size_t BoundedSPSCEventQueue<TBaseObject>::_distance_from_next_cache_line(unsigned char* start_pos,
                                                                          size_t obj_size) noexcept
{
  // increment the pointer to obj size
  start_pos += obj_size;
  // get a new aligned ptr and return the difference from the original
  return static_cast<size_t>(align_pointer<CACHELINE_SIZE, unsigned char>(start_pos) - start_pos);
}
} // namespace detail
} // namespace quill