/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/LogMacroMetadata.h"
#include "quill/detail/serialize/TypeDescriptor.h"
#include <string>

namespace quill
{
namespace detail
{

/**
 * A struct that contains a string with the serialization representation and the LogMacroMetadata
 */
struct SerializationMetadata
{
  SerializationMetadata(std::string in_serialization_info, LogMacroMetadata const& in_metadata)
    : serialization_info(std::move(in_serialization_info)), log_macro_metadata(in_metadata)
  {
  }

  std::string serialization_info;
  LogMacroMetadata log_macro_metadata;
};

/**
 * Creates and returns a static SerializeMetadata pointer during program init
 * @return A pointer to SerializationMetadata
 */
template <typename F, typename... Args>
inline SerializationMetadata const* get_serialization_metadata_ptr()
{
  // create a static SerializeMetadata object
  // we pass a string with the serialization info and the LogMacroMetadata from the anonymous struct
  static SerializationMetadata serialization_metadata{type_descriptor_string<std::decay_t<Args>...>(), F{}()};
  return std::addressof(serialization_metadata);
}

/**
 * A wrapper around SerializeMetadata.
 */
struct SerializationMetadataWrapper
{
  explicit SerializationMetadataWrapper(SerializationMetadata const* in_serialization_metadata)
    : serialization_metadata(in_serialization_metadata)
  {
  }

  SerializationMetadata const* serialization_metadata{nullptr};
};

/**
 * A variable template that will call get_log_data_node_ptr during the program initialisation time
 */
template <typename TLogMarcoMetadata, typename... Args>
SerializationMetadataWrapper seriallization_metadata{
  get_serialization_metadata_ptr<TLogMarcoMetadata, Args...>()};
} // namespace detail
} // namespace quill