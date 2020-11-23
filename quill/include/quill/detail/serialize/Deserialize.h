/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/ThreadContext.h"
#include "quill/detail/misc/Attributes.h"
#include "quill/detail/serialize/SerializationMetadata.h"
#include <cstdint>
#include <string>

namespace quill
{
namespace detail
{
/**
 * Represents a full deserialized message from the queue
 */
struct LogDeserialized
{
  uint64_t timestamp{0};
  LoggerDetails const* logger_details{nullptr};
  SerializationMetadata const* serialization_metadata{nullptr};
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
};

/**
 * Deserializes a single argument from the buffer
 * @param read_buffer
 * @param fmt_store
 * @param type_descriptor
 * @return
 */
QUILL_NODISCARD size_t deserialize_argument(unsigned char const*& read_buffer,
                                            fmt::dynamic_format_arg_store<fmt::format_context>& fmt_store,
                                            std::string const& type_descriptor);
} // namespace detail
} // namespace quill