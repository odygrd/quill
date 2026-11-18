/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

/**
 * Library version, e.g. `#if QUILL_VERSION >= 140000`. The inline namespace follows the major
 * version so that different major versions linked into one program do not clash.
 */
#define QUILL_VERSION_MAJOR 14
#define QUILL_VERSION_MINOR 0
#define QUILL_VERSION_PATCH 0
#define QUILL_VERSION                                                                              \
  (QUILL_VERSION_MAJOR * 10000 + QUILL_VERSION_MINOR * 100 + QUILL_VERSION_PATCH)

// `v` is pasted directly, so a macro named `v` in user code cannot rename the namespace
#define QUILL_DETAIL_VERSION_NAMESPACE_IMPL(major) v##major
#define QUILL_DETAIL_VERSION_NAMESPACE(major) QUILL_DETAIL_VERSION_NAMESPACE_IMPL(major)

#ifndef QUILL_BEGIN_NAMESPACE
  #define QUILL_BEGIN_NAMESPACE                                                                    \
    namespace quill                                                                                \
    {                                                                                              \
    inline namespace QUILL_DETAIL_VERSION_NAMESPACE(QUILL_VERSION_MAJOR)                           \
    {
  #define QUILL_END_NAMESPACE                                                                      \
    }                                                                                              \
    }
#endif

/**
 * __has_feature
 */
#ifndef QUILL_HAS_FEATURE
  #ifdef __has_feature
    #define QUILL_HAS_FEATURE(x) __has_feature(x)
  #else
    #define QUILL_HAS_FEATURE(x) 0
  #endif
#endif

/**
 * __has_attribute
 */
#ifndef QUILL_HAS_ATTRIBUTE
  #ifdef __has_attribute
    #define QUILL_HAS_ATTRIBUTE(x) __has_attribute(x)
  #else
    #define QUILL_HAS_ATTRIBUTE(x) 0
  #endif
#endif

/** C++17 attributes, kept as macros for existing users */
#ifndef QUILL_NODISCARD
  #define QUILL_NODISCARD [[nodiscard]]
#endif

#ifndef QUILL_MAYBE_UNUSED
  #define QUILL_MAYBE_UNUSED [[maybe_unused]]
#endif

/**
 * Prevents the compiler from inlining a function
 */
#ifndef QUILL_NOINLINE
  #if QUILL_HAS_ATTRIBUTE(noinline)
    #define QUILL_NOINLINE __attribute__((noinline))
  #elif defined(_MSC_VER)
    #define QUILL_NOINLINE __declspec(noinline)
  #else
    #define QUILL_NOINLINE
  #endif
#endif

/**
 * Gcc hot/cold attributes
 * Tells GCC that a function is hot or cold. GCC can use this information to
 * improve static analysis, i.e. a conditional branch to a cold function
 * is likely to be not-taken.
 */
#ifndef QUILL_ATTRIBUTE_HOT
  #if QUILL_HAS_ATTRIBUTE(hot)
    #define QUILL_ATTRIBUTE_HOT __attribute__((hot))
  #else
    #define QUILL_ATTRIBUTE_HOT
  #endif
#endif

#ifndef QUILL_ATTRIBUTE_COLD
  #if QUILL_HAS_ATTRIBUTE(cold)
    #define QUILL_ATTRIBUTE_COLD __attribute__((cold))
  #else
    #define QUILL_ATTRIBUTE_COLD
  #endif
#endif

/**
 * Used
 */
#ifndef QUILL_ATTRIBUTE_USED
  #if QUILL_HAS_ATTRIBUTE(used)
    #define QUILL_ATTRIBUTE_USED __attribute__((used))
  #else
    #define QUILL_ATTRIBUTE_USED
  #endif
#endif

/**
 * Tells the compiler that a condition is always true, enabling optimisations
 * that rely on that guarantee.  Behaviour is undefined if the condition is false.
 */
#ifndef QUILL_ASSUME
  #if defined(__clang__)
    #define QUILL_ASSUME(x) __builtin_assume(x)
  #elif defined(__GNUC__)
    #define QUILL_ASSUME(x)                                                                        \
      do                                                                                           \
      {                                                                                            \
        if (!(x))                                                                                  \
          __builtin_unreachable();                                                                 \
      } while (0)
  #elif defined(_MSC_VER)
    #define QUILL_ASSUME(x) __assume(x)
  #else
    #define QUILL_ASSUME(x) ((void)0)
  #endif
#endif

#ifndef QUILL_LIKELY
  #if defined(__GNUC__)
    #define QUILL_LIKELY(x) (__builtin_expect((x), 1))
  #else
    #define QUILL_LIKELY(x) (x)
  #endif
#endif

#ifndef QUILL_UNLIKELY
  #if defined(__GNUC__)
    #define QUILL_UNLIKELY(x) (__builtin_expect((x), 0))
  #else
    #define QUILL_UNLIKELY(x) (x)
  #endif
#endif

/**
 * Visibility
 */
#ifndef QUILL_EXPORT
  #if defined(_WIN32) && defined(_MSC_VER)
    #if defined(QUILL_DLL_EXPORT)
      #define QUILL_EXPORT __declspec(dllexport) // Exporting symbols when building the library
    #elif defined(QUILL_DLL_IMPORT)
      #define QUILL_EXPORT __declspec(dllimport) // Importing symbols when using the library
    #else
      #define QUILL_EXPORT // No special attribute needed for static or other builds
    #endif
  #elif defined(__GNUC__) || defined(__clang__)
    #define QUILL_EXPORT                                                                           \
      __attribute__((visibility("default"))) // Using GCC/Clang visibility attribute
  #else
    #define QUILL_EXPORT // Default for other compilers
  #endif
#endif

/**
 * Module export helpers
 */
#ifndef QUILL_BEGIN_EXPORT
  #if defined(QUILL_MODULE)
    #define QUILL_BEGIN_EXPORT                                                                     \
      export                                                                                       \
      {
    #define QUILL_END_EXPORT }
  #else
    #define QUILL_BEGIN_EXPORT
    #define QUILL_END_EXPORT
  #endif
#endif

/**
 * Check RTTI is available
 */
#ifndef QUILL_USE_RTTI
  // __RTTI is for EDG compilers. _CPPRTTI is for MSVC.
  #if defined(__GXX_RTTI) || QUILL_HAS_FEATURE(cxx_rtti) || defined(_CPPRTTI) ||                   \
    defined(__INTEL_RTTI__) || defined(__RTTI)
    #define QUILL_USE_RTTI 1
  #else
    #define QUILL_USE_RTTI 0
  #endif
#endif

/**
 * Thread-local storage specifier
 */
#if !defined(QUILL_THREAD_LOCAL)
  #if defined(__GNUC__) && defined(__linux__)
    #define QUILL_THREAD_LOCAL __thread
  #else
    #define QUILL_THREAD_LOCAL thread_local
  #endif
#endif
