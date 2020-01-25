#pragma once

#include <cstddef>
#include <string>

namespace quill
{
[[nodiscard]] std::string to_hex(unsigned char* buffer, size_t size) noexcept;

[[nodiscard]] std::string to_hex(char* buffer, size_t size) noexcept;
} // namespace quill