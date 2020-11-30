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
#include "quill/detail/serialize/TypeDescriptor.h"
#include <cstdint>
#include <string>

namespace quill
{
namespace detail
{
/**
 * Deserializes a single argument from the buffer
 * @param read_buffer current buffer to read
 * @param fmt_store fmt_store reference to write the argument from the read_buffer
 * @param type_descriptor type descriptor of the argument we are about to read
 * @return the number of bytes read from the buffer
 */
QUILL_NODISCARD size_t deserialize_argument(unsigned char const*& read_buffer,
                                            fmt::dynamic_format_arg_store<fmt::format_context>& fmt_store,
                                            TypeDescriptor type_descriptor);
} // namespace detail
} // namespace quill