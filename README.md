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

:fire: ** Updated April 2022 ** :fire:

### Log Numbers
The following message is logged 100'000 times per thread  ```LOG_INFO(logger, "Logging int: {}, int: {}, double: {}", i, j, d)```.

The results in the tables below are in nanoseconds (ns).

#### 1 Thread

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[Quill, Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)       |  20  |  21  |  23  |  24  |  27  |  32  |  46  |
|[Quill, Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)     |  21  |  22  |  24  |  25  |  28  |  34  |  54  |
|[Quill, Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)    |  16  |  18  |  21  |  22  |  28  |  39  |  58  |
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                     |  52  |  66  |  76  |  81  |  92  |  107  |  192  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)                             |  39  |  41  |  43  |  44  |  67  |  110  |  216  |
|[fmtlog](https://https://github.com/MengRao/fmtlog)                               |  34  |  50  |  66  |  75  |  92  |  112  |  149  |
|[Reckless](https://github.com/mattiasflodin/reckless)                             |  62  |  72  |  79  |  87  |  107  |  126  |  168  |
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                          |  147  |  169  |  187  |  209  |  283  |  376  |  33623  |                
|[spdlog](https://github.com/gabime/spdlog)                                        |  626  |  675  |  721  |  755  |  877  |  1026  |  1206  |      
|[g3log](https://github.com/KjellKod/g3log)                                        |  5551  |  5759  |  5962  |  6090  |  6338  |  6647  |  7133  |               

#### 4 Threads

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[Quill, Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)      |  20  |  21  |  23  |  24  |  27  |  38  |  59  |
|[Quill, Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)    |  21  |  23  |  25  |  27  |  32  |  43  |  64  |
|[Quill, Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)   |  16  |  19  |  21  |  23  |  30  |  39  |  57  |
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                    |  53  |  67  |  77  |  82  |  93  |  131  |  236  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)                            |  39  |  42  |  43  |  46  |  73  |  119  |  243  |
|[Reckless](https://github.com/mattiasflodin/reckless)                            |  46  |  60  |  75  |  88  |  112  |  156  |  262  |                         
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                         |  140  |  173  |  239  |  273  |  336  |  432  |  43605  |
|[spdlog](https://github.com/gabime/spdlog)                                       |  665  |  742  |  825  |  880  |  1069  |  1395  |  2087  |
|[g3log](https://github.com/KjellKod/g3log)                                       |  5294  |  5532  |  5759  |  5901  |  6179  |  6521  |  7443  |

### Log Numbers and Large Strings
The following message is logged 100'000 times per thread  ```LOG_INFO(logger, "Logging int: {}, int: {}, string: {}", i, j, large_string)```.
The large string is over 35 characters to avoid short string optimisation of `std::string`

#### 1 Thread

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[Quill, Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)     |  26  |  27  |  30  |  31  |  36  |  47  |  65  |   
|[Quill, Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)   |  25  |  26  |  28  |  30  |  35  |  47  |  70  |   
|[Quill, Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)  |  116  |  132  |  145  |  153  |  168  |  185  |  214  | 
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                   |  35  |  36  |  37  |  39  |  46  |  53  |  70  |     
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)                           |  52  |  53  |  56  |  60  |  80  |  133  |  251  |
|[fmtlog](https://https://github.com/MengRao/fmtlog)                             |  37  |  42  |  48  |  59  |  93  |  126  |  356  |
|[Reckless](https://github.com/mattiasflodin/reckless)                           |  211  |  236  |  262  |  280  |  317  |  522  |  1051  |
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                        |  157  |  176  |  196  |  220  |  290  |  372  |  22812  |   
|[spdlog](https://github.com/gabime/spdlog)                                      |  652  |  715  |  775  |  827  |  953  |  1082  |  1453  |     
|[g3log](https://github.com/KjellKod/g3log)                                      |  4563  |  4752  |  4942  |  5066  |  5309  |  5633  |  6188  |  

#### 4 Threads

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[Quill, Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)      |  25  |  27  |  29  |  31  |  39  |  51  |  86  |
|[Quill, Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)    |  25  |  27  |  29  |  30  |  37  |  53  |  83  |
|[Quill, Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)   |  125  |  138  |  151  |  160  |  176  |  192  |  247  |
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                    |  34  |  35  |  36  |  38  |  45  |  53  |  100  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)                            |  51  |  53  |  55  |  60  |  85  |  128  |  243  |
|[Reckless](https://github.com/mattiasflodin/reckless)                            |  184  |  204  |  226  |  240  |  283  |  531  |  761  |
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                         |  151  |  218  |  267  |  296  |  353  |  469  |  71636  |
|[spdlog](https://github.com/gabime/spdlog)                                       |  640  |  710  |  795  |  867  |  1097  |  1465  |  2259  |    
|[g3log](https://github.com/KjellKod/g3log)                                       |  3575  |  3776  |  3967  |  4089  |  4332  |  4650  |  5544  |

The benchmarks are done on `Ubuntu - Intel(R) Xeon(R) Gold 6254 CPU @ 3.10GHz` with GCC 11.2

Each thread is pinned on a **different** cpu. Unfortunately the cores are not isolated on this system.
If the backend logging thread is run in the same CPU as the caller hot-path threads, that slows down the log message processing on the backend logging thread and will cause the SPSC queue to fill faster and re-allocate.

Continuously logging messages in a loop makes the consumer (backend logging thread) unable to follow up and the queue will have to re-allocate or block for most logging libraries expect very high throughput binary loggers like PlatformLab Nanolog.

Therefore, a different approach was followed that suits more to a real time application:
1. 20 messages are logged in a loop.
2. calculate/store the average latency for those messages.
3. wait between 1-2 ms.
4. repeat for n iterations.

I run each logger benchmark 4 times and the above latencies are the second best result.

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
