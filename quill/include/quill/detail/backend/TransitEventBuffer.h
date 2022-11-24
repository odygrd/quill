#pragma once

#include "quill/TransitEvent.h"
#include <cstdint>
#include <vector>

namespace quill::detail
{
class TransitEventBuffer
{
public:
  TransitEventBuffer(uint32_t capacity) { _buffer.resize(capacity); }

  TransitEvent* prepare_push();
  void push();

  TransitEvent* front();
  void pop();

  uint32_t capacity();
  uint32_t size();
  bool empty();
  bool full();

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
  UnboundedTransitEventBuffer(uint32_t initial_capacity)
    : _writer(std::make_shared<Node>(initial_capacity)), _reader(_writer){};
  TransitEvent* prepare_push();
  void push();

  TransitEvent* front();
  void pop();

  uint32_t size();

private:
  std::shared_ptr<Node> _writer{nullptr};
  std::shared_ptr<Node> _reader{nullptr};
};
} // namespace quill::detail