#pragma once

#include <functional>

#include "quill/detail/message/MessageBase.h"

namespace quill::detail
{
/**
 * Special type of command message.
 * Used to execute a function on the logger thread rather than printing a message.
 * We use that to make the caller wait until the logger thread has flushed every message
 */
class CommandMessage final : public MessageBase
{
public:
  explicit CommandMessage(std::function<void()>&& callback) : _callback(std::move(callback)) {}

  [[nodiscard]] size_t size() const noexcept override { return sizeof(*this); }

  void backend_process(uint32_t) const noexcept override {}

  /**
   * Destructor
   */
  ~CommandMessage() override = default;

private:
  std::function<void()> _callback;
  mutable bool _processed{false};
};
} // namespace quill::detail