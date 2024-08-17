/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"

#include <exception>
#include <string>

/**
 * Require check
 */
#define QUILL_REQUIRE(expression, error)                                                           \
  do                                                                                               \
  {                                                                                                \
    if (QUILL_UNLIKELY(!(expression)))                                                             \
    {                                                                                              \
      printf("Quill fatal error: %s (%s:%d)\n", error, __FILE__, __LINE__);                        \
      std::abort();                                                                                \
    }                                                                                              \
  } while (0)

#if defined(QUILL_NO_EXCEPTIONS)
  #define QUILL_TRY if (true)
  #define QUILL_THROW(ex) QUILL_REQUIRE(false, ex.what())
  #define QUILL_CATCH(x) if (false)
  #define QUILL_CATCH_ALL() if (false)
#else
  #define QUILL_TRY try
  #define QUILL_THROW(ex) throw(ex)
  #define QUILL_CATCH(x) catch (x)
  #define QUILL_CATCH_ALL() catch (...)
#endif

QUILL_BEGIN_NAMESPACE

/**
 * custom exception
 */
class QuillError : public std::exception
{
public:
  explicit QuillError(std::string s) : _error(static_cast<std::string&&>(s)) {}
  explicit QuillError(char const* s) : _error(s) {}

  QUILL_NODISCARD char const* what() const noexcept override { return _error.data(); }

private:
  std::string _error;
};

QUILL_END_NAMESPACE