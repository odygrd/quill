/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

/**
 * Include a bundled header-only copy of lib fmt or an external one.
 * By default Quill includes it's bundled copy.
 */
#if defined(QUILL_FMT_EXTERNAL)
  #include <fmt/chrono.h>
  #include <fmt/format.h>
  #include <fmt/ostream.h>
  #include <fmt/printf.h>
  #include <fmt/ranges.h>

  #define QUILL_FMT_VERSION FMT_VERSION
  #define QUILL_FMT_STRING FMT_STRING

namespace fmtquill = fmt;
#else
  #include "quill/bundled/fmt/chrono.h"
  #include "quill/bundled/fmt/format.h"
  #include "quill/bundled/fmt/ostream.h"
  #include "quill/bundled/fmt/printf.h"
  #include "quill/bundled/fmt/ranges.h"

  #define QUILL_FMT_VERSION FMTQUILL_VERSION
  #define QUILL_FMT_STRING FMTQUILL_STRING
#endif

/**
 * Also include additional files due to recent fmt versions
 */
#if QUILL_FMT_VERSION > 70103
  #if defined(QUILL_FMT_EXTERNAL)
    #include <fmt/args.h>
    #include <fmt/xchar.h>
  #else
    #include "quill/bundled/fmt/args.h"
    #include "quill/bundled/fmt/xchar.h"
  #endif
#endif

/**
 * Also include additional files due to recent fmt versions
 */
#if QUILL_FMT_VERSION >= 90000
  #if defined(QUILL_FMT_EXTERNAL)
    #include <fmt/std.h>
  #else
    #include "quill/bundled/fmt/std.h"
  #endif
#endif