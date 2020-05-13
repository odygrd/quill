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
  #include <fmt/ranges.h>
#else
  #include "quill/bundled/fmt/chrono.h"
  #include "quill/bundled/fmt/format.h"
  #include "quill/bundled/fmt/ostream.h"
  #include "quill/bundled/fmt/ranges.h"
#endif