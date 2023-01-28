#include "quill/detail/backend/TransitEventBuffer.h"

#include "quill/QuillError.h"
#include "quill/detail/misc/Utilities.h"

namespace quill::detail
{
/***/
BoundedTransitEventBuffer::BoundedTransitEventBuffer(uint32_t capacity)
  : _capacity(capacity), _mask(capacity - 1)
{
  if (!is_pow_of_two(static_cast<size_t>(capacity)))
  {
    QUILL_THROW(std::runtime_error{"Capacity must be a power of two"});
  }

  _storage.resize(capacity);
}

/***/
TransitEvent* BoundedTransitEventBuffer::front() noexcept
{
  if (_writer_pos == _reader_pos)
  {
    // empty
    return nullptr;
  }

  return &_storage[static_cast<uint32_t>(_reader_pos & _mask)];
};

/***/
void BoundedTransitEventBuffer::pop_front() noexcept { _reader_pos += 1; }

/***/
TransitEvent* BoundedTransitEventBuffer::back() noexcept
{
  if (_capacity - (_writer_pos - _reader_pos) == 0)
  {
    // is full
    return nullptr;
  }

  _storage[static_cast<uint32_t>(_writer_pos & _mask)].reset();
  return &_storage[static_cast<uint32_t>(_writer_pos & _mask)];
}

void BoundedTransitEventBuffer::push_back() noexcept { _writer_pos += 1; }

/***/
uint32_t BoundedTransitEventBuffer::size() const noexcept
{
  return static_cast<uint32_t>(_writer_pos - _reader_pos);
}

/***/
uint32_t BoundedTransitEventBuffer::capacity() const noexcept
{
  return static_cast<uint32_t>(_capacity);
}

/***/
UnboundedTransitEventBuffer::UnboundedTransitEventBuffer(uint32_t initial_transit_buffer_capacity)
  : _writer(new Node(initial_transit_buffer_capacity)), _reader(_writer)
{
}

/***/
UnboundedTransitEventBuffer::~UnboundedTransitEventBuffer()
{
  Node* reader_node = _reader;

  // Look for extra nodes to delete
  while (reader_node)
  {
    auto to_delete = reader_node;
    reader_node = reader_node->next;
    delete to_delete;
  }
}

/***/
TransitEvent* UnboundedTransitEventBuffer::front() noexcept
{
  TransitEvent* next_event = _reader->transit_buffer.front();

  if (!next_event)
  {
    // the buffer is empty check if another buffer exists
    if (QUILL_UNLIKELY(_reader->next != nullptr))
    {
      // a new buffer was added by the producer, this happens only when we have allocated a new queue

      // switch to the new buffer, existing one is deleted
      Node* next_node = _reader->next;
      delete _reader;
      _reader = next_node;
      next_event = _reader->transit_buffer.front();
    }
  }

  return next_event;
}

/***/
void UnboundedTransitEventBuffer::pop_front() noexcept { _reader->transit_buffer.pop_front(); }

/***/
TransitEvent* UnboundedTransitEventBuffer::back() noexcept
{
  // Try to reserve the bounded queue
  TransitEvent* write_event = _writer->transit_buffer.back();

  if (QUILL_UNLIKELY(write_event == nullptr))
  {
    // buffer doesn't have enough space
    auto new_node = new Node{_writer->transit_buffer.capacity() * 2};
    _writer->next = new_node;
    _writer = _writer->next;
    write_event = _writer->transit_buffer.back();
  }

  assert(write_event && "Write event is always true");

  return write_event;
}

/***/
void UnboundedTransitEventBuffer::push_back() noexcept { _writer->transit_buffer.push_back(); }

/***/
uint32_t UnboundedTransitEventBuffer::size() const noexcept
{
  Node* reader = _reader;

  uint32_t size = reader->transit_buffer.size();

  while (reader->next)
  {
    reader = reader->next;
    size += reader->transit_buffer.size();
  }

  return size;
}

/***/
bool UnboundedTransitEventBuffer::empty() noexcept { return front() ? false : true; }
} // namespace quill::detail