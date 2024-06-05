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
  </div>

  <div>
    <a href="https://codecov.io/gh/odygrd/quill">
      <img src="https://img.shields.io/codecov/c/gh/odygrd/quill/master.svg?logo=codecov&style=flat-square" alt="Codecov" />
    </a>
    <a href="https://app.codacy.com/gh/odygrd/quill/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade">
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
      <img src="https://img.shields.io/badge/language-C%2B%2B17-red.svg?style=flat-square" alt="language" />
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
- [Quick Start](#quick-start)
- [Usage](#usage)
- [Design](#design)
- [License](#license)

<br>

| Package Manager |              Installation Command              |
|:---------------:|:----------------------------------------------:|
|      vcpkg      |             `vcpkg install quill`              |
|      Conan      |             `conan install quill`              |
|    Homebrew     |              `brew install quill`              |
|  Meson WrapDB   |           `meson wrap install quill`           |
|      Conda      |      `conda install -c conda-forge quill`      |
|     Bzlmod      | `bazel_dep(name = "quill", version = "x.y.z")` |
|      xmake      |             `xrepo install quill`              |

## Introduction

Quill is a high-performance, cross-platform logging library designed for C++17 and onwards.
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
- **Asynchronous logging**: Log arguments and messages are formatted in a backend logging thread, effectively offloading
  the formatting overhead from the critical path.
- **Custom Formatters**: Customize log formatting based on user-defined patterns.
  Explore [Formatters](http://quillcpp.readthedocs.io/en/latest/tutorial.html#formatters) for further details.
- **Flexible Timestamp Generation**: Choose between rdtsc, chrono, or custom clocks (useful for simulations) for
  log message timestamp generation.
- **Log Stack Traces**: Store log messages in a ring buffer and display them later in response to a higher severity log
  statement or on demand. Refer
  to [Backtrace Logging](http://quillcpp.readthedocs.io/en/latest/tutorial.html#backtrace-logging) for more information.
- **Multiple Logging Sinks**: Utilize various logging targets, including:
    - Console logging with color support.
    - File logging.
    - Rotating log files based on time or size.
    - JSON logging.
    - Custom sinks.
- **Log Message Filtering**: Apply filters to selectively process log messages. Learn more
  about [Filters](http://quillcpp.readthedocs.io/en/latest/tutorial.html#filters).
- **Structured Logging**: Generate JSON structured logs.
  See [Structured-Log](http://quillcpp.readthedocs.io/en/latest/tutorial.html#json-log) for details.
- **Blocking or Dropping Message Modes**: Choose between `blocking` or `dropping` message modes in the library.
  In `blocking` mode, the hot threads pause and wait when the queue is full until space becomes available, ensuring no
  message loss but introducing potential latency. In `dropping` mode, log messages beyond the queue's capacity may be
  dropped to prioritize low latency. The library provides reports on dropped messages, queue reallocations, and blocked
  hot threads for monitoring purposes.
- **Queue Types**: The library supports different types of queues for transferring logs from the hot path to the backend
  thread: bounded queues with a fixed capacity and unbounded queues that start small and can dynamically grow.
- **Wide Character Support**: Wide strings compatible with ASCII encoding are supported, applicable to Windows only.
  Additionally, there is support for logging STL containers consisting of wide strings. Note that chaining STL types,
  such as `std::vector<std::vector<std::wstring>>` is not supported for wide strings.
- **Ordered Log Statements**: Log statements are ordered by timestamp even when produced by different threads,
  facilitating easier debugging of multithreaded applications.
- **Compile-Time Log Level Stripping**: Completely strip out log levels at compile time, reducing `if` branches.
- **Clean and Warning-Free Codebase**: Ensure a clean and warning-free codebase, even with high compiler warning levels.
- **Crash-Safe Behavior**: Benefit from crash-safe behavior with a built-in signal handler.
- **Type-Safe API**: Type safe api using the excellent [{fmt}](http://github.com/fmtlib/fmt) library.
- **Huge Pages**: Benefit from support for huge pages on the hot path. This feature allows for improved performance and
  efficiency.

## Caveats

Quill may not work well with `fork()` since it spawns a background thread and `fork()` doesn't work well
with multithreading.

If your application uses `fork()` and you want to log in the child processes as well, you should call
`quill::start()` after the `fork()` call. Additionally, you should ensure that you write to different
files in the parent and child processes to avoid conflicts.

For example :

```c++
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

int main()
{
  // DO NOT CALL THIS BEFORE FORK
  // quill::Backend::start();

  if (fork() == 0)
  {
    quill::Backend::start();
        
    // Get or create a handler to the file - Write to a different file
    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
      "child.log");
    
    quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));

    QUILL_LOG_INFO(logger, "Hello from Child {}", 123);
  }
  else
  {
    quill::Backend::start();
          
    // Get or create a handler to the file - Write to a different file
    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
      "parent.log");
    
    quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));
    
    QUILL_LOG_INFO(logger, "Hello from Parent {}", 123);
  }
}
```

## Performance

### Latency

The results presented in the tables below are measured in `nanoseconds (ns)`.

#### Logging Numbers

`LOG_INFO(logger, "Logging int: {}, int: {}, double: {}", i, j, d)`.

##### 1 Thread Logging

| Library                                                             | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v4.4 Bounded Dropping Queue](http://github.com/odygrd/quill) |  8   |  9   |  9   |  9   |  10  |   12   |
| [fmtlog](http://github.com/MengRao/fmtlog)                          |  8   |  9   |  10  |  10  |  11  |   13   |
| [Quill v4.4 Unbounded Queue](http://github.com/odygrd/quill)        |  11  |  11  |  11  |  12  |  13  |   17   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)        |  12  |  13  |  16  |  17  |  20  |   25   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                |  19  |  19  |  19  |  20  |  56  |   83   |
| [Reckless](http://github.com/mattiasflodin/reckless)                |  25  |  27  |  29  |  31  |  33  |   39   |
| [XTR](https://github.com/choll/xtr)                                 |  6   |  6   |  39  |  42  |  47  |   59   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)             |  89  | 102  | 124  | 132  | 231  |  380   |
| [spdlog](http://github.com/gabime/spdlog)                           | 147  | 151  | 155  | 158  | 166  |  174   |    
| [g3log](http://github.com/KjellKod/g3log)                           | 1167 | 1240 | 1311 | 1369 | 1593 |  1769  |

##### 4 Threads Logging Simultaneously

| Library                                                             | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [XTR](https://github.com/choll/xtr)                                 |  6   |  6   |  8   |  9   |  40  |   48   |
| [Quill v4.4 Bounded Dropping Queue](http://github.com/odygrd/quill) |  8   |  9   |  9   |  10  |  11  |   13   |
| [fmtlog](http://github.com/MengRao/fmtlog)                          |  8   |  9   |  9   |  10  |  12  |   14   |
| [Quill v4.4 Unbounded Queue](http://github.com/odygrd/quill)        |  12  |  12  |  13  |  13  |  14  |   18   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)        |  13  |  15  |  18  |  21  |  25  |   28   |
| [Reckless](http://github.com/mattiasflodin/reckless)                |  17  |  21  |  24  |  25  |  28  |   47   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                |  19  |  19  |  20  |  21  |  58  |   88   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)             |  94  | 105  | 135  | 144  | 228  |  314   |
| [spdlog](http://github.com/gabime/spdlog)                           | 209  | 248  | 297  | 330  | 423  |  738   | 
| [g3log](http://github.com/KjellKod/g3log)                           | 1253 | 1332 | 1393 | 1437 | 1623 |  2063  |

#### Logging Large Strings

`LOG_INFO(logger, "Logging int: {}, int: {}, string: {}", i, j, large_string)`.

The large string used in the log message is over 35 characters to prevent the short string optimization
of `std::string`.

##### 1 Thread Logging

| Library                                                             | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v4.4 Bounded Dropping Queue](http://github.com/odygrd/quill) |  10  |  11  |  12  |  13  |  13  |   16   |
| [fmtlog](http://github.com/MengRao/fmtlog)                          |  10  |  12  |  13  |  14  |  16  |   17   | 
| [Quill v4.4 Unbounded Queue](http://github.com/odygrd/quill)        |  13  |  13  |  14  |  15  |  16  |   19   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)        |  15  |  18  |  22  |  25  |  29  |   34   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                |  20  |  21  |  22  |  23  |  58  |   86   |
| [XTR](https://github.com/choll/xtr)                                 |  8   |  8   |  29  |  30  |  33  |   49   |
| [Reckless](http://github.com/mattiasflodin/reckless)                |  89  | 108  | 115  | 117  | 123  |  141   | 
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)             |  94  | 106  | 125  | 133  | 240  |  388   |
| [spdlog](http://github.com/gabime/spdlog)                           | 123  | 126  | 130  | 133  | 140  |  148   |
| [g3log](http://github.com/KjellKod/g3log)                           | 890  | 966  | 1028 | 1119 | 1260 |  1463  |

##### 4 Threads Logging Simultaneously

| Library                                                             | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v4.4 Bounded Dropping Queue](http://github.com/odygrd/quill) |  11  |  11  |  13  |  13  |  14  |   17   |
| [XTR](https://github.com/choll/xtr)                                 |  9   |  11  |  13  |  14  |  31  |   39   |
| [Quill v4.4 Unbounded Queue](http://github.com/odygrd/quill)        |  13  |  14  |  15  |  16  |  17  |   20   |
| [fmtlog](http://github.com/MengRao/fmtlog)                          |  12  |  13  |  16  |  16  |  19  |   21   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                |  21  |  22  |  23  |  25  |  60  |   90   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)        |  19  |  24  |  33  |  36  |  42  |   49   |
| [Reckless](http://github.com/mattiasflodin/reckless)                |  82  |  96  | 104  | 108  | 118  |  145   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)             |  57  |  96  | 123  | 137  | 172  |  302   |
| [spdlog](http://github.com/gabime/spdlog)                           | 185  | 207  | 237  | 257  | 362  |  669   |
| [g3log](http://github.com/KjellKod/g3log)                           | 983  | 1046 | 1112 | 1171 | 1376 |  1774  |

#### Logging Complex Types

`LOG_INFO(logger, "Logging int: {}, int: {}, vector: {}", i, j, v)`.

Logging `std::vector<std::string> v` containing 16 large strings, each ranging from 50 to 60 characters.
The strings used in the log message are over 35 characters to prevent the short string optimization of `std::string`.

##### 1 Thread Logging

| Library                                                             | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v4.4 Bounded Dropping Queue](http://github.com/odygrd/quill) |  50  |  52  |  54  |  56  |  59  |   74   |
| [Quill v4.4 Unbounded Queue](http://github.com/odygrd/quill)        |  53  |  55  |  56  |  58  |  61  |   67   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                |  64  |  66  |  70  |  80  |  89  |  271   |
| [XTR](https://github.com/choll/xtr)                                 | 282  | 290  | 338  | 343  | 350  |  575   |
| [fmtlog](http://github.com/MengRao/fmtlog)                          | 721  | 750  | 779  | 793  | 821  |  847   |
| [spdlog](http://github.com/gabime/spdlog)                           | 5881 | 5952 | 6026 | 6082 | 6342 |  6900  |

##### 4 Threads Logging Simultaneously

| Library                                                             | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v4.4 Bounded Dropping Queue](http://github.com/odygrd/quill) |  53  |  55  |  57  |  59  |  62  |   80   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                |  66  |  68  |  71  |  74  |  87  |  295   | 
| [Quill v4.4 Unbounded Queue](http://github.com/odygrd/quill)        |  88  |  95  | 103  | 108  | 119  |  135   |
| [XTR](https://github.com/choll/xtr)                                 | 535  | 730  | 786  | 819  | 885  |  971   |
| [fmtlog](http://github.com/MengRao/fmtlog)                          | 788  | 811  | 831  | 844  | 872  |  906   |
| [spdlog](http://github.com/gabime/spdlog)                           | 6090 | 6165 | 6246 | 6337 | 7351 |  9322  |

The benchmark was conducted on `Linux RHEL 9` with an `Intel Core i5-12600` at 4.8 GHz.
The cpus are isolated on this system and each thread was pinned to a different CPU. `GCC 13.1` was used as the compiler.

The benchmark methodology involved logging 20 messages in a loop, calculating and storing the average latency for those
20 messages, then waiting around ~2 milliseconds, and repeating this process for a specified number of iterations.

_In the `Quill Bounded Dropping` benchmarks, the dropping queue size is set to `262,144` bytes, which is double the
default size of `131,072` bytes._

You can find the benchmark code on the [logger_benchmarks](http://github.com/odygrd/logger_benchmarks) repository.

### Throughput

The maximum throughput is measured by determining the maximum number of log messages the backend logging thread can
write to the log file per second.

When measured on the same system as the latency benchmarks mentioned above the average throughput of the backend
logging thread when formatting a log message consisting of an int and a double is ~`4.40 million msgs/sec`

While the primary focus of the library is not on throughput, it does provide efficient handling of log messages across
multiple threads. The backend logging thread, responsible for formatting and ordering log messages from hot threads,
ensures that all queues are emptied on a high priority basis. The backend thread internally buffers the log messages
and then writes them later when the caller thread queues are empty or when a predefined limit,
`backend_thread_transit_events_soft_limit`, is reached. This approach prevents the need for allocating new queues
or dropping messages on the hot path.

Comparing throughput with other logging libraries in an asynchronous logging scenario has proven challenging. Some
libraries may drop log messages, resulting in smaller log files than expected, while others only offer asynchronous
flush, making it difficult to determine when the logging thread has finished processing all messages.
In contrast, Quill provides a blocking flush log guarantee, ensuring that every log message from the hot threads up to
that point is flushed to the file.

For benchmarking purposes, you can find the
code [here](https://github.com/odygrd/quill/blob/master/benchmarks/backend_throughput/quill_backend_throughput.cpp).

### Compilation Time

Compile times are measured using `clang 15` and for `Release` build.

Below, you can find the additional headers that the library will include when you need to log, following
the [recommended_usage](https://github.com/odygrd/quill/blob/master/examples/recommended_usage/recommended_usage.cpp)
example

![quill_v4_1_compiler_profile.speedscope.png](docs%2Fquill_v4_1_compiler_profile.speedscope.png)

There is also a compile-time benchmark measuring the compilation time of 2000 auto-generated log statements with
various arguments. You can find
it [here](https://github.com/odygrd/quill/blob/master/benchmarks/compile_time/compile_time_bench.cpp). It takes
approximately 30 seconds to compile.

![quill_v4_compiler_bench.speedscope.png](docs%2Fquill_v4_compiler_bench.speedscope.png)

## Quick Start

```c++
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

int main()
{
  // Start the backend thread
  quill::Backend::start();

  // Log to file
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    "example_file_logging.log");

  quill::Logger* logger =
    quill::Frontend::create_or_get_logger("root", std::move(file_sink));

  // set the log level of the logger to trace_l3 (default is info)
  logger->set_log_level(quill::LogLevel::TraceL3);
  
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
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

int main()
{
  // Start the backend thread
  quill::Backend::start();

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

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

## Usage

#### External CMake

##### Building and Installing Quill

```
git clone http://github.com/odygrd/quill.git
mkdir cmake_build
cd cmake_build
cmake ..
make install
```

Note: To install in custom directory invoke cmake with `-DCMAKE_INSTALL_PREFIX=/quill/install-dir/`

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
target_link_libraries(example PUBLIC quill::quill)
```

#### Embedded CMake

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

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(quill)

add_executable(my_project main.cpp)
target_link_libraries(my_project PUBLIC quill::quill)
```

#### Building Quill for Android NDK

To build quill for Android NDK add the following flags when configuring the build:

  ```
  -DQUILL_NO_THREAD_NAME_SUPPORT:BOOL=ON
  ```

#### Meson

##### Using WrapDB

Meson's `wrapdb` includes a quill package, which repackages quill to be built by meson as a subproject.

- Install quill subproject from the wrapdb by running from the root of your project

  ```meson
  meson wrap install quill
  ```

##### Manual Integration

If you prefer not to use WrapDB, you can manually integrate Quill into your project by following these steps:

- Copy the contents of this repository under the subprojects directory in your project.

##### Integration Steps

Once the library is integrated into your Meson project, follow these steps to ensure proper usage:

- In your project’s `meson.build` file, add an entry for the new subproject

  ```meson
  quill = subproject('quill')
  quill_dep = quill.get_variable('quill_dep')
  ```

- Include the new dependency object to link with quill

  ```meson
  my_build_target = executable('name', 'main.cpp', dependencies : [quill_dep], install : true)
  ```

#### Bazel

##### Using Blzmod

The library is available on BLZMOD, allowing for easy integration into your project.

##### Manual Integration

If you prefer manual integration, you can add the library as a dependency in your `BUILD.bazel` file. Below is a sample
`cc_binary` rule demonstrating how to include the library. Ensure to replace `//quill_path` with the actual path to the
directory containing the `BUILD.bazel` file for the quill library within your project structure.

  ```bazel
  cc_binary(name = "app", srcs = ["main.cpp"], deps = ["//quill_path:quill"])
  ```

## Design

### Frontend (caller-thread)

When invoking a `LOG_` macro:

1. Creates a static constexpr metadata object to store `Metadata` such as the format string and source location.

2. Pushes the data SPSC lock-free queue. For each log message, the following variables are pushed

| Variable   |                                                  Description                                                   |
|------------|:--------------------------------------------------------------------------------------------------------------:|
| timestamp  |                                               Current timestamp                                                |
| Metadata*  |                                        Pointer to metadata information                                         |
| Logger*    |                                         Pointer to the logger instance                                         |
| DecodeFunc | A pointer to a templated function containing all the log message argument types, used for decoding the message |
| Args...    |           A serialized binary copy of each log message argument that was passed to the `LOG_` macro            |

### Backend

Consumes each message from the SPSC queue, retrieves all the necessary information and then formats the message.
Subsequently, forwards the log message to all Sinks associated with the Logger.

![design.jpg](docs%2Fdesign.jpg)

## License

Quill is licensed under the [MIT License](http://opensource.org/licenses/MIT)

Quill depends on third party libraries with separate copyright notices and license terms.
Your use of the source code for these subcomponents is subject to the terms and conditions of the following licenses.

- ([MIT License](http://opensource.org/licenses/MIT)) {fmt} (http://github.com/fmtlib/fmt/blob/master/LICENSE.rst)
- ([MIT License](http://opensource.org/licenses/MIT)) doctest (http://github.com/onqtam/doctest/blob/master/LICENSE.txt)
