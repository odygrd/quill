#pragma once

#include "quill/core/Attributes.h"

#if defined(FOO_DLL_EXPORT)
  #define FOO_EXPORT __declspec(dllexport) // Exporting symbols when building the library
#else
  #define FOO_EXPORT __declspec(dllimport) // Importing symbols when using the library
#endif

FOO_EXPORT void init_foo();
FOO_EXPORT void test_foo();