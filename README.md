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

- [Introduction](#introduction)
- [Documentation](#documentation)
- [Features](#features)
- [Caveats](#caveats)
- [Performance](#performance)
- [Basic Usage](#basic-usage)
- [CMake Integration](#cmake-integration)
- [Design](#design)
- [License](#license)

|       homebrew       |         vcpkg         |       conan       |
|:--------------------:|:---------------------:|:-----------------:|
| `brew install quill` | `vcpkg install quill` | `quill/[>=1.2.3]` |

## Introduction

Quill is a high-performance, cross-platform logging library designed for C++14 and onwards. It provides two versions of
the library:

- `v1.7`: This version is based on C++14 and focuses on bug fixes and stability improvements.
- `v2` and onwards: Starting from version 2, Quill is based on C++17 and includes new features and ongoing maintenance.

Quill is a production-ready logging library that has undergone extensive unit testing. It has been successfully utilized
in production environments, including financial trading applications, providing high-performance and reliable logging
capabilities.

## Documentation

For detailed documentation and usage instructions, please visit
the [Quill Documentation on Read the Docs](http://quillcpp.readthedocs.io/). It provides comprehensive information on
how to integrate and utilize Quill in your C++ applications.

Additionally, you can explore the [examples](http://github.com/odygrd/quill/tree/master/examples) folder in the Quill
repository on GitHub. These examples serve as valuable resources to understand different usage scenarios and demonstrate
the capabilities of the library.

## Features

- **Low Latency Logging**: Achieve fast logging performance with low latency. Refer to
  the [Benchmarks](http://github.com/odygrd/quill#performance) for more details.
- **Backend Logging Thread**: Format logs outside the critical path in a backend logging thread. For `non-built-in`
  types, the backend logging thread invokes `ostream::operator<<()` on a copy of the object. Compile-time detection of
  unsafe copying for `non-trivial user defined` types is supported. To avoid formatting them on the critical path, such
  types can be tagged as `safe-to-copy`.
  See [User Defined Types](http://quillcpp.readthedocs.io/en/latest/tutorial.html#user-defined-types) for more
  information.
- **Custom Formatters**: Customize log formatting based on user-defined patterns.
  Explore [Formatters](http://quillcpp.readthedocs.io/en/latest/tutorial.html#formatters) for further details.
- **Flexible Timestamp Generation**: Choose between rdtsc, chrono, or custom clocks (useful for simulations) for
  timestamp generation.
- **Log Stack Traces**: Store log messages in a ring buffer and display them later in response to a higher severity log
  statement or on demand. Refer
  to [Backtrace Logging](http://quillcpp.readthedocs.io/en/latest/tutorial.html#backtrace-logging) for more information.
- **Multiple Logging Targets**: Utilize various logging targets, including:
  - Console logging with color support.
  - File logging.
  - Rotating log files.
  - Time rotating log files.
  - JSON logging.
  - Custom handlers.
- **Log Message Filtering**: Apply filters to selectively process log messages. Learn more
  about [Filters](http://quillcpp.readthedocs.io/en/latest/tutorial.html#filters).
- **Structured Logging**: Generate JSON structured logs.
  See [Structured-Log](http://quillcpp.readthedocs.io/en/latest/tutorial.html#json-log) for details.
- **Blocking or Dropping Message Modes**: Choose between `blocking` or `dropping` message modes in the library.
  In `blocking` mode, the hot threads pause and wait when the queue is full until space becomes available, ensuring no
  message loss but introducing potential latency. In `dropping` mode, log messages beyond the queue's capacity may be
  dropped to prioritize low latency. The library provides reports on dropped messages, queue reallocations, and blocked
  hot threads for monitoring purposes.
- **Queue Types**: The library supports two types of queues for transferring logs from the hot path to the backend
  thread: bounded queues with a fixed capacity and unbounded queues that start small and can dynamically grow.
- **Wide Character Support**: Log messages and filenames with wide characters are supported (Windows and v1.7.x only).
- **Ordered Log Statements**: Log statements are ordered by timestamp even when produced by different threads,
  facilitating easier debugging of multithreaded applications.
- **Compile-Time Log Level Stripping**: Completely strip out log levels at compile time, reducing `if` branches.
- **Clean and Warning-Free Codebase**: Ensure a clean and warning-free codebase, even with high compiler warning levels.
- **Crash-Safe Behavior**: Benefit from crash-safe behavior with a built-in signal handler.
- **Type-Safe Python-Style API**: Utilize a type-safe API inspired by Python, with compile-time checks and built-in
  support for logging STL types/containers using the excellent [{fmt}](http://github.com/fmtlib/fmt) library.
- **Huge Pages**: Benefit from support for huge pages on the hot path. This feature allows for improved performance and
  efficiency.
- **Printf-style format**: The library offers support for the `printf` style format in addition to the existing `libfmt`
  style format. This feature is particularly useful when migrating from legacy codebases that rely on the `printf` style
  format.

## Caveats

Quill may not work well with `fork()` since it spawns a background thread and `fork()` doesn't work well
with multithreading.

If your application uses `fork()` and you want to log in the child processes as well, you should call 
`quill::start()` after the `fork()` call. Additionally, you should ensure that you write to different 
files in the parent and child processes to avoid conflicts.

For example :

```c++
#include "quill/Quill.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
  // DO NOT CALL THIS BEFORE FORK
  // quill::start();

  if (fork() == 0)
  {
    // Get or create a handler to the file - Write to a different file
    std::shared_ptr<quill::Handler> file_handler =
      quill::file_handler("child_log.log", "w", quill::FilenameAppend::DateTime);

    quill::Config cfg;
    cfg.default_handlers.push_back(std::move(file_handler));
    quill::configure(cfg);

    quill::start();

    QUILL_LOG_INFO(quill::get_logger(), "Hello from Child {}", 123);
  }
  else
  {
    // Get or create a handler to the file - Write to a different file
    std::shared_ptr<quill::Handler> file_handler =
      quill::file_handler("parent_log.log", "w", quill::FilenameAppend::DateTime);

    quill::Config cfg;
    cfg.default_handlers.push_back(std::move(file_handler));
    quill::configure(cfg);

    quill::start();

    QUILL_LOG_INFO(quill::get_logger(), "Hello from Parent {}", 123);
  }
}
```

## Performance

### Latency

#### Logging Numbers

The following message is logged 100,000 times per
thread: `LOG_INFO(logger, "Logging int: {}, int: {}, double: {}", i, j, d)`.

The results presented in the tables below are measured in nanoseconds (ns).

##### 1 Thread

| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |
|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v3.0.0 Unbounded Queue](http://github.com/odygrd/quill) |  20  |  21  |  24  |  25  |  27  |   34   |
| [Quill v3.0.0 Bounded Queue](http://github.com/odygrd/quill)   |  17  |  19  |  21  |  22  |  26  |   36   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  16  |  19  |  21  |  22  |  27  |   40   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  41  |  43  |  44  |  46  |  66  |  118   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)   |  53  |  66  |  75  |  80  |  92  |  106   |
| [Reckless](http://github.com/mattiasflodin/reckless)           |  62  |  75  |  79  |  84  |  94  |  103   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)        | 164  | 186  | 213  | 232  | 305  |  389   |         
| [spdlog](http://github.com/gabime/spdlog)                      | 694  | 761  | 838  | 887  | 996  |  1143  |    
| [g3log](http://github.com/KjellKod/g3log)                      | 5398 | 5639 | 5875 | 6025 | 6327 |  6691  |             

##### 4 Threads

| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |
|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v3.0.0 Unbounded Queue](http://github.com/odygrd/quill) |  20  |  22  |  24  |  26  |  28  |   35   |
| [Quill v3.0.0 Bounded Queue](http://github.com/odygrd/quill)   |  17  |  19  |  21  |  22  |  26  |   36   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  16  |  19  |  21  |  23  |  26  |   35   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  42  |  44  |  46  |  48  |  76  |  118   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)   |  56  |  67  |  77  |  82  |  95  |  159   |
| [Reckless](http://github.com/mattiasflodin/reckless)           |  46  |  62  |  78  |  92  | 113  |  155   |                      
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)        | 150  | 168  | 247  | 289  | 355  |  456   |
| [spdlog](http://github.com/gabime/spdlog)                      | 728  | 828  | 907  | 959  | 1140 |  1424  |
| [g3log](http://github.com/KjellKod/g3log)                      | 5103 | 5318 | 5525 | 5657 | 5927 |  6279  |

#### Logging Numbers and Large Strings

The following message is logged 100,000 times per
thread: `LOG_INFO(logger, "Logging int: {}, int: {}, string: {}", i, j, large_string)`.
The large string used in the log message is over 35 characters to prevent the short string optimization
of `std::string`.

##### 1 Thread

| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |
|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v3.0.0 Unbounded Queue](http://github.com/odygrd/quill) |  31  |  33  |  35  |  36  |  39  |   48   |
| [Quill v3.0.0 Bounded Queue](http://github.com/odygrd/quill)   |  30  |  32  |  33  |  35  |  43  |   51   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  29  |  31  |  34  |  37  |  44  |   53   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  50  |  51  |  53  |  56  |  77  |  127   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)   |  71  |  86  | 105  | 117  | 136  |  158   |
| [Reckless](http://github.com/mattiasflodin/reckless)           | 215  | 242  | 268  | 284  | 314  |  517   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)        | 172  | 191  | 218  | 238  | 312  |  401   |  
| [spdlog](http://github.com/gabime/spdlog)                      | 653  | 708  | 770  | 831  | 950  |  1083  |    
| [g3log](http://github.com/KjellKod/g3log)                      | 4802 | 4998 | 5182 | 5299 | 5535 |  5825  |

##### 4 Threads

| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |
|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v3.0.0 Unbounded Queue](http://github.com/odygrd/quill) |  31  |  33  |  35  |  37  |  40  |   48   |
| [Quill v3.0.0 Bounded Queue](http://github.com/odygrd/quill)   |  29  |  31  |  33  |  35  |  41  |   49   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  29  |  31  |  35  |  37  |  44  |   54   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  50  |  52  |  54  |  58  |  86  |  130   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)   |  69  |  82  |  99  | 111  | 134  |  194   |
| [Reckless](http://github.com/mattiasflodin/reckless)           | 187  | 209  | 232  | 247  | 291  |  562   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)        | 159  | 173  | 242  | 282  | 351  |  472   |
| [spdlog](http://github.com/gabime/spdlog)                      | 679  | 751  | 839  | 906  | 1132 |  1478  | 
| [g3log](http://github.com/KjellKod/g3log)                      | 4739 | 4955 | 5157 | 5284 | 5545 |  5898  |

The benchmark was conducted on an Ubuntu system with an Intel Xeon Gold 6254 CPU running at 3.10GHz. The benchmark used
GCC 12.2 as the compiler.

To ensure accurate performance measurement, each thread was pinned to a different CPU. The cores are also isolated on
this particular system.

The benchmark methodology involved logging 20 messages in a loop, calculating and storing the average latency for those
messages, waiting between 1-2 milliseconds, and repeating this process for a specified number of iterations.

The benchmark was executed four times for each logging library, and the reported latencies represent the second-best
result obtained.

You can find the benchmark code on the [logger_benchmarks](http://github.com/odygrd/logger_benchmarks) repository.

### Throughput

While the primary focus of the library is not on throughput, it does provide efficient handling of log messages across
multiple threads. The backend logging thread, responsible for formatting and ordering log messages from hot threads,
ensures that all queues are emptied on a high priority basis. This approach prevents the need for allocating new queues
or dropping messages on the hot path. Instead, the backend thread internally buffers log messages and writes them when
the hot thread queues are empty or when a predefined limit, `backend_thread_transit_events_soft_limit`, is reached.

Comparing throughput with other logging libraries in an asynchronous logging scenario has proven challenging. Some
libraries may drop log messages, resulting in smaller log files than expected, while others only offer asynchronous
flush, making it difficult to determine when the logging thread has finished processing all messages.

In contrast, Quill provides a blocking `flush()` guarantee, ensuring that every log message from the hot threads up to
that point is flushed to the file. The maximum throughput is measured by determining the maximum number of log messages
the backend logging thread can write to the file per second.

For benchmarking purposes, you can find the
code [here](https://github.com/odygrd/quill/blob/master/benchmarks/backend_throughput/quill_backend_throughput.cpp).
When measured on the same system as the latency benchmarks mentioned earlier, logging 4 million messages resulted in a
log file size of 476 MB. The average throughput achieved was 1.76 million messages per second, with a total elapsed time
of 2266 ms.

Please note that while Quill performs well in terms of throughput, its primary strength lies in its efficient handling
of log messages across threads.

## Basic usage

```c++
#include "quill/Quill.h"

int main()
{
  quill::configure(
    []()
    {
      // See Config.h and Tweaks.h for run time and compile time configuration options
      quill::Config cfg;
      return cfg;
    }());

  // Starts the logging backend thread
  quill::start();

  // Create a file logger
  quill::Logger* logger = quill::create_logger(
    "file_logger",
    quill::file_handler("example.log",
                        []()
                        {
                          quill::FileHandlerConfig cfg;
                          cfg.set_open_mode('w');
                          cfg.set_pattern(
                            "[%(ascii_time)] [%(thread)] [%(filename):%(lineno)] [%(logger_name)] "
                            "[%(level_name)] - %(message)",
                            "%H:%M:%S.%Qms");
                          return cfg;
                        }()));

  logger->set_log_level(quill::LogLevel::TraceL3);

  // enable a backtrace that will get flushed when we log CRITICAL
  logger->init_backtrace(2u, quill::LogLevel::Critical);

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

```c++
#include "quill/Quill.h"

int main()
{
  quill::Config cfg;
  cfg.enable_console_colours = true;
  quill::configure(cfg);
  
  quill::start();

  // Default root logger to stdout
  quill::Logger* logger = quill::get_logger();
  logger->set_log_level(quill::LogLevel::TraceL3);

  // enable a backtrace that will get flushed when we log CRITICAL
  logger->init_backtrace(2u, quill::LogLevel::Critical);

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

[![Screenshot-2020-08-14-at-01-09-43.png](http://i.postimg.cc/02Vbt8LH/Screenshot-2020-08-14-at-01-09-43.png)](http://postimg.cc/LnZ95M4z)

## CMake-Integration

#### External

##### Building and Installing Quill as Static Library

```
git clone http://github.com/odygrd/quill.git
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

To embed the library directly, copy the source [folder](http://github.com/odygrd/quill/tree/master/quill/quill) to your
project and call `add_subdirectory()` in your `CMakeLists.txt` file

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

#### Building Quill as a Shared Library (DLL) on Windows

To build Quill as a shared library (DLL) on Windows, follow these steps:

1. Add the following CMake flags when configuring the build:
   ```
   -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE -DBUILD_SHARED_LIBS=ON
   ```

2. Additionally, you need to define `QUILL_BUILD_SHARED` either in your code before including `Quill.h` or as a compiler
   flag when building outside of CMake.

#### Building Quill for Android NDK

To build Quill for Android NDK add the following CMake flags when configuring the build:

  ```
  -DQUILL_NO_THREAD_NAME_SUPPORT:BOOL=ON
  ```

## Design

![design.jpg](docs%2Fdesign.jpg)

## License

Quill is licensed under the [MIT License](http://opensource.org/licenses/MIT)

Quill depends on third party libraries with separate copyright notices and license terms.
Your use of the source code for these subcomponents is subject to the terms and conditions of the following licenses.

- ([MIT License](http://opensource.org/licenses/MIT)) {fmt} (http://github.com/fmtlib/fmt/blob/master/LICENSE.rst)
- ([MIT License](http://opensource.org/licenses/MIT)) doctest (http://github.com/onqtam/doctest/blob/master/LICENSE.txt)
