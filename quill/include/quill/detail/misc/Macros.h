/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <cstdio>
#include <cstdlib>

/**
 * Convert number to string
 */
#define QUILL_AS_STR(x) #x
#define QUILL_STRINGIFY(x) QUILL_AS_STR(x)

/**
 * Likely
 */
#if defined(__GNUC__)
  #define QUILL_LIKELY(x) (__builtin_expect((x), 1))
  #define QUILL_UNLIKELY(x) (__builtin_expect((x), 0))
#else
  #define QUILL_LIKELY(x) (x)
  #define QUILL_UNLIKELY(x) (x)
#endif

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
