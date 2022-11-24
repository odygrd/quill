#include "quill/detail/backend/TransitEventBuffer.h"

namespace quill::detail
{
/***/
TransitEvent* TransitEventBuffer::prepare_push()
{
  if ((capacity() - size()) > 1)
  {
    return &_buffer[_write_index];
  }

  return nullptr;
}

/***/
void TransitEventBuffer::push()
{
  ++_write_index;
  if (_write_index == _buffer.size())
  {
    _write_index = 0;
  }
}

/***/
TransitEvent* TransitEventBuffer::front()
{
  if (size() > 0)
  {

    return &_buffer[_read_index];
  }

  return nullptr;
}

/***/
void TransitEventBuffer::pop()
{
  ++_read_index;
  if (_read_index == _buffer.size())
  {
    _read_index = 0;
  }
}

/***/
uint32_t TransitEventBuffer::capacity() { return static_cast<uint32_t>(_buffer.capacity()); }

/***/
uint32_t TransitEventBuffer::size()
{
  if (_write_index >= _read_index)
  {
    return _write_index - _read_index;
  }
  else
  {
    return capacity() - _read_index + _write_index;
  }
}

/***/
bool TransitEventBuffer::empty() { return size() == 0; }

/***/
bool TransitEventBuffer::full() { return size() == capacity() - 1; }

/***/
TransitEvent* UnboundedTransitEventBuffer::prepare_push()
{
  TransitEvent* writeable = _writer->transit_event_buffer.prepare_push();

  if (!writeable)
  {
    _writer->next = std::make_shared<Node>(_writer->transit_event_buffer.capacity() * 2);
    _writer = _writer->next;

    writeable = _writer->transit_event_buffer.prepare_push();
    assert(writeable && "Writeable can not be nulltptr");
  }

  return writeable;
}

/***/
void UnboundedTransitEventBuffer::push() { _writer->transit_event_buffer.push(); }

/***/
TransitEvent* UnboundedTransitEventBuffer::front()
{
  TransitEvent* readable = _reader->transit_event_buffer.front();

  if (!readable)
  {
    if (_reader->next != nullptr)
    {
      _reader = _reader->next;
      readable = _reader->transit_event_buffer.front();
    }
  }

  return readable;
}

/***/
void UnboundedTransitEventBuffer::pop() { _reader->transit_event_buffer.pop(); }

/***/
uint32_t UnboundedTransitEventBuffer::size()
{
  uint32_t s{0};
  s = _reader->transit_event_buffer.size();

  Node* next = _reader->next.get();
  while (next != nullptr)
  {
    // there is another buffer
    s += _reader->next->transit_event_buffer.size();
    next = next->next.get();
  }

  return s;
}

} // namespace quill::detail