#pragma once

#ifdef QUILL_SHARED_TEST_EXPORTS
  #define QUILL_SHARED_TEST_API __declspec(dllexport)
#else
  #define QUILL_SHARED_TEST_API __declspec(dllimport)
#endif

QUILL_SHARED_TEST_API void init();

QUILL_SHARED_TEST_API void log();

QUILL_SHARED_TEST_API void stop();