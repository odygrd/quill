module;

#ifndef QUILL_HAS_INCLUDE
  #ifdef __has_include
    #define QUILL_HAS_INCLUDE(x) __has_include(x)
  #else
    #define QUILL_HAS_INCLUDE(x) 0
  #endif
#endif

// Put implementation-provided declarations into the global module fragment
// to prevent them from being attached to the Quill module.
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <codecvt>
#include <complex>
#include <condition_variable>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <ostream>
#include <set>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#if QUILL_HAS_INCLUDE(<filesystem>)
  #include <filesystem>
#elif QUILL_HAS_INCLUDE(<experimental/filesystem>)
  #include <experimental/filesystem>
#endif
#include <climits>
#include <version>

#if QUILL_HAS_INCLUDE(<cxxabi.h>)
  #include <cxxabi.h>
#endif
#if defined(_MSC_VER) || defined(__MINGW32__)
  #include <intrin.h>
#endif
#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif
  #if !defined(NOMINMAX)
    #define NOMINMAX
  #endif
  #include <io.h>
  #include <windows.h>
#endif
#if !defined(__INTEL_COMPILER)
  #if QUILL_HAS_INCLUDE(<x86gprintrin.h>)
    #include <x86gprintrin.h>
  #elif QUILL_HAS_INCLUDE(<x86intrin.h>)
    #include <x86intrin.h>
  #endif
#endif
#if defined __APPLE__ || defined(__FreeBSD__)
  #include <xlocale.h>
#endif
#if QUILL_HAS_INCLUDE(<winapifamily.h>)
  #include <winapifamily.h>
#endif

export module quill;

#define QUILL_MODULE
#define FMTQUILL_MODULE
#define FMTQUILL_EXPORT export
#define FMTQUILL_BEGIN_EXPORT export {
#define FMTQUILL_END_EXPORT }

#include "quill/bundled/fmt/ostream.h"
#include "quill/bundled/fmt/ranges.h"

#include "quill/Backend.h"
#include "quill/BackendTscClock.h"
#include "quill/BinaryDataDeferredFormatCodec.h"
#include "quill/CsvWriter.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/DirectFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/Logger.h"
#include "quill/SimpleSetup.h"
#include "quill/StringRef.h"
#include "quill/StopWatch.h"
#include "quill/UserClockSource.h"
#include "quill/Utility.h"
#include "quill/filters/Filter.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/JsonSink.h"
#include "quill/sinks/NullSink.h"
#include "quill/sinks/RotatingFileSink.h"
#include "quill/sinks/RotatingJsonFileSink.h"
#include "quill/sinks/RotatingSink.h"
#include "quill/sinks/Sink.h"
#include "quill/sinks/StreamSink.h"
#include "quill/std/Array.h"
#include "quill/std/Bitset.h"
#include "quill/std/Chrono.h"
#include "quill/std/Complex.h"
#include "quill/std/Deque.h"
#include "quill/std/FilesystemPath.h"
#include "quill/std/ForwardList.h"
#include "quill/std/List.h"
#include "quill/std/Map.h"
#include "quill/std/Optional.h"
#include "quill/std/Pair.h"
#include "quill/std/Set.h"
#include "quill/std/SystemError.h"
#include "quill/std/Tuple.h"
#include "quill/std/UnorderedMap.h"
#include "quill/std/UnorderedSet.h"
#include "quill/std/Variant.h"
#include "quill/std/Vector.h"
#include "quill/std/WideString.h"
