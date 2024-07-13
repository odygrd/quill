/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"

/**
 * Allows compile-time filtering of log messages to completely compile out log levels,
 * resulting in zero-cost logging.
 *
 * Macros like LOG_TRACE_L3(..), LOG_TRACE_L2(..) will expand to empty statements,
 * reducing branches in compiled code and the number of MacroMetadata constexpr instances.
 *
 * The default value of -1 enables all log levels.
 * Specify a logging level to disable all levels equal to or higher than the specified level.
 *
 * For example to only log warnings and above you can use:
 *   add_compile_definitions(-DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)
 * or
 *   target_compile_definitions(${TARGET} PRIVATE -DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)
 **/
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3 0
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L2 1
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L1 2
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG 3
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO 4
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING 5
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR 6
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL 7

#if !defined(QUILL_COMPILE_ACTIVE_LOG_LEVEL)
  #define QUILL_COMPILE_ACTIVE_LOG_LEVEL -1
#endif

#if !defined(QUILL_IMMEDIATE_FLUSH)
  #define QUILL_IMMEDIATE_FLUSH 0
#endif

// Helper macro to expand __VA_ARGS__ correctly in MSVC
#define QUILL_EXPAND(x) x

/** -- LOGV_ helpers begin -- **/

// Helper macros to generate format strings with placeholders
#define QUILL_GENERATE_FORMAT_STRING_0(text) text
#define QUILL_GENERATE_FORMAT_STRING_1(text, x) text " [" #x ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_2(text, x, y) text " [" #x ": {}, " #y ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_3(text, x, y, z) text " [" #x ": {}, " #y ": {}, " #z ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_4(text, x, y, z, w)                                           \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_5(text, x, y, z, w, v)                                        \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_6(text, x, y, z, w, v, u)                                     \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_7(text, x, y, z, w, v, u, t)                                  \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_8(text, x, y, z, w, v, u, t, s)                               \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_9(text, x, y, z, w, v, u, t, s, r)                            \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_10(text, x, y, z, w, v, u, t, s, r, q)                        \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_11(text, x, y, z, w, v, u, t, s, r, q, p)                     \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_12(text, x, y, z, w, v, u, t, s, r, q, p, o)                  \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_13(text, x, y, z, w, v, u, t, s, r, q, p, o, n)               \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_14(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m)            \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_15(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l)         \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_16(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k)      \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l         \
       ": {}, " #k ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_17(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j)   \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l         \
       ": {}, " #k ": {}, " #j ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_18(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j, i) \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l         \
       ": {}, " #k ": {}, " #j ": {}, " #i ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_19(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j, i, h) \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l         \
       ": {}, " #k ": {}, " #j ": {}, " #i ": {}, " #h ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_20(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j, i, h, g) \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s  \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l          \
       ": {}, " #k ": {}, " #j ": {}, " #i ": {}, " #h ": {}, " #g ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_21(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j, i, h, g, f) \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s     \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l             \
       ": {}, " #k ": {}, " #j ": {}, " #i ": {}, " #h ": {}, " #g ": {}, " #f ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_22(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j,   \
                                        i, h, g, f, e)                                             \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s        \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l                \
       ": {}, " #k ": {}, " #j ": {}, " #i ": {}, " #h ": {}, " #g ": {}, " #f ": {}, " #e ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_23(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j,   \
                                        i, h, g, f, e, d)                                          \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s           \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l                   \
       ": {}, " #k ": {}, " #j ": {}, " #i ": {}, " #h ": {}, " #g ": {}, " #f ": {}, " #e                   \
       ": {}, " #d ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_24(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j,   \
                                        i, h, g, f, e, d, c)                                                   \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l         \
       ": {}, " #k ": {}, " #j ": {}, " #i ": {}, " #h ": {}, " #g ": {}, " #f ": {}, " #e         \
       ": {}, " #d ": {}, " #c ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_25(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j,   \
                                        i, h, g, f, e, d, c, b)                                                \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l         \
       ": {}, " #k ": {}, " #j ": {}, " #i ": {}, " #h ": {}, " #g ": {}, " #f ": {}, " #e         \
       ": {}, " #d ": {}, " #c ": {}, " #b ": {}]"
#define QUILL_GENERATE_FORMAT_STRING_26(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j,   \
                                        i, h, g, f, e, d, c, b, a)                                             \
  text " [" #x ": {}, " #y ": {}, " #z ": {}, " #w ": {}, " #v ": {}, " #u ": {}, " #t ": {}, " #s \
       ": {}, " #r ": {}, " #q ": {}, " #p ": {}, " #o ": {}, " #n ": {}, " #m ": {}, " #l         \
       ": {}, " #k ": {}, " #j ": {}, " #i ": {}, " #h ": {}, " #g ": {}, " #f ": {}, " #e         \
       ": {}, " #d ": {}, " #c ": {}, " #b ": {}, " #a ": {}]"

// Macro to select the appropriate format generator based on the number of arguments
#define QUILL_GET_FORMAT_GENERATOR_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13,   \
                                         _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, NAME, ...) \
  NAME

// Main macro to generate format strings (up to 26 variables plus the text)
#define QUILL_GENERATE_FORMAT_STRING(...)                                                              \
  QUILL_EXPAND(QUILL_GET_FORMAT_GENERATOR_MACRO(                                                       \
    __VA_ARGS__, QUILL_GENERATE_FORMAT_STRING_26, QUILL_GENERATE_FORMAT_STRING_25,                     \
    QUILL_GENERATE_FORMAT_STRING_24, QUILL_GENERATE_FORMAT_STRING_23,                                  \
    QUILL_GENERATE_FORMAT_STRING_22, QUILL_GENERATE_FORMAT_STRING_21, QUILL_GENERATE_FORMAT_STRING_20, \
    QUILL_GENERATE_FORMAT_STRING_19, QUILL_GENERATE_FORMAT_STRING_18, QUILL_GENERATE_FORMAT_STRING_17, \
    QUILL_GENERATE_FORMAT_STRING_16, QUILL_GENERATE_FORMAT_STRING_15, QUILL_GENERATE_FORMAT_STRING_14, \
    QUILL_GENERATE_FORMAT_STRING_13, QUILL_GENERATE_FORMAT_STRING_12, QUILL_GENERATE_FORMAT_STRING_11, \
    QUILL_GENERATE_FORMAT_STRING_10, QUILL_GENERATE_FORMAT_STRING_9, QUILL_GENERATE_FORMAT_STRING_8,   \
    QUILL_GENERATE_FORMAT_STRING_7, QUILL_GENERATE_FORMAT_STRING_6, QUILL_GENERATE_FORMAT_STRING_5,    \
    QUILL_GENERATE_FORMAT_STRING_4, QUILL_GENERATE_FORMAT_STRING_3, QUILL_GENERATE_FORMAT_STRING_2,    \
    QUILL_GENERATE_FORMAT_STRING_1, QUILL_GENERATE_FORMAT_STRING_0)(__VA_ARGS__))

/** -- LOGV_ helpers end -- **/

/** -- LOGJ_ helpers begin -- **/

// Helper macros to generate format strings with placeholders
#define QUILL_GENERATE_NAMED_FORMAT_STRING_0(text) text
#define QUILL_GENERATE_NAMED_FORMAT_STRING_1(text, x) text " {" #x "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_2(text, x, y) text " {" #x "}, {" #y "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_3(text, x, y, z) text " {" #x "}, {" #y "}, {" #z "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_4(text, x, y, z, w)                                     \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_5(text, x, y, z, w, v)                                  \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_6(text, x, y, z, w, v, u)                               \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_7(text, x, y, z, w, v, u, t)                            \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_8(text, x, y, z, w, v, u, t, s)                         \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_9(text, x, y, z, w, v, u, t, s, r)                      \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_10(text, x, y, z, w, v, u, t, s, r, q)                  \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_11(text, x, y, z, w, v, u, t, s, r, q, p)               \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_12(text, x, y, z, w, v, u, t, s, r, q, p, o)            \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_13(text, x, y, z, w, v, u, t, s, r, q, p, o, n)         \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_14(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m)      \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_15(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l)   \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_16(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k) \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_17(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j) \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}, {" #j "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_18(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j, i) \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r      \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}, {" #j "}, {" #i "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_19(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l, k, j, i, h) \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r         \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}, {" #j "}, {" #i       \
       "}, {" #h "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_20(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l,   \
                                              k, j, i, h, g)                                       \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r            \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}, {" #j "}, {" #i          \
       "}, {" #h "}, {" #g "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_21(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l,   \
                                              k, j, i, h, g, f)                                    \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r               \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}, {" #j "}, {" #i             \
       "}, {" #h "}, {" #g "}, {" #f "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_22(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l,   \
                                              k, j, i, h, g, f, e)                                             \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}, {" #j "}, {" #i   \
       "}, {" #h "}, {" #g "}, {" #f "}, {" #e "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_23(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l,   \
                                              k, j, i, h, g, f, e, d)                                          \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}, {" #j "}, {" #i   \
       "}, {" #h "}, {" #g "}, {" #f "}, {" #e "}, {" #d "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_24(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l,   \
                                              k, j, i, h, g, f, e, d, c)                                       \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}, {" #j "}, {" #i   \
       "}, {" #h "}, {" #g "}, {" #f "}, {" #e "}, {" #d "}, {" #c "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_25(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l,   \
                                              k, j, i, h, g, f, e, d, c, b)                                    \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}, {" #j "}, {" #i   \
       "}, {" #h "}, {" #g "}, {" #f "}, {" #e "}, {" #d "}, {" #c "}, {" #b "}"
#define QUILL_GENERATE_NAMED_FORMAT_STRING_26(text, x, y, z, w, v, u, t, s, r, q, p, o, n, m, l,   \
                                              k, j, i, h, g, f, e, d, c, b, a)                                 \
  text " {" #x "}, {" #y "}, {" #z "}, {" #w "}, {" #v "}, {" #u "}, {" #t "}, {" #s "}, {" #r     \
       "}, {" #q "}, {" #p "}, {" #o "}, {" #n "}, {" #m "}, {" #l "}, {" #k "}, {" #j "}, {" #i   \
       "}, {" #h "}, {" #g "}, {" #f "}, {" #e "}, {" #d "}, {" #c "}, {" #b "}, {" #a "}"

// Macro to select the appropriate format generator based on the number of arguments
#define QUILL_GET_NAMED_FORMAT_GENERATOR_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12,  \
                                               _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24,    \
                                         _25, _26, _27, NAME, ...)                                 \
  NAME

// Main macro to generate format strings (up to 26 variables plus the text)
#define QUILL_GENERATE_NAMED_FORMAT_STRING(...)                                                                       \
  QUILL_EXPAND(QUILL_GET_NAMED_FORMAT_GENERATOR_MACRO(                                                                \
    __VA_ARGS__, QUILL_GENERATE_NAMED_FORMAT_STRING_26, QUILL_GENERATE_NAMED_FORMAT_STRING_25,                        \
    QUILL_GENERATE_NAMED_FORMAT_STRING_24, QUILL_GENERATE_NAMED_FORMAT_STRING_23,                                     \
    QUILL_GENERATE_NAMED_FORMAT_STRING_22, QUILL_GENERATE_NAMED_FORMAT_STRING_21,                                     \
    QUILL_GENERATE_NAMED_FORMAT_STRING_20, QUILL_GENERATE_NAMED_FORMAT_STRING_19,                                     \
    QUILL_GENERATE_NAMED_FORMAT_STRING_18, QUILL_GENERATE_NAMED_FORMAT_STRING_17,                                     \
    QUILL_GENERATE_NAMED_FORMAT_STRING_16, QUILL_GENERATE_NAMED_FORMAT_STRING_15,                                     \
    QUILL_GENERATE_NAMED_FORMAT_STRING_14, QUILL_GENERATE_NAMED_FORMAT_STRING_13,                                     \
    QUILL_GENERATE_NAMED_FORMAT_STRING_12, QUILL_GENERATE_NAMED_FORMAT_STRING_11,                                     \
    QUILL_GENERATE_NAMED_FORMAT_STRING_10, QUILL_GENERATE_NAMED_FORMAT_STRING_9,                                      \
    QUILL_GENERATE_NAMED_FORMAT_STRING_8, QUILL_GENERATE_NAMED_FORMAT_STRING_7,                                       \
    QUILL_GENERATE_NAMED_FORMAT_STRING_6, QUILL_GENERATE_NAMED_FORMAT_STRING_5,                                       \
    QUILL_GENERATE_NAMED_FORMAT_STRING_4, QUILL_GENERATE_NAMED_FORMAT_STRING_3, QUILL_GENERATE_NAMED_FORMAT_STRING_2, \
    QUILL_GENERATE_NAMED_FORMAT_STRING_1, QUILL_GENERATE_NAMED_FORMAT_STRING_0)(__VA_ARGS__))

/** -- LOGJ_ helpers end -- **/

#define QUILL_DEFINE_MACRO_METADATA(caller_function, fmt, tags, log_level)                         \
  static constexpr quill::MacroMetadata macro_metadata                                             \
  {                                                                                                \
    __FILE__ ":" QUILL_STRINGIFY(__LINE__), caller_function, fmt, tags, log_level,                 \
      quill::MacroMetadata::Event::Log                                                             \
  }

#define QUILL_LOGGER_CALL(likelyhood, logger, log_level, fmt, ...)                                                  \
  do                                                                                                                \
  {                                                                                                                 \
    if (likelyhood(logger->template should_log_statement<log_level>()))                                             \
    {                                                                                                               \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, nullptr, log_level);                                           \
      logger->template log_statement<QUILL_IMMEDIATE_FLUSH>(quill::LogLevel::None, &macro_metadata, ##__VA_ARGS__); \
    }                                                                                                               \
  } while (0)

#define QUILL_LOGGER_CALL_WITH_TAGS(likelyhood, logger, log_level, tags, fmt, ...)                                  \
  do                                                                                                                \
  {                                                                                                                 \
    if (likelyhood(logger->template should_log_statement<log_level>()))                                             \
    {                                                                                                               \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, &tags, log_level);                                             \
      logger->template log_statement<QUILL_IMMEDIATE_FLUSH>(quill::LogLevel::None, &macro_metadata, ##__VA_ARGS__); \
    }                                                                                                               \
  } while (0)

#define QUILL_LOGGER_CALL_LIMIT(min_interval, likelyhood, logger, log_level, fmt, ...)             \
  do                                                                                               \
  {                                                                                                \
    if (likelyhood(logger->template should_log_statement<log_level>()))                            \
    {                                                                                              \
      thread_local std::chrono::time_point<std::chrono::steady_clock> next_log_time;               \
      auto const now = std::chrono::steady_clock::now();                                           \
                                                                                                   \
      if (now < next_log_time)                                                                     \
      {                                                                                            \
        break;                                                                                     \
      }                                                                                            \
                                                                                                   \
      next_log_time = now + min_interval;                                                          \
      QUILL_LOGGER_CALL(likelyhood, logger, log_level, fmt, ##__VA_ARGS__);                        \
    }                                                                                              \
  } while (0)

#define QUILL_BACKTRACE_LOGGER_CALL(logger, fmt, ...)                                                               \
  do                                                                                                                \
  {                                                                                                                 \
    if (QUILL_LIKELY(logger->template should_log_statement<quill::LogLevel::Backtrace>()))                          \
    {                                                                                                               \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, nullptr, quill::LogLevel::Backtrace);                          \
      logger->template log_statement<QUILL_IMMEDIATE_FLUSH>(quill::LogLevel::None, &macro_metadata, ##__VA_ARGS__); \
    }                                                                                                               \
  } while (0)

/**
 * Dynamic runtime log level with a tiny overhead
 * @Note: Prefer using the compile time log level macros
 */
#define QUILL_DYNAMIC_LOGGER_CALL(logger, log_level, fmt, ...)                                          \
  do                                                                                                    \
  {                                                                                                     \
    if (logger->should_log_statement(log_level))                                                        \
    {                                                                                                   \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, nullptr, quill::LogLevel::Dynamic);                \
      logger->template log_statement<QUILL_IMMEDIATE_FLUSH>(log_level, &macro_metadata, ##__VA_ARGS__); \
    }                                                                                                   \
  } while (0)

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3
  #define QUILL_LOG_TRACE_L3(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_WITH_TAGS(logger, tags, fmt, ...)                                     \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, tags, fmt, ##__VA_ARGS__)

  #define QUILL_LOGV_TRACE_L3(logger, message, ...)                                                \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3,                            \
                      QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_TRACE_L3_LIMIT(min_interval, logger, message, ...)                            \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3,        \
                            QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_TRACE_L3_WITH_TAGS(logger, tags, message, ...)                                \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, tags,            \
                                QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_TRACE_L3(logger, message, ...)                                                \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3,                            \
                      QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_TRACE_L3_LIMIT(min_interval, logger, message, ...)                            \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3,        \
                            QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_TRACE_L3_WITH_TAGS(logger, tags, message, ...)                                \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, tags,            \
                                QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L3(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_WITH_TAGS(logger, tags, fmt, ...) (void)0
  #define QUILL_LOGV_TRACE_L3(logger, message, ...) (void)0
  #define QUILL_LOGV_TRACE_L3_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGV_TRACE_L3_WITH_TAGS (void)0
  #define QUILL_LOGJ_TRACE_L3(logger, message, ...) (void)0
  #define QUILL_LOGJ_TRACE_L3_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGJ_TRACE_L3_WITH_TAGS (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L2
  #define QUILL_LOG_TRACE_L2(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_WITH_TAGS(logger, tags, fmt, ...)                                     \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, tags, fmt, ##__VA_ARGS__)

  #define QUILL_LOGV_TRACE_L2(logger, message, ...)                                                \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2,                            \
                      QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_TRACE_L2_LIMIT(min_interval, logger, message, ...)                            \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2,        \
                            QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_TRACE_L2_WITH_TAGS(logger, tags, message, ...)                                \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, tags,            \
                                QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_TRACE_L2(logger, message, ...)                                                \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2,                            \
                      QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_TRACE_L2_LIMIT(min_interval, logger, message, ...)                            \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2,        \
                            QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_TRACE_L2_WITH_TAGS(logger, tags, message, ...)                                \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, tags,            \
                                QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L2(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_WITH_TAGS(logger, tags, fmt, ...) (void)0
  #define QUILL_LOGV_TRACE_L2(logger, message, ...) (void)0
  #define QUILL_LOGV_TRACE_L2_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGV_TRACE_L2_WITH_TAGS(logger, tags, message, ...) (void)0
  #define QUILL_LOGJ_TRACE_L2(logger, message, ...) (void)0
  #define QUILL_LOGJ_TRACE_L2_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGJ_TRACE_L2_WITH_TAGS(logger, tags, message, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L1
  #define QUILL_LOG_TRACE_L1(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_WITH_TAGS(logger, tags, fmt, ...)                                     \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, tags, fmt, ##__VA_ARGS__)

  #define QUILL_LOGV_TRACE_L1(logger, message, ...)                                                \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1,                            \
                      QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_TRACE_L1_LIMIT(min_interval, logger, message, ...)                            \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1,        \
                            QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_TRACE_L1_WITH_TAGS(logger, tags, message, ...)                                \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, tags,            \
                                QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_TRACE_L1(logger, message, ...)                                                \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1,                            \
                      QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_TRACE_L1_LIMIT(min_interval, logger, message, ...)                            \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1,        \
                            QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_TRACE_L1_WITH_TAGS(logger, tags, message, ...)                                \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, tags,            \
                                QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L1(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_WITH_TAGS(logger, tags, fmt, ...) (void)0
  #define QUILL_LOGV_TRACE_L1(logger, message, ...) (void)0
  #define QUILL_LOGV_TRACE_L1_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGV_TRACE_L1_WITH_TAGS(logger, tags, message, ...) (void)0
  #define QUILL_LOGJ_TRACE_L1(logger, message, ...) (void)0
  #define QUILL_LOGJ_TRACE_L1_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGJ_TRACE_L1_WITH_TAGS(logger, tags, message, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG
  #define QUILL_LOG_DEBUG(logger, fmt, ...)                                                        \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...)                                    \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_WITH_TAGS(logger, tags, fmt, ...)                                        \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, tags, fmt, ##__VA_ARGS__)

  #define QUILL_LOGV_DEBUG(logger, message, ...)                                                   \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::Debug,                              \
                      QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_DEBUG_LIMIT(min_interval, logger, message, ...)                               \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::Debug,          \
                            QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_DEBUG_WITH_TAGS(logger, tags, message, ...)                                   \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, tags,              \
                                QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_DEBUG(logger, message, ...)                                                   \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::Debug,                              \
                      QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_DEBUG_LIMIT(min_interval, logger, message, ...)                               \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::Debug,          \
                            QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_DEBUG_WITH_TAGS(logger, tags, message, ...)                                   \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, tags,              \
                                QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)
#else
  #define QUILL_LOG_DEBUG(logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_WITH_TAGS(logger, tags, fmt, ...) (void)0
  #define QUILL_LOGV_DEBUG(logger, message, ...) (void)0
  #define QUILL_LOGV_DEBUG_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGV_DEBUG_WITH_TAGS(logger, tags, message, ...) (void)0
  #define QUILL_LOGJ_DEBUG(logger, message, ...) (void)0
  #define QUILL_LOGJ_DEBUG_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGJ_DEBUG_WITH_TAGS(logger, tags, message, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO
  #define QUILL_LOG_INFO(logger, fmt, ...)                                                         \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_LIMIT(min_interval, logger, fmt, ...)                                     \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_WITH_TAGS(logger, tags, fmt, ...)                                         \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Info, tags, fmt, ##__VA_ARGS__)

  #define QUILL_LOGV_INFO(logger, message, ...)                                                    \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Info,                                 \
                      QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_INFO_LIMIT(min_interval, logger, message, ...)                                \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Info,             \
                            QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_INFO_WITH_TAGS(logger, tags, message, ...)                                    \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Info, tags,                 \
                                QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_INFO(logger, message, ...)                                                    \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Info,                                 \
                      QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_INFO_LIMIT(min_interval, logger, message, ...)                                \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Info,             \
                            QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_INFO_WITH_TAGS(logger, tags, message, ...)                                    \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Info, tags,                 \
                                QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)
#else
  #define QUILL_LOG_INFO(logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_WITH_TAGS(logger, tags, fmt, ...) (void)0
  #define QUILL_LOGV_INFO(logger, message, ...) (void)0
  #define QUILL_LOGV_INFO_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGV_INFO_WITH_TAGS(logger, tags, message, ...) (void)0
  #define QUILL_LOGJ_INFO(logger, message, ...) (void)0
  #define QUILL_LOGJ_INFO_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGJ_INFO_WITH_TAGS(logger, tags, message, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING
  #define QUILL_LOG_WARNING(logger, fmt, ...)                                                      \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_LIMIT(min_interval, logger, fmt, ...)                                  \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_WITH_TAGS(logger, tags, fmt, ...)                                      \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Warning, tags, fmt, ##__VA_ARGS__)

  #define QUILL_LOGV_WARNING(logger, message, ...)                                                 \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Warning,                              \
                      QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_WARNING_LIMIT(min_interval, logger, message, ...)                             \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Warning,          \
                            QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_WARNING_WITH_TAGS(logger, tags, message, ...)                                 \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Warning, tags,              \
                                QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_WARNING(logger, message, ...)                                                 \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Warning,                              \
                      QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_WARNING_LIMIT(min_interval, logger, message, ...)                             \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Warning,          \
                            QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_WARNING_WITH_TAGS(logger, tags, message, ...)                                 \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Warning, tags,              \
                                QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)
#else
  #define QUILL_LOG_WARNING(logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_WITH_TAGS(logger, tags, fmt, ...) (void)0
  #define QUILL_LOGV_WARNING(logger, message, ...) (void)0
  #define QUILL_LOGV_WARNING_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGV_WARNING_WITH_TAGS(logger, tags, message, ...) (void)0
  #define QUILL_LOGJ_WARNING(logger, message, ...) (void)0
  #define QUILL_LOGJ_WARNING_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGJ_WARNING_WITH_TAGS(logger, tags, message, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR
  #define QUILL_LOG_ERROR(logger, fmt, ...)                                                        \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_LIMIT(min_interval, logger, fmt, ...)                                    \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_WITH_TAGS(logger, tags, fmt, ...)                                        \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Error, tags, fmt, ##__VA_ARGS__)

  #define QUILL_LOGV_ERROR(logger, message, ...)                                                   \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Error,                                \
                      QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_ERROR_LIMIT(min_interval, logger, message, ...)                               \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Error,            \
                            QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_ERROR_WITH_TAGS(logger, tags, message, ...)                                   \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Error, tags,                \
                                QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_ERROR(logger, message, ...)                                                   \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Error,                                \
                      QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_ERROR_LIMIT(min_interval, logger, message, ...)                               \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Error,            \
                            QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_ERROR_WITH_TAGS(logger, tags, message, ...)                                   \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Error, tags,                \
                                QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)
#else
  #define QUILL_LOG_ERROR(logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_WITH_TAGS(logger, tags, fmt, ...) (void)0
  #define QUILL_LOGV_ERROR(logger, message, ...) (void)0
  #define QUILL_LOGV_ERROR_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGV_ERROR_WITH_TAGS(logger, tags, message, ...) (void)0
  #define QUILL_LOGJ_ERROR(logger, message, ...) (void)0
  #define QUILL_LOGJ_ERROR_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGJ_ERROR_WITH_TAGS(logger, tags, message, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL
  #define QUILL_LOG_CRITICAL(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_WITH_TAGS(logger, tags, fmt, ...)                                     \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Critical, tags, fmt, ##__VA_ARGS__)

  #define QUILL_LOGV_CRITICAL(logger, message, ...)                                                \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Critical,                             \
                      QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_CRITICAL_LIMIT(min_interval, logger, message, ...)                            \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Critical,         \
                            QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGV_CRITICAL_WITH_TAGS(logger, tags, message, ...)                                \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Critical, tags,             \
                                QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_CRITICAL(logger, message, ...)                                                \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Critical,                             \
                      QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_CRITICAL_LIMIT(min_interval, logger, message, ...)                            \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Critical,         \
                            QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

  #define QUILL_LOGJ_CRITICAL_WITH_TAGS(logger, tags, message, ...)                                \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Critical, tags,             \
                                QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)
#else
  #define QUILL_LOG_CRITICAL(logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_WITH_TAGS(logger, tags, fmt, ...) (void)0
  #define QUILL_LOGV_CRITICAL(logger, message, ...) (void)0
  #define QUILL_LOGV_CRITICAL_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGV_CRITICAL_WITH_TAGS(logger, tags, message, ...) (void)0
  #define QUILL_LOGJ_CRITICAL(logger, message, ...) (void)0
  #define QUILL_LOGJ_CRITICAL_LIMIT(min_interval, logger, message, ...) (void)0
  #define QUILL_LOGJ_CRITICAL_WITH_TAGS(logger, tags, message, ...) (void)0
#endif

#define QUILL_LOG_BACKTRACE(logger, fmt, ...)                                                      \
  QUILL_BACKTRACE_LOGGER_CALL(logger, fmt, ##__VA_ARGS__)

#define QUILL_LOGV_BACKTRACE(logger, message, ...)                                                 \
  QUILL_BACKTRACE_LOGGER_CALL(logger, QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

#define QUILL_LOGJ_BACKTRACE(logger, message, ...)                                                 \
  QUILL_BACKTRACE_LOGGER_CALL(logger, QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

#define QUILL_LOG_DYNAMIC(logger, log_level, fmt, ...)                                             \
  QUILL_DYNAMIC_LOGGER_CALL(logger, log_level, fmt, ##__VA_ARGS__)

#define QUILL_LOGV_DYNAMIC(logger, log_level, message, ...)                                        \
  QUILL_DYNAMIC_LOGGER_CALL(logger, log_level, QUILL_GENERATE_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

#define QUILL_LOGJ_DYNAMIC(logger, log_level, message, ...)                                        \
  QUILL_DYNAMIC_LOGGER_CALL(                                                                       \
    logger, log_level, QUILL_GENERATE_NAMED_FORMAT_STRING(message, ##__VA_ARGS__), ##__VA_ARGS__)

#if !defined(QUILL_DISABLE_NON_PREFIXED_MACROS)
  #define LOG_TRACE_L3(logger, fmt, ...) QUILL_LOG_TRACE_L3(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2(logger, fmt, ...) QUILL_LOG_TRACE_L2(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1(logger, fmt, ...) QUILL_LOG_TRACE_L1(logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG(logger, fmt, ...) QUILL_LOG_DEBUG(logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO(logger, fmt, ...) QUILL_LOG_INFO(logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING(logger, fmt, ...) QUILL_LOG_WARNING(logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR(logger, fmt, ...) QUILL_LOG_ERROR(logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL(logger, fmt, ...) QUILL_LOG_CRITICAL(logger, fmt, ##__VA_ARGS__)
  #define LOG_BACKTRACE(logger, fmt, ...) QUILL_LOG_BACKTRACE(logger, fmt, ##__VA_ARGS__)
  #define LOG_DYNAMIC(logger, log_level, fmt, ...)                                                 \
    QUILL_LOG_DYNAMIC(logger, log_level, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...)                                       \
    QUILL_LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...)                                       \
    QUILL_LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...)                                       \
    QUILL_LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...)                                          \
    QUILL_LOG_DEBUG_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO_LIMIT(min_interval, logger, fmt, ...)                                           \
    QUILL_LOG_INFO_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_LIMIT(min_interval, logger, fmt, ...)                                        \
    QUILL_LOG_WARNING_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_LIMIT(min_interval, logger, fmt, ...)                                          \
    QUILL_LOG_ERROR_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...)                                       \
    QUILL_LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_WITH_TAGS(logger, tags, fmt, ...)                                           \
    QUILL_LOG_TRACE_L3_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_WITH_TAGS(logger, tags, fmt, ...)                                           \
    QUILL_LOG_TRACE_L2_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_WITH_TAGS(logger, tags, fmt, ...)                                           \
    QUILL_LOG_TRACE_L1_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_WITH_TAGS(logger, tags, fmt, ...)                                              \
    QUILL_LOG_DEBUG_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_INFO_WITH_TAGS(logger, tags, fmt, ...)                                               \
    QUILL_LOG_INFO_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_WITH_TAGS(logger, tags, fmt, ...)                                            \
    QUILL_LOG_WARNING_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_WITH_TAGS(logger, tags, fmt, ...)                                              \
    QUILL_LOG_ERROR_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_WITH_TAGS(logger, tags, fmt, ...)                                           \
    QUILL_LOG_CRITICAL_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)

  #define LOGV_TRACE_L3(logger, message, ...) QUILL_LOGV_TRACE_L3(logger, message, ##__VA_ARGS__)
  #define LOGV_TRACE_L2(logger, message, ...) QUILL_LOGV_TRACE_L2(logger, message, ##__VA_ARGS__)
  #define LOGV_TRACE_L1(logger, message, ...) QUILL_LOGV_TRACE_L1(logger, message, ##__VA_ARGS__)
  #define LOGV_DEBUG(logger, message, ...) QUILL_LOGV_DEBUG(logger, message, ##__VA_ARGS__)
  #define LOGV_INFO(logger, message, ...) QUILL_LOGV_INFO(logger, message, ##__VA_ARGS__)
  #define LOGV_WARNING(logger, message, ...) QUILL_LOGV_WARNING(logger, message, ##__VA_ARGS__)
  #define LOGV_ERROR(logger, message, ...) QUILL_LOGV_ERROR(logger, message, ##__VA_ARGS__)
  #define LOGV_CRITICAL(logger, message, ...) QUILL_LOGV_CRITICAL(logger, message, ##__VA_ARGS__)
  #define LOGV_BACKTRACE(logger, message, ...) QUILL_LOGV_BACKTRACE(logger, message, ##__VA_ARGS__)
  #define LOGV_DYNAMIC(logger, log_level, message, ...)                                            \
    QUILL_LOGV_DYNAMIC(logger, log_level, fmt, ##__VA_ARGS__)

  #define LOGV_TRACE_L3_LIMIT(min_interval, logger, message, ...)                                  \
    QUILL_LOGV_TRACE_L3_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGV_TRACE_L2_LIMIT(min_interval, logger, message, ...)                                  \
    QUILL_LOGV_TRACE_L2_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGV_TRACE_L1_LIMIT(min_interval, logger, message, ...)                                  \
    QUILL_LOGV_TRACE_L1_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGV_DEBUG_LIMIT(min_interval, logger, message, ...)                                     \
    QUILL_LOGV_DEBUG_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGV_INFO_LIMIT(min_interval, logger, message, ...)                                      \
    QUILL_LOGV_INFO_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGV_WARNING_LIMIT(min_interval, logger, message, ...)                                   \
    QUILL_LOGV_WARNING_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGV_ERROR_LIMIT(min_interval, logger, message, ...)                                     \
    QUILL_LOGV_ERROR_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGV_CRITICAL_LIMIT(min_interval, logger, message, ...)                                  \
    QUILL_LOGV_CRITICAL_LIMIT(min_interval, logger, message, ##__VA_ARGS__)

  #define LOGV_TRACE_L3_WITH_TAGS(logger, tags, message, ...)                                      \
    QUILL_LOGV_TRACE_L3_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGV_TRACE_L2_WITH_TAGS(logger, tags, message, ...)                                      \
    QUILL_LOGV_TRACE_L2_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGV_TRACE_L1_WITH_TAGS(logger, tags, message, ...)                                      \
    QUILL_LOGV_TRACE_L1_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGV_DEBUG_WITH_TAGS(logger, tags, message, ...)                                         \
    QUILL_LOGV_DEBUG_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGV_INFO_WITH_TAGS(logger, tags, message, ...)                                          \
    QUILL_LOGV_INFO_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGV_WARNING_WITH_TAGS(logger, tags, message, ...)                                       \
    QUILL_LOGV_WARNING_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGV_ERROR_WITH_TAGS(logger, tags, message, ...)                                         \
    QUILL_LOGV_ERROR_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGV_CRITICAL_WITH_TAGS(logger, tags, message, ...)                                      \
    QUILL_LOGV_CRITICAL_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)

  #define LOGJ_TRACE_L3(logger, message, ...) QUILL_LOGJ_TRACE_L3(logger, message, ##__VA_ARGS__)
  #define LOGJ_TRACE_L2(logger, message, ...) QUILL_LOGJ_TRACE_L2(logger, message, ##__VA_ARGS__)
  #define LOGJ_TRACE_L1(logger, message, ...) QUILL_LOGJ_TRACE_L1(logger, message, ##__VA_ARGS__)
  #define LOGJ_DEBUG(logger, message, ...) QUILL_LOGJ_DEBUG(logger, message, ##__VA_ARGS__)
  #define LOGJ_INFO(logger, message, ...) QUILL_LOGJ_INFO(logger, message, ##__VA_ARGS__)
  #define LOGJ_WARNING(logger, message, ...) QUILL_LOGJ_WARNING(logger, message, ##__VA_ARGS__)
  #define LOGJ_ERROR(logger, message, ...) QUILL_LOGJ_ERROR(logger, message, ##__VA_ARGS__)
  #define LOGJ_CRITICAL(logger, message, ...) QUILL_LOGJ_CRITICAL(logger, message, ##__VA_ARGS__)
  #define LOGJ_BACKTRACE(logger, message, ...) QUILL_LOGJ_BACKTRACE(logger, message, ##__VA_ARGS__)
  #define LOGJ_DYNAMIC(logger, log_level, message, ...)                                            \
    QUILL_LOGJ_DYNAMIC(logger, log_level, fmt, ##__VA_ARGS__)

  #define LOGJ_TRACE_L3_LIMIT(min_interval, logger, message, ...)                                  \
    QUILL_LOGJ_TRACE_L3_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGJ_TRACE_L2_LIMIT(min_interval, logger, message, ...)                                  \
    QUILL_LOGJ_TRACE_L2_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGJ_TRACE_L1_LIMIT(min_interval, logger, message, ...)                                  \
    QUILL_LOGJ_TRACE_L1_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGJ_DEBUG_LIMIT(min_interval, logger, message, ...)                                     \
    QUILL_LOGJ_DEBUG_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGJ_INFO_LIMIT(min_interval, logger, message, ...)                                      \
    QUILL_LOGJ_INFO_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGJ_WARNING_LIMIT(min_interval, logger, message, ...)                                   \
    QUILL_LOGJ_WARNING_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGJ_ERROR_LIMIT(min_interval, logger, message, ...)                                     \
    QUILL_LOGJ_ERROR_LIMIT(min_interval, logger, message, ##__VA_ARGS__)
  #define LOGJ_CRITICAL_LIMIT(min_interval, logger, message, ...)                                  \
    QUILL_LOGJ_CRITICAL_LIMIT(min_interval, logger, message, ##__VA_ARGS__)

  #define LOGJ_TRACE_L3_WITH_TAGS(logger, tags, message, ...)                                      \
    QUILL_LOGJ_TRACE_L3_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGJ_TRACE_L2_WITH_TAGS(logger, tags, message, ...)                                      \
    QUILL_LOGJ_TRACE_L2_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGJ_TRACE_L1_WITH_TAGS(logger, tags, message, ...)                                      \
    QUILL_LOGJ_TRACE_L1_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGJ_DEBUG_WITH_TAGS(logger, tags, message, ...)                                         \
    QUILL_LOGJ_DEBUG_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGJ_INFO_WITH_TAGS(logger, tags, message, ...)                                          \
    QUILL_LOGJ_INFO_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGJ_WARNING_WITH_TAGS(logger, tags, message, ...)                                       \
    QUILL_LOGJ_WARNING_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGJ_ERROR_WITH_TAGS(logger, tags, message, ...)                                         \
    QUILL_LOGJ_ERROR_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
  #define LOGJ_CRITICAL_WITH_TAGS(logger, tags, message, ...)                                      \
    QUILL_LOGJ_CRITICAL_WITH_TAGS(logger, tags, message, ##__VA_ARGS__)
#endif