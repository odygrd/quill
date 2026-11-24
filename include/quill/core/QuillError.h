/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"

#ifndef QUILL_MODULE
  #include <exception>
  #include <string>
#endif

#if defined(QUILL_NO_EXCEPTIONS)
  #ifndef QUILL_MODULE
    #include <cstdio>
    #include <cstdlib>
  #endif

  #define QUILL_REQUIRE(expression, error)                                                         \
    do                                                                                             \
    {                                                                                              \
      if (QUILL_UNLIKELY(!(expression)))                                                           \
      {                                                                                            \
        /* stderr is unbuffered so the message is not lost when abort() skips stream flushing */   \
        std::fprintf(stderr, "Quill fatal error: %s\n", error);                                    \
        std::abort();                                                                              \
      }                                                                                            \
    } while (0)

  #define QUILL_TRY if (true)
  #define QUILL_THROW(ex) QUILL_REQUIRE(false, ex.what())
  #define QUILL_CATCH(x) if (false)
  #define QUILL_CATCH_ALL() if (false)

  /**
   * For an I/O failure of a sink that the backend survives, such as a write to a full disk. With
   * exceptions these throw `ex`, which the backend passes to error_notifier. Without them, on the
   * backend thread while it works, they report the message through error_notifier and return
   * from the function, `value` from one that returns something. The caller continues, so a
   * function that fails this way tells its caller. On any other thread they end the process, as
   * QUILL_THROW does, except where the failures are discarded, as the destructor of FileSink does.
   */
  #define QUILL_THROW_OR_RETURN(ex)                                                                \
    do                                                                                             \
    {                                                                                              \
      ::quill::detail::report_error(ex.what());                                                    \
      return;                                                                                      \
    } while (0)

  #define QUILL_THROW_OR_RETURN_VALUE(ex, value)                                                   \
    do                                                                                             \
    {                                                                                              \
      ::quill::detail::report_error(ex.what());                                                    \
      return value;                                                                                \
    } while (0)
#else
  #define QUILL_TRY try
  #define QUILL_THROW(ex) throw(ex)
  #define QUILL_CATCH(x) catch (x)
  #define QUILL_CATCH_ALL() catch (...)
  #define QUILL_THROW_OR_RETURN(ex) throw(ex)
  #define QUILL_THROW_OR_RETURN_VALUE(ex, value) throw(ex)
#endif

QUILL_BEGIN_NAMESPACE

#if defined(QUILL_NO_EXCEPTIONS)
namespace detail
{
/** Takes the failures of QUILL_THROW_OR_RETURN. The backend sets one for its thread while it works */
struct ErrorReceiver
{
  void (*report)(void* context, char const* message);
  void* context;
};

// The exported accessor keeps its thread-local state shared with DLL consumers.
QUILL_EXPORT inline ErrorReceiver const*& error_receiver() noexcept
{
  static QUILL_THREAD_LOCAL ErrorReceiver const* receiver{nullptr};
  return receiver;
}

inline void report_error(char const* message)
{
  ErrorReceiver const* const receiver = error_receiver();
  QUILL_REQUIRE(receiver != nullptr, message);
  receiver->report(receiver->context, message);
}
} // namespace detail
#endif

QUILL_BEGIN_EXPORT

/**
 * custom exception
 */
class QuillError : public std::exception
{
public:
  explicit QuillError(std::string s) : _error(static_cast<std::string&&>(s)) {}
  explicit QuillError(char const* s) : _error(s) {}

  QUILL_NODISCARD char const* what() const noexcept override { return _error.c_str(); }

private:
  std::string _error;
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
