#pragma once

#include <functional>

#include "quill/detail/record/RecordBase.h"

namespace quill::detail
{
/**
 * Special type of command record.
 * Used to execute a function inside the backend thread rather than printing a LogRecord.
 * We use that to make the caller wait until the logger thread has flushed all LogRecords to all sinks
 */
class CommandRecord final : public RecordBase
{
public:
  explicit CommandRecord(std::function<void()>&& callback) : _callback(std::move(callback)) {}

  [[nodiscard]] size_t size() const noexcept override { return sizeof(*this); }

  void backend_process(uint32_t) const noexcept override {}

  /**
   * Destructor
   */
  ~CommandRecord() override = default;

private:
  std::function<void()> _callback;
  mutable bool _processed{false};
};
} // namespace quill::detail