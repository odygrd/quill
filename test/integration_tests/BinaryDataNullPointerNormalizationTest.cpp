#include "doctest/doctest.h"

#include "quill/BinaryDataDeferredFormatCodec.h"

struct BinaryPayloadTag
{
};

using TaggedBinaryData = quill::BinaryData<BinaryPayloadTag>;

/***/
TEST_CASE("binary_data_null_pointer_with_nonzero_size_is_normalized")
{
  TaggedBinaryData const binary_data{static_cast<std::byte const*>(nullptr), 8u};

  REQUIRE_EQ(binary_data.data(), nullptr);
  REQUIRE_EQ(binary_data.size(), 0u);
}
