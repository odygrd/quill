#pragma once

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
 * Always Inline
 */
// always inline
#if defined(__GNUC__)
  #define QUILL_ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
  #define QUILL_ALWAYS_INLINE __forceinline
#else
  #define QUILL_ALWAYS_INLINE inline
#endif

/**
 * Convert number to string
 */
#define QUILL_AS_STR(x) #x
#define QUILL_STRINGIFY(x) QUILL_AS_STR(x)

/**
 * C++ language standard detection
 */
#if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_HAS_CXX17) && _HAS_CXX17 == 1) // fix for issue #464
  #define QUILL_HAS_CPP_17
  #define QUILL_HAS_CPP_14
#elif (defined(__cplusplus) && __cplusplus >= 201402L) || (defined(_HAS_CXX14) && _HAS_CXX14 == 1)
  #define QUILL_HAS_CPP_14
#endif

/**
 * Portable nodiscard warnings
 */
#if defined(__has_cpp_attribute)
  #if __has_cpp_attribute(nodiscard)
    #if defined(__clang__) && !defined(QUILL_HAS_CPP_17)
      #define QUILL_NODISCARD
    #else
      #define QUILL_NODISCARD [[nodiscard]]
    #endif
  #elif __has_cpp_attribute(gnu::warn_unused_result)
    #define QUILL_NODISCARD [[gnu::warn_unused_result]]
  #else
    #define QUILL_NODISCARD
  #endif
#else
  #define QUILL_NODISCARD
#endif