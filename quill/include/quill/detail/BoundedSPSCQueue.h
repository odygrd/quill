#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#include <sys/mman.h>
#include <unistd.h>

#include "quill/detail/CommonMacros.h"
#include "quill/detail/CommonUtilities.h"

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
template <typename TBaseObject, std::size_t Capacity>
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
    [[nodiscard]] value_type* data() const noexcept { return _data; }

    /**
     * Checks the validity of this handle.
     * @return True if valid, otherwise false.
     */
    [[nodiscard]] bool is_valid() const noexcept { return static_cast<bool>(_indicator); }

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
  BoundedSPSCQueue();

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
   * Add a new object to the queue
   * @tparam TInsertedObject
   * @tparam Args
   * @param args
   * @return
   */
  template <typename TInsertedObject, typename... Args>
  [[nodiscard]] inline bool try_emplace(Args&&... args) noexcept;

  /**
   * Return a handle containing the consumed data of the requested size
   * @param size the size we requested to consume
   * @return
   */
  [[nodiscard]] inline Handle try_pop() noexcept;

  /**
   * @return total capacity of the queue in bytes
   */
  [[nodiscard]] inline std::size_t capacity() const noexcept { return Capacity; }

  /**
   * @return True when the queue is empty, false if there is still data to read
   */
  [[nodiscard]] inline bool empty() const noexcept;

private:
  static uint32_t _get_page_size() noexcept;

private:
  /** Mask to shift around power of two capacity */
  static constexpr std::size_t MASK{Capacity - 1};

  /**
   * Packed in a struct to maintain both in the same cache line
   */
  struct local_state
  {
    unsigned char* buffer{nullptr}; /**< The whole buffer */
    uint64_t cached_atomic{0}; /**< producer cache to tail address or consumer cache to head address */
  };

  /** Local State - Read and Write only by either the producer or consumer **/
  alignas(detail::CACHELINE_SIZE) local_state _producer;
  alignas(detail::CACHELINE_SIZE) local_state _consumer;

  /** Shared State - Both consumer and producer can read and write on each variable **/
  alignas(detail::CACHELINE_SIZE) std::atomic<uint64_t> _head{0}; /**< The index of the head */
  alignas(detail::CACHELINE_SIZE) std::atomic<uint64_t> _tail{0}; /**< The index of the tail */
};

/***/
template <typename TBaseObject, std::size_t Capacity>
BoundedSPSCQueue<TBaseObject, Capacity>::BoundedSPSCQueue()
{
  if (!is_pow_of_two(capacity()))
  {
    throw std::runtime_error("capacity needs to be power of two");
  }
  else if (capacity() % _get_page_size() != 0)
  {
    throw std::runtime_error("capacity needs to be multiple of page size");
  }

  char shm_path[] = "/dev/shm/quill-XXXXXX";
  char tmp_path[] = "/tmp/quill-XXXXXX";
  char const* chosen_path{nullptr};

  // Try to open an fd by creating a unique file in one of the above locations
  int fd = mkstemp(shm_path);

  if (fd < 0)
  {
    // if we failed try the tmp path
    fd = mkstemp(tmp_path);
    if (fd < 0)
    {
      std::ostringstream error_message;
      error_message << "Could not open file in any location. Error : " << std::strerror(errno);
      throw std::runtime_error(error_message.str());
    }
    chosen_path = tmp_path;
  }
  else
  {
    chosen_path = shm_path;
  }

  // Delete the file as we only want the fd
  if (unlink(chosen_path) == -1)
  {
    close(fd);
    std::ostringstream error_message;
    error_message << "Could not unlink file. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  if (ftruncate(fd, capacity()) == -1)
  {
    close(fd);
    std::ostringstream error_message;
    error_message << "Could not truncate file. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  // ask mmap for a good address where we can put both virtual copies of the buffer
  auto address = static_cast<unsigned char*>(
    mmap(nullptr, 2 * capacity(), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0));

  if (address == MAP_FAILED)
  {
    close(fd);
    std::ostringstream error_message;
    error_message << "Failed to mmap. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  // map first region
  auto other_address = static_cast<unsigned char*>(
    mmap(address, capacity(), PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE, fd, 0));
  if (other_address != address)
  {
    munmap(address, 2 * capacity());
    close(fd);
    std::ostringstream error_message;
    error_message << "Failed to mmap. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  // map second region
  other_address = static_cast<unsigned char*>(
    mmap(address + capacity(), capacity(), PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE, fd, 0));
  if (other_address != address + capacity())
  {
    munmap(address, 2 * capacity());
    close(fd);
    std::ostringstream error_message;
    error_message << "Failed to mmap. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  // we don't need the fd any longer
  if (close(fd) == -1)
  {
    munmap(address, 2 * capacity());
    std::ostringstream error_message;
    error_message << "Failed to close the fd. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  _producer.buffer = address;
  _consumer.buffer = address;
}

/***/
template <typename TBaseObject, std::size_t Capacity>
BoundedSPSCQueue<TBaseObject, Capacity>::~BoundedSPSCQueue()
{
  if (!_producer.buffer || !_consumer.buffer)
  {
    return;
  }

#if defined(_WIN32)
  BOOL ok;
  ok = UnmapViewOfFile(_producer.buffer);
  assert(ok);
  ok = UnmapViewOfFile(_producer.buffer + capacity());
  assert(ok);
  ok = CloseHandle((HANDLE)mem->priv);
  assert(ok);
#else
  int err = munmap(_producer.buffer, 2 * capacity());
  (void)err;
  assert(!err);
#endif
}

/***/
template <typename TBaseObject, std::size_t Capacity>
template <typename TInsertedObject, typename... Args>
bool BoundedSPSCQueue<TBaseObject, Capacity>::try_emplace(Args&&... args) noexcept
{
  // Try to produce the required Bytes

  // load the existing head address into the new_head address
  uint64_t const head_loaded = _head.load(std::memory_order_relaxed);

  // The remaining buffer capacity is capacity - (head - tail)
  // Tail is also only going once direction so we can first check the cached value
  if (QUILL_UNLIKELY((capacity() - sizeof(TInsertedObject)) < (head_loaded - _producer.cached_atomic)))
  {
    // we can't produce based on the cached tail so lets load the real one
    // get the updated tail value as the consumer now may have consumed some data
    _producer.cached_atomic = _tail.load(std::memory_order_acquire);

    if (QUILL_UNLIKELY((capacity() - sizeof(TInsertedObject)) < (head_loaded - _producer.cached_atomic)))
    {
      // not enough space to produce
      return false;
    }
  }

  // Get the pointer to the beginning of the buffer
  // copy construct the Message there
  new (_producer.buffer + (head_loaded & MASK)) TInsertedObject{std::forward<Args>(args)...};

  // update the head
  _head.store(head_loaded + sizeof(TInsertedObject), std::memory_order_release);

  return true;
}

/***/
template <typename TBaseObject, std::size_t Capacity>
typename BoundedSPSCQueue<TBaseObject, Capacity>::Handle BoundedSPSCQueue<TBaseObject, Capacity>::try_pop() noexcept
{
  // we have been asked to consume but we don't know yet how much to consume
  // e.g object T might be a base class

  // load the existing head address into the new_head address
  uint64_t const tail_loaded = _tail.load(std::memory_order_relaxed);

  // Check for data
  if (_consumer.cached_atomic == tail_loaded)
  {
    // We can't consume based on the cached head but we can load the actual value
    // and try again see if the producer has produced more data
    _consumer.cached_atomic = _head.load(std::memory_order_acquire);

    if (_consumer.cached_atomic == tail_loaded)
    {
      // nothing to consume
      return Handle{};
    }
  }

  // Get the tail pointer (beginning of the new object)
  auto object_base = reinterpret_cast<value_type*>(_consumer.buffer + (tail_loaded & MASK));

  // Return a Handle to the user for this object and increment the tail
  return Handle(object_base, _tail, _tail + object_base->size());
}

/***/
template <typename TBaseObject, std::size_t Capacity>
bool BoundedSPSCQueue<TBaseObject, Capacity>::empty() const noexcept
{
  return _head.load(std::memory_order_relaxed) == _tail.load(std::memory_order_relaxed);
}

/***/
template <typename TBaseObject, std::size_t Capacity>
uint32_t BoundedSPSCQueue<TBaseObject, Capacity>::_get_page_size() noexcept
{
  static uint32_t page_size{0};
#if defined(_WIN32)
  SYSTEM_INFO system_info;
  GetSystemInfo(&system_info);
  page_size = std::max(system_info.dwPageSize, system_info.dwAllocationGranularity);
#else
  page_size = static_cast<uint32_t>(sysconf(_SC_PAGESIZE));
#endif
  return page_size;
}
} // namespace detail
} // namespace quill