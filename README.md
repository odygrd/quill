<div align="center">
  <a href="https://github.com/odygrd/quill">
    <img width="125" src="https://i.postimg.cc/DZrH8HkX/quill-circle-photos-v2-x2-colored-toned.png" alt="Quill logo">
  </a>
  <h1>Quill</h1>

  <div>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Alinux">
      <img src="https://img.shields.io/github/workflow/status/odygrd/quill/linux?label=linux&logo=linux&style=flat-square" alt="linux-ci" />
    </a>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Amacos">
      <img src="https://img.shields.io/github/workflow/status/odygrd/quill/macos?label=macos&logo=apple&logoColor=white&style=flat-square" alt="macos-ci" />
    </a>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Awindows">
      <img src="https://img.shields.io/github/workflow/status/odygrd/quill/windows?label=windows&logo=windows&logoColor=blue&style=flat-square" alt="windows-ci" />
    </a>
    <a href="https://cloud.drone.io/odygrd/quill">
      <img src="https://img.shields.io/drone/build/odygrd/quill/master?label=ARM&logo=drone&style=flat-square" alt="drone-ci" />
    </a>
  </div>
  
  <div>
    <a href="https://codecov.io/gh/odygrd/quill">
      <img src="https://img.shields.io/codecov/c/gh/odygrd/quill/master.svg?logo=codecov&style=flat-square" alt="Codecov" />
    </a>
    <a href="https://www.codacy.com/manual/odygrd/quill?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=odygrd/quill&amp;utm_campaign=Badge_Grade">
      <img src="https://img.shields.io/codacy/grade/cd387bc34658475d98bff84db3ad5287?logo=codacy&style=flat-square" alt="Codacy" />
    </a>
    <a href="https://www.codefactor.io/repository/github/odygrd/quill">
      <img src="https://img.shields.io/codefactor/grade/github/odygrd/quill?logo=codefactor&style=flat-square" alt="CodeFactor" />
     </a>
  </div>

  <div>
    <a href="https://opensource.org/licenses/MIT">
      <img src="https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square" alt="license" />
    </a>
    <a href="https://en.wikipedia.org/wiki/C%2B%2B14">
      <img src="https://img.shields.io/badge/language-C%2B%2B14-red.svg?style=flat-square" alt="language" />
    </a>
  </div>

  <p><b>Asynchronous Low Latency C++ Logging Library</b></p>

</div>


<br>

-  [Introduction](#introduction)
-  [Features](#features)
-  [Performance](#performance)
-  [Supported Platforms And Compilers](#supported-platforms-and-compilers)
-  [Basic Usage](#basic-usage)
-  [CMake Integration](#cmake-integration)
-  [Documentation](#documentation)
-  [License](#license)


|  homebrew             |  vcpkg                 |  conan            |
|:---------------------:|:----------------------:|:-----------------:|
|  `brew install quill` |  `vcpkg install quill` | `quill/[>=1.2.3]` |


## Introduction
Quill is a cross-platform low latency logging library based on C++14.

The main goals of the library are:

-  **Simplicity** A small example code snippet should be enough to get started and use most of features.
-  **Performance** Ultra low latency. No formatting on the hot-path, asynchronous only mode. No hot-path allocations for fundamental types, enums and strings (including `std::string` and `std::string_view`). Any other custom or user defined type gets copy constructed with the formatting done on a backend worker thread.
-  **Convenience** Ease application monitoring/debugging. Latency is equal to latencies of binary loggers, but the produced log is in human readable form.

## Features
 -  Log anything - Blazing fast. See [Benchmarks](https://github.com/odygrd/quill#performance).
 -  Format outside of the hot-path in a backend logging thread. For `non-built-in` types `ostream::operator<<()` is called on a copy of the object by the backend logging thread. Unsafe to copy `non-trivial user defined` are detected in compile time. Those types can be tagged as `safe-to-copy` to avoid formatting them on the hot path. See [User Defined Types](https://github.com/odygrd/quill/wiki/8.-User-Defined-Types).
 -  Custom formatters. Logs can be formatted based on a user specified pattern. See [Formatters](https://github.com/odygrd/quill/wiki/4.-Formatters).
 -  Support for log stack traces. Store log messages in a ring buffer and display later on a higher severity log statement or on demand. See [Backtrace Logging](https://github.com/odygrd/quill/wiki/6.-Backtrace-Logging).
 -  Various logging targets. See [Handlers](https://github.com/odygrd/quill/wiki/2.-Handlers).
    -  Console logging with colours support.
    -  File Logging
    -  Rotating log files
    -  Time rotating log files
    -  Custom Handlers
 -  Filters for filtering log messages. See [Filters](https://github.com/odygrd/quill/wiki/3.-Filters).
 -  `guaranteed non-blocking` or `non-guaranteed` logging. In `non-guaranteed` mode there is no heap allocation of a new queue but log messages can be dropped. See [FAQ](https://github.com/odygrd/quill/wiki/7.-FAQ#guaranteed-logging-mode).
 -  Support for wide character logging and wide character filenames (Windows only).
 -  Log statements in timestamp order even when produced by different threads. This makes debugging easier in multi-threaded applications.
 -  Log levels can be completely stripped out at compile time reducing `if` branches.
 -  Clean warning-free codebase even on high warning levels.
 -  Crash safe behaviour with a build-in signal handler.
 -  Type safe python style API with compile type checks and built-in support for logging STL types/containers by using the excellent [{fmt}](https://github.com/fmtlib/fmt) library.

## Performance

### Log Numbers
The following message is logged 100'000 times per thread  ```LOG_INFO(logger, "Logging int: {}, int: {}, double: {}", i, j, d)```.

The results in the tables below are in nanoseconds (ns).

#### 1 Thread

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[Quill, Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)       |  20  |  22  |  25  |  27  |  34  |  67  |  118  |
|[Quill, Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)     |  20  |  24  |  28  |  30  |  36  |  69  |  134  |
|[Quill, Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)    |  23  |  31  |  35  |  37  |  52  |  75  |  134  |
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                     |  19  |  22  |  23  |  25  |  28  |  64  |  128  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)                             |  46  |  47  |  48  |  49  |  82  |  129  |  353  |
|[Reckless](https://github.com/mattiasflodin/reckless)                             |  69  |  70  |  74  |  80  |  106  |  133  |  5908477  |
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                          |  112  |  122  |  134  |  147  |  207  |  337  |  597293  |                
|[spdlog](https://github.com/gabime/spdlog)                                        |  288  |  312  |  328  |  345  |  670  |  914  |  4794  |      
|[g3log](https://github.com/KjellKod/g3log)                                        |  2434  |  2886  |  2992  |  3055  |  3178  |  3338  |  5579  |               

#### 4 Threads

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[Quill, Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)      |  19  |  22  |  26  |  30  |  46  |  67  |  343  |
|[Quill, Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)    |  20  |  22  |  26  |  30  |  46  |  66  |  242  |
|[Quill, Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)   |  21  |  28  |  33  |  38  |  64  |  79  |  4207  |
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                    |  18  |  21  |  23  |  24  |  42  |  58  |  223  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)                            |  45  |  46  |  48  |  49  |  75  |  125  |  271  |
|[Reckless](https://github.com/mattiasflodin/reckless)                            |  93  |  117  |  125  |  129  |  163  |  358  |  30186464  |                         
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                         |  115  |  156  |  191  |  214  |  299  |  441  |  1142546  |
|[spdlog](https://github.com/gabime/spdlog)                                       |  335  |  537  |  714  |  796  |  1081  |  1534  |  27378  |
|[g3log](https://github.com/KjellKod/g3log)                                       |  2834  |  2929  |  3032  |  3132  |  5081  |  5994  |  26563  |

### Log Numbers and Large Strings
The following message is logged 100'000 times per thread  ```LOG_INFO(logger, "Logging int: {}, int: {}, string: {}", i, j, large_string)```.
The large string is over 35 characters to avoid short string optimisation of `std::string`

#### 1 Thread

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[Quill, Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)       |  26  |  31  |  37  |  40  |  48  |  73  |  136  |
|[Quill, Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)     |  28  |  33  |  39  |  42  |  59  |  79  |  138  |
|[Quill, Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)    |  195  |  211  |  224  |  232  |  251  |  283  |  325  |
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                     |  29  |  34  |  39  |  54  |  57  |  77  |  178  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)                             |  51  |  53  |  56  |  58  |  75  |  131  |  343  |
|[Reckless](https://github.com/mattiasflodin/reckless)                             |  94  |  96  |  102  |  117  |  155  |  176  |  6924627  |
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                          |  111  |  119  |  127  |  131  |  172  |  319  |  9553  |
|[spdlog](https://github.com/gabime/spdlog)                                        |  231  |  253  |  269  |  282  |  587  |  806  |  1055  |
|[g3log](https://github.com/KjellKod/g3log)                                        |  1122  |  1142  |  1368  |  2293  |  2516  |  2698  |  4051  |

#### 4 Threads

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[Quill, Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)      |  27  |  32  |  38  |  45  |  59  |  82  |  440  |
|[Quill, Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)    |  26  |  32  |  38  |  46  |  60  |  106  |  3866  |
|[Quill, Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)   |  203  |  226  |  243  |  252  |  269  |  300  |  4278  |
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                    |  29  |  34  |  47  |  55  |  59  |  69  |  289  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)                            |  51  |  54  |  58  |  62  |  96  |  133  |  375  |
|[Reckless](https://github.com/mattiasflodin/reckless)                            |  115  |  134  |  157  |  172  |  216  |  377  |  30721857  |
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                         |  111  |  121  |  137  |  175  |  231  |  811  |  32100  |
|[spdlog](https://github.com/gabime/spdlog)                                       |  266  |  309  |  593  |  672  |  911  |  1329  |  13600  |
|[g3log](https://github.com/KjellKod/g3log)                                       |  1816  |  2207  |  2389  |  2497  |  3457  |  4211  |  8365  |

The benchmarks are done on `Linux (Ubuntu/RHEL) - Intel(R) Xeon(R) CPU E5-2690 0 @ 2.90GHz` with GCC 8.1.

Each thread is pinned on a different cpu. Unfortunately the cores are not isolated. 
Running the backend logger thread in the same CPU as the caller hot-path threads, slows down the log message processing on the backend logging thread and will cause the SPSC queue to fill faster and re-allocate.

Continuously Logging messages in a loop makes the consumer (backend logging thread) unable to follow up and the queue will have to re-allocate or block for most logging libraries expect very high throughput binary loggers like PlatformLab Nanolog.
Therefore, a different approach was followed that suits more to a real time application:
1. 20 messages are logged in a loop.
2. calculate/store the average latency for those messages.
3. wait between 1-2 ms.
4. repeat for n iterations.

I run each logger benchmark four times and the above latencies are the second best result.

The benchmark code and results can be found [here](https://github.com/odygrd/logger_benchmarks).

## Supported Platforms And Compilers
Quill requires a C++14 compiler. Minimum required versions of supported compilers are shown in the below table.

| Compiler  | Notes            |
|-----------|------------------|
| GCC       | version >= 5.0   |
| Clang     | version >= 5.0   |
| MSVC++    | version >= 14.3  |

| Platform  | Notes                                          |
|-----------|------------------------------------------------|
| Linux     | Ubuntu, RHEL, Centos, Fedora                   |
| Windows   | Windows 10 - version 1607, Windows Server 2016 |
| macOS     | Tested with Xcode 9.4                          |

## Basic usage

```c++
#include "quill/Quill.h"

int main()
{
  quill::enable_console_colours();
  quill::start();

  quill::Logger* logger = quill::get_logger();
  logger->set_log_level(quill::LogLevel::TraceL3);

  // enable a backtrace that will get flushed when we log CRITICAL
  logger->init_backtrace(2, quill::LogLevel::Critical);

  LOG_BACKTRACE(logger, "Backtrace log {}", 1);
  LOG_BACKTRACE(logger, "Backtrace log {}", 2);

  LOG_INFO(logger, "Welcome to Quill!");
  LOG_ERROR(logger, "An error message. error code {}", 123);
  LOG_WARNING(logger, "A warning message.");
  LOG_CRITICAL(logger, "A critical error.");
  LOG_DEBUG(logger, "Debugging foo {}", 1234);
  LOG_TRACE_L1(logger, "{:>30}", "right aligned");
  LOG_TRACE_L2(logger, "Positional arguments are {1} {0} ", "too", "supported");
  LOG_TRACE_L3(logger, "Support for floats {:03.2f}", 1.23456);
}
```

### Output
[![Screenshot-2020-08-14-at-01-09-43.png](https://i.postimg.cc/02Vbt8LH/Screenshot-2020-08-14-at-01-09-43.png)](https://postimg.cc/LnZ95M4z)

## CMake-Integration

#### External

##### Building and Installing Quill as Static Library
```
git clone https://github.com/odygrd/quill.git
mkdir cmake_build
cd cmake_build
make install
```

Note: To install in custom directory invoke cmake with `-DCMAKE_INSTALL_PREFIX=/quill/install-dir/`
##### Building and Installing Quill as Static Library With External `libfmt`
```
cmake -DCMAKE_PREFIX_PATH=/my/fmt/fmt-config.cmake-directory/ -DQUILL_FMT_EXTERNAL=ON -DCMAKE_INSTALL_PREFIX=/quill/install-dir/'
```

Then use the library from a CMake project, you can locate it directly with `find_package()`
##### Directory Structure
```
my_project/
├── CMakeLists.txt
├── main.cpp
```

##### CMakeLists.txt
```cmake
# Set only if needed - quill was installed under a custom non-standard directory
set(CMAKE_PREFIX_PATH /test_quill/usr/local/)

find_package(quill REQUIRED)

# Linking your project against quill
add_executable(example main.cpp)
target_link_libraries(example PRIVATE quill::quill)
```

##### main.cpp
See [basic usage](#basic-usage)

#### Embedded

To embed the library directly, copy the source [folder](https://github.com/odygrd/quill/tree/master/quill/quill) to your project and call `add_subdirectory()` in your `CMakeLists.txt` file

##### Directory Structure
```
my_project/
├── quill/            (source folder)
├── CMakeLists.txt
├── main.cpp
```

##### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.1.0)
project(my_project)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(quill)

add_executable(my_project main.cpp)
target_link_libraries(my_project PRIVATE quill::quill)
```
##### main.cpp
See [basic usage](#basic-usage)

## Documentation
Advanced usage and additional documentation can be found in the [wiki](https://github.com/odygrd/quill/wiki) pages.

The [examples](https://github.com/odygrd/quill/tree/master/examples) folder is also a good source of documentation.

## License
Quill is licensed under the [MIT License](https://opensource.org/licenses/MIT)

Quill depends on third party libraries with separate copyright notices and license terms.
Your use of the source code for these subcomponents is subject to the terms and conditions of the following licenses.

   - ([MIT License](https://opensource.org/licenses/MIT)) {fmt} (https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)
   - ([MIT License](https://opensource.org/licenses/MIT)) invoke.hpp (https://github.com/BlackMATov/invoke.hpp/blob/master/LICENSE.md)
   - ([MIT License](https://opensource.org/licenses/MIT)) doctest (https://github.com/onqtam/doctest/blob/master/LICENSE.txt)
