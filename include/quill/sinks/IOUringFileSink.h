/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/Filesystem.h"
#include "quill/sinks/FileSink.h"

#include <liburing.h>

#include <array>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

QUILL_BEGIN_NAMESPACE

/**
 * On Rhel9
 *   sudo dnf install liburing liburing-devel
 *   sudo sysctl kernel.io_uring_disabled=0
 */
class IOUringFileSink : public FileSink
{
public:
  explicit IOUringFileSink(fs::path const& filename, FileSinkConfig const& config = FileSinkConfig{},
                           FileEventNotifier file_event_notifier = FileEventNotifier{}, bool do_fopen = true,
                           std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now())
    : FileSink(filename, config, std::move(file_event_notifier), do_fopen, start_time)
  {
    _fd = fileno(_file);
    std::fill(_pending_writes_per_chunk.begin(), _pending_writes_per_chunk.end(), false);

    if (::io_uring_queue_init(_num_chunks, &_ring, 0u) != 0)
    {
      QUILL_THROW(QuillError{std::string{"Failed to initialize io_uring. error: "} + strerror(-errno)});
    }

    // Register fixed-size buffer chunks with io_uring
    std::vector<iovec> iovecs;
    iovecs.resize(_num_chunks);

    for (std::size_t i = 0; i < _num_chunks; ++i)
    {
      iovecs[i].iov_base = _buffer + i * _chunk_size;
      iovecs[i].iov_len = _chunk_size;
    }

    if (::io_uring_register_buffers(&_ring, iovecs.data(), _num_chunks) != 0)
    {
      QUILL_THROW(QuillError{std::string{"Failed to register buffers. error: "} + strerror(-errno)});
    }
  }

  /**
   * Destructor
   */
  ~IOUringFileSink() override { ::io_uring_queue_exit(&_ring); }

  /**
   * @brief Writes a formatted log message to the stream
   */
  void write_log(MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                 std::string const&, std::string_view, LogLevel, std::string_view, std::string_view,
                 std::vector<std::pair<std::string, std::string>> const*, std::string_view,
                 std::string_view log_statement) override
  {
    // If the current log statement does not fit into the remaining buffer then use the other
    if (_buffer_pos + log_statement.size() > _chunk_size)
    {
      _submit_and_advance_buffer();

      // Wait for this buffer completion if it has pending writes
      _wait_for_completion(_current_buffer);
    }

    // Copy log statement into the buffer
    std::memcpy(_buffer + (_current_buffer * _chunk_size) + _buffer_pos, log_statement.data(),
                log_statement.size());
    _buffer_pos += log_statement.size();
  }

  void flush_sink() override
  {
    if (_buffer_pos == 0)
    {
      // Nothing to flush
      return;
    }

    // Submit any remaining data
    _submit_and_advance_buffer();

    // Wait for all completions
    _wait_for_all_completions();
  }

private:
  void _submit_and_advance_buffer()
  {
    _submit_message(_current_buffer, _buffer_pos);

    _pending_writes_per_chunk[_current_buffer] = true;

    // Switch to the next buffer
    _current_buffer = (_current_buffer + 1u) % _num_chunks;

    _buffer_pos = 0;
  }

  void _submit_message(uint8_t buffer_id, size_t length)
  {
    io_uring_sqe* sqe = ::io_uring_get_sqe(&_ring);

    if (QUILL_UNLIKELY(!sqe))
    {
      QUILL_THROW(QuillError("Failed to get SQE, SQ ring is full"));
    }

    ::io_uring_prep_write_fixed(sqe, _fd, _buffer + buffer_id * _chunk_size, length, _file_offset, buffer_id);

    ::io_uring_sqe_set_data64(sqe, _current_buffer);
    _file_offset += length;

    ::io_uring_submit(&_ring);
  }

  void _wait_for_all_completions()
  {
    for (size_t i = 0; i < _num_chunks; ++i)
    {
      _wait_for_completion(i);
    }
  }

  void _wait_for_completion(size_t buffer_id)
  {
    while (_pending_writes_per_chunk[buffer_id])
    {
      io_uring_cqe* cqe;

      int ret = ::io_uring_wait_cqe(&_ring, &cqe);

      if (QUILL_UNLIKELY(ret < 0))
      {
        QUILL_THROW(QuillError{std::string("io_uring_wait_cqe failed: ") + std::strerror(-ret)});
      }

      // Check ceq event result
      if (QUILL_UNLIKELY(cqe->res < 0))
      {
        QUILL_THROW(QuillError{std::string("Write failed: ") + std::strerror(-cqe->res)});
      }

      auto const completed_buffer = static_cast<uint8_t>(io_uring_cqe_get_data64(cqe));

      _pending_writes_per_chunk[completed_buffer] = false;

      ::io_uring_cqe_seen(&_ring, cqe);
    }
  }

private:
  static constexpr std::size_t _num_chunks = 4;
  static constexpr std::size_t _chunk_size = 32'768;

  alignas(64) char _buffer[_chunk_size * _num_chunks];
  ::io_uring _ring;
  std::size_t _buffer_pos{0};
  std::size_t _file_offset{0};
  int _fd{0};
  uint8_t _current_buffer{0};
  std::array<bool, _num_chunks> _pending_writes_per_chunk{};
};

QUILL_END_NAMESPACE