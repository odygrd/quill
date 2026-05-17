#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/BinaryDataDeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Utility.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"
#include "quill/sinks/FileSink.h"

#include <array>
#include <string>
#include <vector>

using namespace quill;

struct BinaryPayloadTag
{
};

using TaggedBinaryData = quill::BinaryData<BinaryPayloadTag>;

template <>
struct quill::Codec<TaggedBinaryData> : quill::BinaryDataDeferredFormatCodec<TaggedBinaryData>
{
};

struct BinaryEnvelope
{
  TaggedBinaryData payload;
  std::string trailer;
};

template <>
struct fmtquill::formatter<BinaryEnvelope>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::BinaryEnvelope const& envelope, FormatContext& ctx) const
  {
    return fmtquill::format_to(
      ctx.out(), "payload={}, trailer={}",
      quill::utility::to_hex(envelope.payload.data(), envelope.payload.size(), false, false),
      envelope.trailer);
  }
};

template <>
struct quill::Codec<BinaryEnvelope>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     ::BinaryEnvelope const& envelope) noexcept
  {
    return compute_total_encoded_size(conditional_arg_size_cache, envelope.payload, envelope.trailer);
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, ::BinaryEnvelope const& envelope) noexcept
  {
    encode_members(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                   envelope.payload, envelope.trailer);
  }

  static ::BinaryEnvelope decode_arg(std::byte*& buffer)
  {
    std::array<std::byte, 16> payload_storage{};
    ::BinaryEnvelope envelope{TaggedBinaryData{payload_storage.data(), 0u}, {}};
    decode_members(buffer, envelope, envelope.payload, envelope.trailer);
    return envelope;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

/***/
TEST_CASE("binary_data_codec_composes_with_following_members")
{
  static constexpr char const* filename = "binary_data_codec_composes_with_following_members.log";
  static std::string const logger_name = "binary_envelope_logger";

  Backend::start();

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(
    logger_name, std::move(file_sink),
    PatternFormatterOptions{
      "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) "
      "%(logger:<22) %(message)"});

  std::array<std::byte, 4> payload{std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};

  LOG_INFO(logger, "{}", BinaryEnvelope{TaggedBinaryData{payload.data(), payload.size()}, "done"});

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 1);
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"payload=deadbeef, trailer=done"}));

  testing::remove_file(filename);
}
