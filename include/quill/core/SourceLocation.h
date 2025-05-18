#pragma once

/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include <cstdint>

QUILL_BEGIN_NAMESPACE

struct SourceLocation
{
  static constexpr SourceLocation current(const char* file = __builtin_FILE(),
                                          const char* function = __builtin_FUNCTION(),
                                          std::uint_least32_t line = __builtin_LINE()) noexcept
  {
    return SourceLocation{file, function, line};
  }

  constexpr SourceLocation(const char* file, const char* function, std::uint_least32_t line)
    : _file(file), _function(function), _line(line)
  {
  }

  QUILL_NODISCARD constexpr const char* file_name() const noexcept { return _file; }
  QUILL_NODISCARD constexpr const char* function_name() const noexcept { return _function; }
  QUILL_NODISCARD constexpr std::uint_least32_t line() const noexcept { return _line; }

private:
  const char* _file;
  const char* _function;
  std::uint_least32_t _line;
};

QUILL_END_NAMESPACE