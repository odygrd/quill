#pragma once

#include <cstdint>
#include <tuple>

#include <x86intrin.h>

#include "quill/detail/LogLineInfo.h"
#include "quill/detail/MessageHelpers.h"

#include <iostream> // todo:: remove me

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
   * Process a message
   */
  void virtual process(uint32_t thread_id) const noexcept = 0;

private:
  uint64_t _rdtsc{__rdtsc()};
};

/**
 * For each log statement a message is produced and pushed to the thread local spsc queue.
 * The logging thread will retrieve the messages from the queue using the base class pointer.
 * @tparam TPromotedTuple
 * @tparam U This is unused for this message. We need it for the custom command message
 */
template <typename... FmtArgs>
class Message final : public MessageBase
{
public:
  using PromotedTupleT = std::tuple<PromotedTypeT<FmtArgs>...>;

  /**
   * Make a new message. Created by the caller
   * @param rdtsc
   * @param fmt
   * @param fmt_args
   */
  explicit Message(LogLineInfo const* log_line_info, FmtArgs&&... fmt_args)
    : _log_line_info(log_line_info), _fmt_args(std::make_tuple(std::forward<FmtArgs>(fmt_args)...))
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
   * Process a message
   */
  void process(uint32_t thread_id) const noexcept
  {
    std::cout << thread_id << " " << rdtsc() << std::endl;
  }

private:
  LogLineInfo const* _log_line_info;
  PromotedTupleT _fmt_args;
};

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

  void process(uint32_t) const noexcept {}

  /**
   * Destructor
   */
  ~CommandMessage() override = default;

private:
  std::function<void()> _callback;
  mutable bool _processed{false};
};
} // namespace quill::detail