#pragma once

#ifdef NDEBUG
#else
  #define QUILL_IMMEDIATE_FLUSH true
  #define QUILL_COMPILE_ACTIVE_LOG_LEVEL QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3
#endif

#include "quill/core/Attributes.h"

QUILL_EXPORT void setup_quill();