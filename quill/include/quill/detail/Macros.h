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
