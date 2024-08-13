/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#ifndef QUILL_BEGIN_NAMESPACE
  #define QUILL_BEGIN_NAMESPACE                                                                    \
    namespace quill                                                                                \
    {                                                                                              \
    inline namespace v7                                                                            \
    {
  #define QUILL_END_NAMESPACE                                                                      \
    }                                                                                              \
    }
#endif

/**
 * __has_attribute
 */
#ifdef __has_attribute
  #define QUILL_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
  #define QUILL_HAS_ATTRIBUTE(x) 0
#endif

/**
 * __has_cpp_attribute
 */
#if defined(__cplusplus) && defined(__has_cpp_attribute)
  #define QUILL_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
  #define QUILL_HAS_CPP_ATTRIBUTE(x) 0
#endif

/**
 * Portable no discard warnings
 */
#if QUILL_HAS_CPP_ATTRIBUTE(nodiscard)
  #define QUILL_NODISCARD [[nodiscard]]
#elif QUILL_HAS_CPP_ATTRIBUTE(gnu::warn_unused_result)
  #define QUILL_NODISCARD [[gnu::warn_unused_result]]
#else
  #define QUILL_NODISCARD
#endif

/**
 * Portable maybe_unused
 */
#if QUILL_HAS_CPP_ATTRIBUTE(maybe_unused) && (defined(_HAS_CXX17) && _HAS_CXX17 == 1)
  #define QUILL_MAYBE_UNUSED [[maybe_unused]]
#elif QUILL_HAS_ATTRIBUTE(__unused__) || defined(__GNUC__)
  #define QUILL_MAYBE_UNUSED __attribute__((__unused__))
#elif defined(_MSC_VER)
  #define QUILL_MAYBE_UNUSED __pragma(warning(suppress : 4100))
#else
  #define QUILL_MAYBE_UNUSED
#endif

/**
 * Gcc hot/cold attributes
 * Tells GCC that a function is hot or cold. GCC can use this information to
 * improve static analysis, i.e. a conditional branch to a cold function
 * is likely to be not-taken.
 */
#if QUILL_HAS_ATTRIBUTE(hot) || (defined(__GNUC__) && !defined(__clang__))
  #define QUILL_ATTRIBUTE_HOT __attribute__((hot))
#else
  #define QUILL_ATTRIBUTE_HOT
#endif

#if QUILL_HAS_ATTRIBUTE(cold) || (defined(__GNUC__) && !defined(__clang__))
  #define QUILL_ATTRIBUTE_COLD __attribute__((cold))
#else
  #define QUILL_ATTRIBUTE_COLD
#endif

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
 * Visibility
 */
#if defined(_WIN32)
  #if defined(QUILL_DLL_EXPORT)
    #define QUILL_EXPORT __declspec(dllexport) // Exporting symbols when building the library
  #elif defined(QUILL_DLL_IMPORT)
    #define QUILL_EXPORT __declspec(dllimport) // Importing symbols when using the library
  #else
    #define QUILL_EXPORT // No special attribute needed for static or other builds
  #endif
#elif defined(__GNUC__) || defined(__clang__)
  #define QUILL_EXPORT                                                                             \
    __attribute__((visibility("default"))) // Using GCC/Clang visibility attribute
#else
  #define QUILL_EXPORT // Default for other compilers
#endif