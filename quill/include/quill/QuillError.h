/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/detail/misc/Macros.h"
#include <exception>
#include <string>

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

namespace quill
{
/**
 * custom exception
 */
class QuillError : public std::exception
{
public:
  explicit QuillError(std::string s) : _error(std::move(s)) {}
  explicit QuillError(char const* s) : _error(s) {}

  char const* what() const noexcept override { return _error.data(); }
private:
  std::string _error;
};

} // namespace quill
