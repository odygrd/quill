#pragma once

#include "quill/detail/LogLineInfo.h"
#include <cstdint>
#include <tuple>

namespace quill::detail
{
/**
 * Base message class used to retrieve and process the messages from the queue
 */
class MessageBase
{
public:
  MessageBase() = default;
  virtual ~MessageBase() = default;

  /**
   * Required by the queue to get the real message size
   * @return The size of the derived message
   */
  [[nodiscard]] virtual size_t size() const noexcept = 0;
  [[nodiscard]] virtual uint64_t rdtsc() const noexcept = 0;
};

/**
 * For each log statement a message is produced and pushed to the thread local spsc queue.
 * The logging thread will retrieve the messages from the queue using the base class pointer.
 * @tparam T
 * @tparam U This is unused for this message. We need it for the custom command message
 */
template <typename T, typename U = void, typename... FmtArgs>
class Message final : public MessageBase
{
public:
  using tuple_t = T;

  /**
   * Make a new message. Created by the caller
   * @param rdtsc
   * @param fmt
   * @param fmt_args
   */
  Message(uint64_t rdtsc, LogLineInfo const* log_line_info, FmtArgs&&... fmt_args)
    : _fmt_args(std::make_tuple(std::forward<FmtArgs>(fmt_args)...)), _rdtsc(rdtsc), _log_line_info(log_line_info)
  {
  }

  /**
   * Destructor
   */
  ~Message() override = default;

  /**
   * @return the size of the object
   */
  [[nodiscard]] size_t size() const noexcept override { return sizeof(*this); }

  /**
   * Get the rdtsc timestamp
   * @note Called on the logger thread
   */
  [[nodiscard]] uint64_t rdtsc() const noexcept override { return _rdtsc; }

private:
  tuple_t _fmt_args;
  uint64_t _rdtsc;
  LogLineInfo const* _log_line_info;
};
} // namespace quill::detail