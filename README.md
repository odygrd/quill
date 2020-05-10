<div align="center">

  <a href="https://github.com/odygrd/quill">
    <img width="125" src="https://i.postimg.cc/DZrH8HkX/quill-circle-photos-v2-x2-colored-toned.png" alt="Quill logo">
  </a>
  <h1>Quill</h1>

  <div>
    <a href="https://travis-ci.org/odygrd/quill">
      <img src="https://img.shields.io/travis/odygrd/quill?logo=travis&style=flat-square" alt="Travis-ci" />
    </a>
    <a href="https://ci.appveyor.com/project/odygrd/quill/branch/master">
      <img src="https://img.shields.io/appveyor/ci/odygrd/quill?logo=appveyor&style=flat-square" alt="Appveyor-ci" />
    </a>
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
    <a href="http://opensource.org/licenses/MIT">
      <img src="https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square" alt="license" />
    </a>
    <a href="https://en.wikipedia.org/wiki/C%2B%2B14">
      <img src="https://img.shields.io/badge/language-C%2B%2B14-red.svg?style=flat-square" alt="language" />
    </a>
  </div>
  
  <p><b>Asynchronous Low Latency Logging Library</b></p>
  
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
-  **Performance** Ultra low latency for the caller threads, no formatting on the hot-path, asynchronous only mode.
-  **Convenience** Ease application monitoring/debugging. Latency is equal to latencies of binary loggers, but the produced log is in human readable form.

## Features
 -  Blazing fast. 
 -  Type safe python style formatting with compile type checks and build in support for logging STL containers, std::pair, std::tuple, std::chrono, user defined types and much more by using the excellent [{fmt}](https://github.com/fmtlib/fmt) library.
 -  Object formatting outside of the hot path with compile time checks for unsafe to copy and format later types. Non trivial user defined types can be tagged as safe-to-copy to avoid formatting on the hot path. Compile time checks for safe-to-copy user defined types.
 -  Log statements of different levels can be completely stripped out at compile time.
 -  Highly configurable log pattern. The log statements can be formatted to any format based on a user specified pattern.
 -  Log statements in timestamp order even when produced by different threads.
 -  Guaranteed non-blocking or non-guaranteed dropping logging. In guaranteed logging more memory is allocated but the caller never get's blocked and the log messages are never dropped. In non-guaranteed mode there is no heap allocation but the log messages are dropped instead.
 -  Support for wide character logging and wide character filenames (Windows only)
 -  Various log targets (Handlers)
    -  Console logging 
    -  File Logging
    -  Rotating log files
    -  Daily log files
 -  Thread safe.
 -  Clean warning-free codebase even on high warning levels.
 
## Performance

#### 1 Thread

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)   |  13  |  15  |  16  |  17  |  18  |  21  |  93  |
|[Quill v1.3.0 Bounded Queue](https://github.com/odygrd/quill)    |  14  |  17  |  20  |  22  |  27  |  39  |  211  |
|[Quill v1.3.0 Unbounded Queue](https://github.com/odygrd/quill)   |  16  |  18  |  21  |  23  |  28  |  41  |  185  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)   |  30  |  33  |  34  |  34  |  39  |  92  |  227  |
|[Reckless](https://github.com/mattiasflodin/reckless)          |  70  |  76  |  81  |  84  |  91  |  135  |  5649209  |
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)       |  72  |  75  |  79  |  123  |  173  |  264  |  34295  |
|[spdlog](https://github.com/gabime/spdlog)                     |  522  |  589  |  656  |  698  |  783  |  907  |  1595  |
|[g3log](https://github.com/KjellKod/g3log)                     |  2705  |  2850  |  2991  |  3085  |  3279  |  3530  |  3949  |
#### 4 Threads

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)   |  13  |  15  |  16  |  17  |  18  |  21  |  150  |
|[Quill v.1.30 Bounded Queue](https://github.com/odygrd/quill)   |  13  |  16  |  19  |  21  |  27  |  50  |  214  |
|[Quill v.1.30 Unbounded Queue](https://github.com/odygrd/quill)   |  14  |  18  |  21  |  23  |  31  |  87  |  1377  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)    |  30  |  33  |  34  |  35  |  42  |  90  |  203  |
|[Reckless](https://github.com/mattiasflodin/reckless)   |  99  |  103  |  106  |  109  |  134  |  343  |  31535706  |
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)   |  72  |  125  |  159  |  176  |  219  |  321  |  11402  |
|[spdlog](https://github.com/gabime/spdlog)                 |  538  |  616  |  700  |  757  |  912  |  1116  |  1592  |
|[g3log](https://github.com/KjellKod/g3log)                 |  2637  |  2792  |  2932  |  3021  |  3214  |  3485  |  4025  |


The benchmarks are done on Linux (Ubuntu/RHEL) with GCC 9.1.

Each thread is pinned on a different cpu. Note that running the backend logger thread in the same CPU as the caller threads, slows down the log message processing and will cause Quill's queue to fill faster performing a new allocation. Therefore, you will see bigger worst latencies.

The following message is logged 2'000'000 times per thread  
```LOG_INFO(logger, "Logging int: {}, int: {}, double: {}", i, j, d)```  
all reported latencies are in nanoseconds.  

Continuously Logging messages in a loop makes the consumer (hackend logging thread) unable to follow up and the queue will have to re-allocate or block for most logging libraries expect very high throughput binary loggers like PlatformLab Nanolog.
Therefore, a different approach was followed that suits more to a real time application:
1. 20 messages are logged in a loop.
2. calculate/store the average latency for those messages.
3. wait between 1-2 ms.
4. repeat for n iterations.

I run each logger benchmark four times and the above latencies are the second best result.

The benchmark code can be found [here](https://github.com/odygrd/logger_benchmarks).  
More benchmarks results (bench_results_*.txt) can be found [here](https://github.com/odygrd/logger_benchmarks).

### Verdict
If you want to use a `printf` API and only log primitive types, `PlatformLab NanoLog` is the fastest logger with the lowest latencies and high throughput when we look at 99th percentile. 
However :
1) Need to decompress a binary log file to read log each time.
2) Need to specify the type we are logging for each call to the logger.
3) To log any user defined type or a something like ```std::vector``` via `NanoLog` you would first have to convert it to a string in the hot path.  Instead, Quill copies the object and covertion to string is performed by the backend thread.

`Quill` backend is not as high throughput as `NanoLog` as it doesn't log binary. In terms of latency it is almost as fast as `Nanolog`. `Quill` is much more feature rich offering custom formatting, several logger objects, human readable log files and a superior format API that also supports user-defined types.

## Supported Platforms And Compilers
Quill requires a C++14 compiler. Minimum required versions of supported compilers are shown in the below table.

| Compiler  | Notes            |
|-----------|------------------|
| GCC       | version >= 5.0   |
| Clang     | version >= 5.0   |      
| MSVC++    | version >= 14.3  |

| Platform  | Notes                                                   |
|-----------|---------------------------------------------------------|
| Linux     | Ubuntu, RHEL, Centos, Fedora                   |                                                    |
| Windows   | Windows 10 - version 1607, Windows Server 2016 |
| macOS     | Tested with Xcode 9.4                          |

## Basic usage

```c++
#include "quill/Quill.h"

int main()
{
  // Start the logging backend thread
  quill::start();
  
  // Get a pointer to the default logger
  quill::Logger* dl = quill::get_logger();

  LOG_INFO(dl, "Welcome to Quill!");
  LOG_ERROR(dl, "An error message with error code {}, error message {}", 123, "system_error");

  LOG_WARNING(dl, "Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
  LOG_CRITICAL(dl, "Easy padding in numbers like {:08d}", 12);

  LOG_DEBUG(dl, "This message and any message below this log level will not be displayed..");

  // Enable additional log levels on this logger
  dl->set_log_level(quill::LogLevel::TraceL3);

  LOG_DEBUG(dl, "The answer is {}", 1337);
  LOG_TRACE_L1(dl, "{:>30}", "right aligned");
  LOG_TRACE_L2(dl, "Positional arguments are {1} {0} ", "too", "supported");
  LOG_TRACE_L3(dl, "Support for floats {:03.2f}", 1.23456);
}
```

### Output
By default Quill outputs to stdout using the default formatting pattern:

`ascii_time [thread_id] filename:line log_level logger_name - message`

```
01:29:06.190725386 [1783860] example_01.cpp:11 LOG_INFO     root - Welcome to Quill!
01:29:06.190727584 [1783860] example_01.cpp:12 LOG_ERROR    root - An error message with error code 123, error message system_error
01:29:06.190731526 [1783860] example_01.cpp:14 LOG_WARNING  root - Support for int: 42;  hex: 2a;  oct: 52; bin: 101010
01:29:06.190732157 [1783860] example_01.cpp:15 LOG_CRITICAL root - Easy padding in numbers like 00000012
01:29:06.190732723 [1783860] example_01.cpp:22 LOG_DEBUG    root - The answer is 1337
01:29:06.190733093 [1783860] example_01.cpp:23 LOG_TRACE_L1 root -                  right aligned
01:29:06.190735322 [1783860] example_01.cpp:24 LOG_TRACE_L2 root - Positional arguments are supported too 
01:29:06.190736334 [1783860] example_01.cpp:25 LOG_TRACE_L3 root - Support for floats 1.23
```

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
Quill is licensed under the [MIT License](http://opensource.org/licenses/MIT)

Quill depends on third party libraries with separate copyright notices and license terms. 
Your use of the source code for these subcomponents is subject to the terms and conditions of the following licenses.

   - ([MIT License](http://opensource.org/licenses/MIT)) {fmt} (https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)
   - ([MIT License](http://opensource.org/licenses/MIT)) invoke.hpp (https://github.com/BlackMATov/invoke.hpp/blob/master/LICENSE.md)
