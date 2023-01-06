<div align="center">
  <a href="https://github.com/odygrd/quill">
    <img width="125" src="https://i.postimg.cc/DZrH8HkX/quill-circle-photos-v2-x2-colored-toned.png" alt="Quill logo">
  </a>
  <h1>Quill</h1>

  <div>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Alinux">
      <img src="https://img.shields.io/github/actions/workflow/status/odygrd/quill/linux.yml?branch=master&label=linux&logo=linux&style=flat-square" alt="linux-ci" />
    </a>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Amacos">
      <img src="https://img.shields.io/github/actions/workflow/status/odygrd/quill/macos.yml?branch=master&label=macos&logo=apple&logoColor=white&style=flat-square" alt="macos-ci" />
    </a>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Awindows">
      <img src="https://img.shields.io/github/actions/workflow/status/odygrd/quill/windows.yml?branch=master&label=windows&logo=windows&logoColor=blue&style=flat-square" alt="windows-ci" />
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
    <a href="https://en.wikipedia.org/wiki/C%2B%2B17">
      <img src="https://img.shields.io/badge/language-C%2B%2B17%20%2F%20C%2B%2B14-red.svg?style=flat-square" alt="language" />
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
| `brew install quill`  | `vcpkg install quill`  | `quill/[>=1.2.3]` |

## Introduction

Quill is a cross-platform low latency logging library based on C++14/C++17.

There are two versions on the library:

`v2` : C++17
`v1.7` : C++14

Going forward any new features will only be added to the C++17 version of the library.
The old library (v1.7.x) still remains there and there will be releases only for bug fixes.

The main goals of the library are:

- **Simplicity** A small example code snippet should be enough to get started and use most of the features.
- **Performance** Ultra low latency. No formatting on the hot-path, asynchronous only mode. No hot-path allocations for
  fundamental types, enums and strings (including `std::string` and `std::string_view`). Any other custom or user
  defined type gets copy constructed with the formatting done on a backend worker thread.
- **Convenience** Ease application monitoring/debugging. Latency is equal to latencies of binary loggers, but the
  produced log is in human-readable form.

## Features

- Log anything - Blazing fast. See [Benchmarks](https://github.com/odygrd/quill#performance).
- Format outside the hot-path in a backend logging thread. For `non-built-in` types `ostream::operator<<()` is called on
  a copy of the object by the backend logging thread. Unsafe to copy `non-trivial user defined` are detected in compile
  time. Those types can be tagged as `safe-to-copy` to avoid formatting them on the hot path.
  See [User Defined Types](https://github.com/odygrd/quill/wiki/8.-User-Defined-Types).
- Custom formatters. Logs can be formatted based on a user specified pattern.
  See [Formatters](https://github.com/odygrd/quill/wiki/4.-Formatters).
 -  Support for log stack traces. Store log messages in a ring buffer and display later on a higher severity log statement or on demand. See [Backtrace Logging](https://github.com/odygrd/quill/wiki/6.-Backtrace-Logging).
 -  Various logging targets. See [Handlers](https://github.com/odygrd/quill/wiki/2.-Handlers).
    -  Console logging with colours support.
    -  File Logging
    -  Rotating log files
    -  Time rotating log files
    -  JSON logging
    -  Custom Handlers
 -  Filters for filtering log messages. See [Filters](https://github.com/odygrd/quill/wiki/3.-Filters).
- Ability to produce structured log. See [Structured-Log](https://github.com/odygrd/quill/wiki/10.-Structured-Log)
- `guaranteed non-blocking` or `non-guaranteed` logging. In `non-guaranteed` mode there is no heap allocation of a new
  queue but log messages can be dropped. See [FAQ](https://github.com/odygrd/quill/wiki/7.-FAQ#guaranteed-logging-mode).
- Support for wide character logging and wide character filenames (Windows and v1.7.x only).
- Log statements in timestamp order even when produced by different threads. This makes debugging
  multithreading applications easier.
 -  Log levels can be completely stripped out at compile time reducing `if` branches.
 -  Clean warning-free codebase even on high warning levels.
 -  Crash safe behaviour with a build-in signal handler.
 -  Type safe python style API with compile type checks and built-in support for logging STL types/containers by using the excellent [{fmt}](https://github.com/fmtlib/fmt) library.

## Performance 

:fire: ** Updated January 2023 ** :fire:

### Log Numbers
The following message is logged 100'000 times per thread  ```LOG_INFO(logger, "Logging int: {}, int: {}, double: {}", i, j, d)```.

The results in the tables below are in nanoseconds (ns).

#### 1 Thread

| Library                                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th | Worst |
|--------------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|:-----:|
| [Quill v2.6.0 Unbounded Queue](https://github.com/odygrd/quill)                     |  18  |  21  |  24  |  25  |  27  |  33  |  53  |
| [Quill v1.7 Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)   |  16  |  18  |  20  |  22  |  26  |  32  |  55  |
| [Quill v1.7 Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)  |  15  |  17  |  19  |  21  |  26  |  36  |  51  |
| [Quill v1.7 Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)     |  16  |  17  |  19  |  20  |  25  |  29  |  47  |
| [fmtlog](https://github.com/MengRao/fmtlog)                                         |  17  |  19  |  21  |  22  |  25  |  34  |  62  |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                       |  53  |  66  |  75  |  80  |  92  |  106  |  199  |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                               |  41  |  43  |  44  |  46  |  66  |  118  |  236  |
| [Reckless](https://github.com/mattiasflodin/reckless)                               |  62  |  75  |  79  |  84  |  94  |  103  |  158  |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                            |  164  |  186  |  213  |  232  |  305  |  389  |  24257  |           
| [spdlog](https://github.com/gabime/spdlog)                                          |  694  |  761  |  838  |  887  |  996  |  1143  |  1762  |      
| [g3log](https://github.com/KjellKod/g3log)                                          |  5398  |  5639  |  5875  |  6025  |  6327  |  6691  |  7545  |              

#### 4 Threads

| Library                                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th | Worst |
|--------------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|:-----:|
| [Quill v2.6.0 Unbounded Queue](https://github.com/odygrd/quill)                     |  18  |  21  |  23  |  25  |  28  |  32  |  60  |
| [Quill v1.7 Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)   |  16  |  19  |  22  |  24  |  32  |  45  |  59  |
| [Quill v1.7 Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)  |  15  |  18  |  21  |  23  |  30  |  40  |  57  |
| [Quill v1.7 Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)     |  16  |  18  |  21  |  23  |  29  |  42  |  61  |
| [fmtlog](https://github.com/MengRao/fmtlog)                                         |  15  |  18  |  21  |  22  |  25  |  32  |  68  |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                       |  56  |  67  |  77  |  82  |  95  |  159  |  340  |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                               |  42  |  44  |  46  |  48  |  76  |  118  |  214  |
| [Reckless](https://github.com/mattiasflodin/reckless)                               |  46  |  62  |  78  |  92  |  113  |  155  |  229  |                       
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                            |  150  |  168  |  247  |  289  |  355  |  456  |  25126  |
| [spdlog](https://github.com/gabime/spdlog)                                          |  728  |  828  |  907  |  959  |  1140  |  1424  |  2060  |
| [g3log](https://github.com/KjellKod/g3log)                                          |  5103  |  5318  |  5525  |  5657  |  5927  |  6279  |  7290  |

### Log Numbers and Large Strings
The following message is logged 100'000 times per thread  ```LOG_INFO(logger, "Logging int: {}, int: {}, string: {}", i, j, large_string)```.
The large string is over 35 characters to avoid short string optimisation of `std::string`

#### 1 Thread

| Library                                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th | Worst |
|--------------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|:-----:|
| [Quill v2.6.0 Unbounded Queue](https://github.com/odygrd/quill)                      |  32  |  33  |  35  |  37  |  40  |  44  |  70  | 
| [Quill v1.7 Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)    |  26  |  28  |  29  |  31  |  35  |  45  |  68  |
| [Quill v1.7 Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)   |  122  |  136  |  148  |  156  |  170  |  187  |  223  |
| [Quill v1.7 Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)      |  27  |  29  |  31  |  32  |  36  |  44  |  64  |
| [fmtlog](https://github.com/MengRao/fmtlog)                                          |  29  |  31  |  34  |  36  |  41  |  50  |  83  |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                        |  71  |  86  |  105  |  117  |  136  |  158  |  247  |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                                |  50  |  51  |  53  |  56  |  77  |  127  |  234  |
| [Reckless](https://github.com/mattiasflodin/reckless)                                |  215  |  242  |  268  |  284  |  314  |  517  |  830  |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                             |  172  |  191  |  218  |  238  |  312  |  401  |  55110  |   
| [spdlog](https://github.com/gabime/spdlog)                                           |  653  |  708  |  770  |  831  |  950  |  1083  |  1272  |    
| [g3log](https://github.com/KjellKod/g3log)                                           |  4802  |  4998  |  5182  |  5299  |  5535  |  5825  |  6525  |

#### 4 Threads

| Library                                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th | Worst |
|--------------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|:-----:|
| [Quill v2.6.0 Unbounded Queue](https://github.com/odygrd/quill)                       |  31  |  33  |  36  |  37  |  41  |  47  |  81  |
| [Quill v1.7 Dual Queue Enabled, Unbounded Queue](https://github.com/odygrd/quill)     |  27  |  29  |  31  |  32  |  37  |  47  |  75  |
| [Quill v1.7 Dual Queue Disabled, Unbounded Queue](https://github.com/odygrd/quill)    |  127  |  141  |  157  |  168  |  185  |  203  |  227  |
| [Quill v1.7 Dual Queue Enabled, Bounded Queue](https://github.com/odygrd/quill)       |  27  |  29  |  31  |  32  |  39  |  51  |  100  |
| [fmtlog](https://github.com/MengRao/fmtlog)                                           |  28  |  30  |  33  |  35  |  41  |  51  |  82  |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)                         |  69  |  82  |  99  |  111  |  134  |  194  |  321  |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                                 |  50  |  52  |  54  |  58  |  86  |  130  |  246  |
| [Reckless](https://github.com/mattiasflodin/reckless)                                 |  187  |  209  |  232  |  247  |  291  |  562  |  818  |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                              |  159  |  173  |  242  |  282  |  351  |  472  |  66730  |
| [spdlog](https://github.com/gabime/spdlog)                                            |  679  |  751  |  839  |  906  |  1132  |  1478  |  2190  |  
| [g3log](https://github.com/KjellKod/g3log)                                            |  4739  |  4955  |  5157  |  5284  |  5545  |  5898  |  6823  |

The benchmarks are done on `Ubuntu - Intel(R) Xeon(R) Gold 6254 CPU @ 3.10GHz` with GCC 12.2

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
Quill v1.7.x requires a C++14 compiler. Minimum required versions of supported compilers are shown in the below table.

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
  quill::Config cfg;
  cfg.enable_console_colours = true;
  quill::configure(cfg);
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
