#pragma once

#include "quill/TransitEvent.h"
#include <cstdint>
#include <vector>

namespace quill::detail
{
class TransitEventBuffer
{
public:
  explicit TransitEventBuffer(uint32_t capacity) { _buffer.resize(capacity); }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* prepare_push();
  QUILL_ATTRIBUTE_HOT void push();

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* front();
  QUILL_ATTRIBUTE_HOT void pop();

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT uint32_t capacity();
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT uint32_t size();
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool empty();
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool full();

private:
  std::vector<TransitEvent> _buffer;
  uint32_t _read_index{0};
  uint32_t _write_index{0};
};

class UnboundedTransitEventBuffer
{
private:
  struct Node
  {
    explicit Node(uint32_t capacity) : transit_event_buffer(capacity){};
    std::shared_ptr<Node> next;
    TransitEventBuffer transit_event_buffer;
  };

public:
  explicit UnboundedTransitEventBuffer(uint32_t initial_capacity)
    : _writer(std::make_shared<Node>(initial_capacity)), _reader(_writer){};

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* prepare_push();
  QUILL_ATTRIBUTE_HOT void push();

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* front();
  QUILL_ATTRIBUTE_HOT void pop();

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT uint32_t size();

private:
  std::shared_ptr<Node> _writer{nullptr};
  std::shared_ptr<Node> _reader{nullptr};
};
} // namespace quill::detail