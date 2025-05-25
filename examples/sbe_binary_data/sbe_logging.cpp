#include "quill/Backend.h"
#include "quill/BinaryDataDeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/Utility.h"
#include "quill/sinks/ConsoleSink.h"

#include <array>
#include <iostream>
#include <sstream>
#include <utility>

#include "sample/CancelOrder.h"
#include "sample/NewOrder.h"

/**
 * @brief Efficient binary data logging with deferred formatting
 *
 * This example demonstrates how to efficiently log variable-sized binary data such as network messages
 * or binary protocol buffers (like SBE messages) using the library's deferred formatting capabilities.
 *
 * ## Key Benefits
 *
 * - **Performance**: Only performs a memory copy on the hot path (critical logging section)
 * - **Deferred Formatting**: All parsing and formatting happen in the background logging thread
 * - **Human-Readable Output**: Formats binary data appropriately for log files
 *
 * ## How It Works
 *
 * 1. Create a tag struct to identify your binary protocol type
 * 2. Define a type alias using `quill::BinaryData<YourTag>`
 * 3. Implement a formatter for your binary data type
 * 4. Specialize `quill::Codec` to use `quill::BinaryDataDeferredFormatCodec`
 * 5. Log your binary data using the type alias
 *
 * This example uses SBE (Simple Binary Encoding) messages as a demonstration.
 *
 * SBE is a binary encoding protocol often used in financial systems.
 * (See https://github.com/aeron-io/simple-binary-encoding for more information)
 */

/**
 * Step 1: Create a tag struct to give semantic meaning to your binary protocol
 * This empty struct acts as a compile-time tag to identify the type of binary data.
 * You can create different tags for different protocols or message formats.
 */
struct TradingProtocol
{
};

/**
 * Step 2: Create a BinaryData specialization for your protocol
 * This type alias will be used when logging binary data of this specific protocol.
 */
using TradingProtocolData = quill::BinaryData<TradingProtocol>;

/**
 * Step 3: Implement a formatter for your binary data type
 * This formatter is called in the backend thread to convert binary data to a human-readable text.
 * Since the library always writes to human-readable log files, you must format binary data
 * appropriately.
 */
template <>
struct fmtquill::formatter<TradingProtocolData>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::TradingProtocolData const& bin_data, format_context& ctx) const
  {
    // Option 1: Convert binary data to hexadecimal representation
    // Useful when you don't need to parse the structure
    // return fmtquill::format_to(ctx.out(), "{}", quill::utility::to_hex(bin_data.data(), bin_data.size()));

    // Option 2: Parse the binary data into a meaningful representation
    // Get raw data pointer in the format expected by the SBE API
    char* data = reinterpret_cast<char*>(const_cast<std::byte*>(bin_data.data()));
    std::stringstream oss;

    // Parse the SBE message header to determine message type
    sbe::sample::MessageHeader header{data, bin_data.size()};

    if (header.templateId() == sbe::sample::NewOrder::sbeTemplateId())
    {
      sbe::sample::NewOrder msg;
      msg.wrapForDecode(data, sbe::sample::MessageHeader::encodedLength(), header.blockLength(),
                        header.version(), bin_data.size());
      oss << msg;
    }
    else if (header.templateId() == sbe::sample::CancelOrder::sbeTemplateId())
    {
      sbe::sample::CancelOrder msg;
      msg.wrapForDecode(data, sbe::sample::MessageHeader::encodedLength(), header.blockLength(),
                        header.version(), bin_data.size());
      oss << msg;
    }

    return fmtquill::format_to(ctx.out(), "{}", oss.str());
  }
};

/**
 * Step 4: Specialize the Codec for your binary data type
 */
template <>
struct quill::Codec<TradingProtocolData> : quill::BinaryDataDeferredFormatCodec<TradingProtocolData>
{
};

int main()
{
  // Initialize Quill backend
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Create a console sink and logger
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "trading", std::move(console_sink),
    quill::PatternFormatterOptions{"[%(time)] %(message)", "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

  // Sample data for our demonstration
  std::array<std::string, 9> symbols = {"AAPL", "MSFT", "AMZN", "GOOGL", "META",
                                        "TSLA", "NVDA", "PYPL", "NFLX"};

  std::array<std::string, 4> cancel_reasons = {"User requested", "Risk limit exceeded",
                                               "Price away from market", "Timeout"};

  // Buffer for encoding SBE messages
  std::array<char, 128> buffer{};

  // Example 1: Log new order messages
  for (uint32_t i = 0; i < symbols.size(); ++i)
  {
    sbe::sample::NewOrder order;
    order.wrapAndApplyHeader(buffer.data(), 0, buffer.size());
    order.orderId(i);
    order.price(10000 + i * 1000);
    order.quantity(1000 + i * 500);
    order.side(i % 2 == 0 ? sbe::sample::Side::BUY : sbe::sample::Side::SELL);
    order.putSymbol(symbols[i]);

    size_t const encoded_size = order.encodedLength() + sbe::sample::MessageHeader::encodedLength();

    // Step 5: Log the binary data using TradingProtocolData
    // Only a memcpy happens here (on the hot path)
    // The actual formatting will be deferred to the backend thread
    LOG_INFO(logger, "[SEND] {}", TradingProtocolData{reinterpret_cast<uint8_t*>(buffer.data()), encoded_size});
  }

  // Log cancel order messages for some orders
  for (uint32_t i = 0; i < 5; ++i)
  {
    sbe::sample::CancelOrder cancel;
    cancel.wrapAndApplyHeader(buffer.data(), 0, buffer.size());
    cancel.orderId(2000000 + i);
    cancel.origOrderId(i);
    cancel.cancelQuantity(500 + i * 100);
    cancel.putReason(cancel_reasons[i % cancel_reasons.size()]);

    size_t const encoded_size = cancel.encodedLength() + sbe::sample::MessageHeader::encodedLength();

    // Step 5: Log using the same pattern - formatting happens in the backend thread
    LOG_INFO(logger, "[SEND] {}", TradingProtocolData{reinterpret_cast<uint8_t*>(buffer.data()), encoded_size});
  }

  return 0;
}
