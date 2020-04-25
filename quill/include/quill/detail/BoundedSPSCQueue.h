/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Macros.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/Utilities.h"
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
 * The queue is implemented as a circular buffer and must always be a power of two to optimise the
 * wrapping operations by using a simple mod mask.
 *
 * The circular buffer is also backed by an anonymous file in order to
 * not have to worry if the objects fits in the buffer in the case we have to wrap around.
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
class BoundedSPSCQueue
{
public:
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
      : _data(other._data), _indicator(other._indicator), _indicator_value(other._indicator_value)
    {
      // invalidate other
      other._indicator = nullptr;
      other._data = nullptr;
      other._indicator_value = 0;
    }

    /**
     * Move assignment operator
     */
    Handle& operator=(Handle&& other) noexcept
    {
      std::swap(_data, other._data);
      std::swap(_indicator, other._indicator);
      std::swap(_indicator_value, other._indicator_value);
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
        _indicator->store(_indicator_value, std::memory_order_release);
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
    QUILL_NODISCARD bool is_valid() const noexcept { return static_cast<bool>(_indicator); }

    /**
     * Release will release the handle without incrementing the tail.
     * This can be used if you want to observe a value without removing it from the queue
     * Calling queue.pop() again after release will return a Handle to the same object
     */
    void release() noexcept { _indicator = nullptr; }

  private:
    friend class BoundedSPSCQueue;

    /**
     * Private constructor
     * Only this constructor creates a valid handle
     */
    Handle(value_type* data, std::atomic<uint64_t>& indicator, uint64_t indicator_value) noexcept
      : _data(data), _indicator(&indicator), _indicator_value(indicator_value)
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
    std::atomic<uint64_t>* _indicator{nullptr};
    uint64_t _indicator_value{0};
  };

public:
  /**
   * Circular Buffer class Constructor
   * @throws on system error
   */
  explicit BoundedSPSCQueue(size_t capacity);

  /**
   * Destructor
   */
  ~BoundedSPSCQueue();

  /**
   * Deleted
   */
  BoundedSPSCQueue(BoundedSPSCQueue const&) = delete;
  BoundedSPSCQueue& operator=(BoundedSPSCQueue const&) = delete;

  /**
   * madvices and prefetches the memory in the allocated queue buffer.
   * This optimises page size misses which occur every 4K otherwise.
   * Should only be called once during init
   */
  QUILL_ATTRIBUTE_COLD void madvice() const;

  /**
   * Add a new object to the queue
   * @param args constructor arguments of the object we want to insert
   * @return true if we emplaced false otherwise
   */
  template <typename TInsertedObject, typename... Args>
  QUILL_NODISCARD_ALWAYS_INLINE_HOT bool try_emplace(Args&&... args) noexcept;

  /**
   * Return a handle containing the consumed data of the requested size
   * @return a handle to the popped object or false otherwise
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT Handle try_pop() noexcept;

  /**
   * @return total capacity of the queue in bytes
   */
  QUILL_NODISCARD size_t capacity() const noexcept { return _immutable_data.capacity; }

  /**
   * @return True when the queue is empty, false if there is still data to read
   */
  QUILL_NODISCARD bool empty() const noexcept;

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

private:
  /** Shared state - Never modified after initialisation */
  struct immutable_data
  {
    unsigned char* buffer{nullptr}; /**< The whole buffer */
    size_t capacity{0};
    size_t mask{0}; /** capacity - 1 mask for quick mod with &. Only for pow of 2 capacity */

#if defined(_WIN32)
    /**
     * For windows we have to store an extra pointer that we will give later to free. This is defined
     */
    void* mapped_file_handler{nullptr};
#endif
  };

  /** Members **/
  alignas(CACHELINE_SIZE) immutable_data _immutable_data;

  /** Shared mutable state - Both consumer and producer can read and write on each variable */
  alignas(CACHELINE_SIZE) std::atomic<uint64_t> _shared_head{0}; /**< The index of the head */
  alignas(CACHELINE_SIZE) std::atomic<uint64_t> _shared_tail{0}; /**< The index of the tail */

  /** Local state - modified by either the producer or consumer, but never both **/
  alignas(CACHELINE_SIZE) uint64_t _local_cached_head{0}; /**< cached head index */
  alignas(CACHELINE_SIZE) uint64_t _local_cached_tail{0}; /**< cached tail index */
};

/***/
template <typename TBaseObject>
BoundedSPSCQueue<TBaseObject>::BoundedSPSCQueue(size_t capacity)
{
  std::pair<unsigned char*, void*> res = create_memory_mapped_files(capacity);

  assert((reinterpret_cast<uintptr_t>(res.first) % CACHELINE_SIZE) == 0 &&
         "We must start with a cache aligned address");

  // Save the returned address
  _immutable_data.buffer = res.first;
  _immutable_data.capacity = capacity;
  _immutable_data.mask = capacity - 1;

#if defined(_WIN32)
  // On windows we have to store the extra pointer, for linux and macos it is just nullptr
  _immutable_data.mapped_file_handler = res.second;
#endif

  madvice();
}

/***/
template <typename TBaseObject>
BoundedSPSCQueue<TBaseObject>::~BoundedSPSCQueue()
{
#if defined(_WIN32)
  destroy_memory_mapped_files(
    std::pair<unsigned char*, void*>{_immutable_data.buffer, _immutable_data.mapped_file_handler},
    _immutable_data.capacity);
#else
  destroy_memory_mapped_files(std::pair<unsigned char*, void*>{_immutable_data.buffer, nullptr},
                              _immutable_data.capacity);
#endif
}

/***/
template <typename TBaseObject>
void BoundedSPSCQueue<TBaseObject>::madvice() const
{
  detail::madvice(_immutable_data.buffer, 2 * _immutable_data.capacity);
}

/***/
template <typename TBaseObject>
template <typename TInsertedObject, typename... Args>
bool BoundedSPSCQueue<TBaseObject>::try_emplace(Args&&... args) noexcept
{
  // Try to produce the required Bytes

  // load the existing head address into the new_head address
  uint64_t const head_loaded = _shared_head.load(std::memory_order_relaxed);

  // Start of buffer where we will put the item if we have enough space
  unsigned char* buffer_pos = _immutable_data.buffer + (head_loaded & _immutable_data.mask);

  // We will align all non trivial objects to cache line boundaries to avoid false sharing when we
  // call the destructor. We calculate the remaining bytes until the end of this cache line
  // and add them to the size
  size_t const obj_size =
    sizeof(TInsertedObject) + _distance_from_next_cache_line(buffer_pos, sizeof(TInsertedObject));

  // The remaining buffer capacity is capacity - (head - tail)
  // Tail is also only going once direction so we can first check the cached value
  if (obj_size > _immutable_data.capacity - (head_loaded - _local_cached_tail))
  {
    // we can't produce based on the cached tail so lets load the real one
    // get the updated tail value as the consumer now may have consumed some data
    _local_cached_tail = _shared_tail.load(std::memory_order_acquire);

    if (QUILL_UNLIKELY(obj_size > _immutable_data.capacity - (head_loaded - _local_cached_tail)))
    {
      // not enough space to produce
      return false;
    }
  }

  // Get the pointer to the beginning of the buffer
  // copy construct the Message there
  new (buffer_pos) TInsertedObject{std::forward<Args>(args)...};

  // update the head
  _shared_head.store(head_loaded + obj_size, std::memory_order_release);

  return true;
}

/***/
template <typename TBaseObject>
typename BoundedSPSCQueue<TBaseObject>::Handle BoundedSPSCQueue<TBaseObject>::try_pop() noexcept
{
  // we have been asked to consume but we don't know yet how much to consume
  // e.g object T might be a base class

  // load the existing head address into the new_head address
  uint64_t const tail_loaded = _shared_tail.load(std::memory_order_relaxed);

  // Check for data
  if (_local_cached_head == tail_loaded)
  {
    // We can't consume based on the cached tail but we can load the actual value
    // and try again see if the producer has produced more data
    _local_cached_head = _shared_head.load(std::memory_order_acquire);

    if (_local_cached_head == tail_loaded)
    {
      // nothing to consume
      return Handle{};
    }
  }

  // Get the tail pointer (beginning of the new object)
  unsigned char* buffer_pos = _immutable_data.buffer + (tail_loaded & _immutable_data.mask);

  assert((reinterpret_cast<uintptr_t>(buffer_pos) % CACHELINE_SIZE == 0) &&
         "Object should always be cache aligned");

  // Get the new object
  auto object_base = reinterpret_cast<value_type*>(buffer_pos);

  // Get the size of the object via the virtual function
  size_t const base_obj_size = object_base->size();

  // Return a Handle to the user for this object and increment the tail.
  // The producer aligns the objects in cache line boundaries so we have to calculate the same
  return Handle(object_base, _shared_tail,
                tail_loaded + base_obj_size + _distance_from_next_cache_line(buffer_pos, base_obj_size));
}

/***/
template <typename TBaseObject>
bool BoundedSPSCQueue<TBaseObject>::empty() const noexcept
{
  return _shared_head.load(std::memory_order_relaxed) == _shared_tail.load(std::memory_order_relaxed);
}

/***/
template <typename TBaseObject>
size_t BoundedSPSCQueue<TBaseObject>::_distance_from_next_cache_line(unsigned char* start_pos, size_t obj_size) noexcept
{
  // increment the pointer to obj size
  start_pos += obj_size;
  // get a new aligned ptr and return the difference from the original
  return static_cast<size_t>(align_pointer<CACHELINE_SIZE, unsigned char>(start_pos) - start_pos);
}
} // namespace detail
} // namespace quill