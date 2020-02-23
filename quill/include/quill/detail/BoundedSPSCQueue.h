#pragma once

#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Macros.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/Utilities.h"
#include <atomic>
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
 * @tparam T A base class type
 * @tparam capacity The total buffer's capacity in bytes
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
     * @param other
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
     * @param other
     * @return
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
     * @tparam U
     * @return
     */
    template <typename UBaseObject = TBaseObject>
    typename std::enable_if<std::is_trivially_destructible<UBaseObject>::value>::type _destroy()
    {
    }

    /**
     * Run a destructor for a trivially destructible object
     * @tparam U
     * @return
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
   * @tparam TInsertedObject
   * @tparam Args
   * @param args
   * @return
   */
  template <typename TInsertedObject, typename... Args>
  QUILL_NODISCARD_ALWAYS_INLINE_HOT bool try_emplace(Args&&... args) noexcept;

  /**
   * Return a handle containing the consumed data of the requested size
   * @param size the size we requested to consume
   * @return
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT Handle try_pop() noexcept;

  /**
   * @return total capacity of the queue in bytes
   */
  QUILL_NODISCARD size_t capacity() const noexcept { return _capacity; }

  /**
   * @return True when the queue is empty, false if there is still data to read
   */
  QUILL_NODISCARD bool empty() const noexcept;

private:
  /** Mask to shift around power of two capacity */
  unsigned char* _buffer{nullptr}; /**< The whole buffer */
  size_t _capacity{0};
  size_t _mask{0}; /** capacity - 1 mask for quick mod with &. Only for pow of 2 capacity */
  uint64_t _cached_head{0};
  uint64_t _cached_tail{0};

  /** Shared State - Both consumer and producer can read and write on each variable **/
  alignas(detail::CACHELINE_SIZE) std::atomic<uint64_t> _head{0}; /**< The index of the head */
  char _pad3[detail::CACHELINE_SIZE - sizeof(std::atomic<uint64_t>)]; /** We don't really need to specify the padding explictly but Visual Studio will give a warning */
  alignas(detail::CACHELINE_SIZE) std::atomic<uint64_t> _tail{0}; /**< The index of the tail */
  char _pad4[detail::CACHELINE_SIZE - sizeof(std::atomic<uint64_t>)]; /** We don't really need to specify the padding explictly but Visual Studio will give a warning */

#if defined(_WIN32)
  /**
   * For windows we have to store an extra pointer that we will give later to free
   */
  void* _mapped_file_handler{nullptr};
#endif
};

/***/
template <typename TBaseObject>
BoundedSPSCQueue<TBaseObject>::BoundedSPSCQueue(size_t capacity)
  : _capacity(capacity), _mask(capacity - 1)
{
  std::pair<unsigned char*, void*> res = create_memory_mapped_files(capacity);

  // Save the returned address
  _buffer = res.first;

#if defined(_WIN32)
  // On windows we have to store the extra pointer, for linux and macos it is just nullptr
  _mapped_file_handler = res.second;
#endif

  madvice();
}

/***/
template <typename TBaseObject>
BoundedSPSCQueue<TBaseObject>::~BoundedSPSCQueue()
{
#if defined(_WIN32)
  destroy_memory_mapped_files(std::pair<unsigned char*, void*>{_buffer, _mapped_file_handler}, capacity());
#else
  destroy_memory_mapped_files(std::pair<unsigned char*, void*>{_buffer, nullptr}, capacity());
#endif
}

/***/
template <typename TBaseObject>
void BoundedSPSCQueue<TBaseObject>::madvice() const
{
  detail::madvice(_buffer, 2 * _capacity);
}

/***/
template <typename TBaseObject>
template <typename TInsertedObject, typename... Args>
bool BoundedSPSCQueue<TBaseObject>::try_emplace(Args&&... args) noexcept
{
  // Try to produce the required Bytes

  // load the existing head address into the new_head address
  uint64_t const head_loaded = _head.load(std::memory_order_relaxed);

  // The remaining buffer capacity is capacity - (head - tail)
  // Tail is also only going once direction so we can first check the cached value
  if (QUILL_UNLIKELY((capacity() - sizeof(TInsertedObject)) < (head_loaded - _cached_head)))
  {
    // we can't produce based on the cached tail so lets load the real one
    // get the updated tail value as the consumer now may have consumed some data
    _cached_head = _tail.load(std::memory_order_acquire);

    if (QUILL_UNLIKELY((capacity() - sizeof(TInsertedObject)) < (head_loaded - _cached_head)))
    {
      // not enough space to produce
      return false;
    }
  }

  // Get the pointer to the beginning of the buffer
  // copy construct the Message there
  new (_buffer + (head_loaded & _mask)) TInsertedObject{std::forward<Args>(args)...};

  // update the head
  _head.store(head_loaded + sizeof(TInsertedObject), std::memory_order_release);

  return true;
}

/***/
template <typename TBaseObject>
typename BoundedSPSCQueue<TBaseObject>::Handle BoundedSPSCQueue<TBaseObject>::try_pop() noexcept
{
  // we have been asked to consume but we don't know yet how much to consume
  // e.g object T might be a base class

  // load the existing head address into the new_head address
  uint64_t const tail_loaded = _tail.load(std::memory_order_relaxed);

  // Check for data
  if (_cached_tail == tail_loaded)
  {
    // We can't consume based on the cached tail but we can load the actual value
    // and try again see if the producer has produced more data
    _cached_tail = _head.load(std::memory_order_acquire);

    if (_cached_tail == tail_loaded)
    {
      // nothing to consume
      return Handle{};
    }
  }

  // Get the tail pointer (beginning of the new object)
  auto object_base = reinterpret_cast<value_type*>(_buffer + (tail_loaded & _mask));

  // Return a Handle to the user for this object and increment the tail
  return Handle(object_base, _tail, _tail + object_base->size());
}

/***/
template <typename TBaseObject>
bool BoundedSPSCQueue<TBaseObject>::empty() const noexcept
{
  return _head.load(std::memory_order_relaxed) == _tail.load(std::memory_order_relaxed);
}
} // namespace detail
} // namespace quill