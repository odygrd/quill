#pragma once

#include <cstdint>

#include <x86intrin.h>

namespace quill::detail
{
/**
 * Base message class used to retrieve and process the messages from the queue
 */
class MessageBase
{
public:
  MessageBase() = default;
  MessageBase(MessageBase const&) = delete;
  MessageBase& operator=(MessageBase const&) = delete;
  virtual ~MessageBase() = default;

  /**
   * Get the rdtsc timestamp
   * @note Called on the logger thread
   */
  [[nodiscard]] uint64_t rdtsc() const noexcept { return _rdtsc; }

  /**
   * Required by the queue to get the real message size
   * @return The size of the derived message
   */
  [[nodiscard]] virtual size_t size() const noexcept = 0;

  /**
   * Process a message. Called on the logging worker thread
   * @param thread_id the thread_id of the caller thread
   */
  virtual void backend_process(uint32_t thread_id) const noexcept = 0;

private:
  uint64_t _rdtsc{__rdtsc()};
};
} // namespace quill::detail