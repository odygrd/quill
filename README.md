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
-  [Documentation](#documentation)
-  [Features](#features)
-  [Performance](#performance)
-  [Basic Usage](#basic-usage)
- [CMake Integration](#cmake-integration)
- [Design](#design)
- [License](#license)

|  homebrew             |  vcpkg                 |  conan            |
|:---------------------:|:----------------------:|:-----------------:|
| `brew install quill`  | `vcpkg install quill`  | `quill/[>=1.2.3]` |

## Introduction

Quill is a cross-platform low latency logging library based on C++14/C++17.

There are two versions on the library:

`v1.7` : C++14 - Bug fix only

`v2` : C++17 - New features and actively maintained

## Documentation

[ReadtheDocs](http://quillcpp.readthedocs.io/)

The [examples](http://github.com/odygrd/quill/tree/master/examples) folder is also a good source of documentation.

## Features

- Low latency logging See [Benchmarks](http://github.com/odygrd/quill#performance).
- Format outside the hot-path in a backend logging thread. For `non-built-in` types `ostream::operator<<()` is called on
  a copy of the object by the backend logging thread. Unsafe to copy `non-trivial user defined` are detected in compile
  time. Those types can be tagged as `safe-to-copy` to avoid formatting them on the hot path.
  See [User Defined Types](http://quillcpp.readthedocs.io/en/latest/tutorial.html#user-defined-types).
- Custom formatters. Logs can be formatted based on a user specified pattern.
  See [Formatters](http://quillcpp.readthedocs.io/en/latest/tutorial.html#formatters).
- Support for rdtsc, chrono or custom clock (usefull for simulations) for timestamp generation.
- Support for log stack traces. Store log messages in a ring buffer and display later on a higher severity log statement
  or on demand. See [Backtrace Logging](http://quillcpp.readthedocs.io/en/latest/tutorial.html#backtrace-logging).
- Various logging targets. See [Handlers](http://quillcpp.readthedocs.io/en/latest/tutorial.html#handlers).
    - Console logging with colours support.
    - File Logging
    - Rotating log files
    - Time rotating log files
    - JSON logging
    - Custom Handlers
- Filters for filtering log messages. See [Filters](http://quillcpp.readthedocs.io/en/latest/tutorial.html#filters).
- Ability to produce JSON structured log. See [Structured-Log](http://quillcpp.readthedocs.io/en/latest/tutorial.html#json-log)
- `guaranteed non-blocking` or `non-guaranteed` logging. In `non-guaranteed` mode there is no heap allocation of a new
  queue but log messages can be dropped. See [FAQ](http://quillcpp.readthedocs.io/en/latest/features.html#guaranteed-logging).
- Support for wide character logging and wide character filenames (Windows and v1.7.x only).
- Log statements in timestamp order even when produced by different threads. This makes debugging
  multithreading applications easier.
- Log levels can be completely stripped out at compile time reducing `if` branches.
- Clean warning-free codebase even on high warning levels.
- Crash safe behaviour with a built-in signal handler.
- Type safe python style API with compile type checks and built-in support for logging STL types/containers by using the
  excellent [{fmt}](http://github.com/fmtlib/fmt) library.

## Performance

:fire: ** Updated January 2023 ** :fire:

### Latency

#### Log Numbers

The following message is logged 100'000 times per
thread  ```LOG_INFO(logger, "Logging int: {}, int: {}, double: {}", i, j, d)```.

The results in the tables below are in nanoseconds (ns).

##### 1 Thread

| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |
|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v2.7.0 Unbounded Queue](http://github.com/odygrd/quill) |  20  |  21  |  24  |  25  |  27  |   34   |
| [Quill v2.7.0 Bounded Queue](http://github.com/odygrd/quill)   |  17  |  19  |  21  |  22  |  26  |   36   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  16  |  19  |  21  |  22  |  27  |   40   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  41  |  43  |  44  |  46  |  66  |  118   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)   |  53  |  66  |  75  |  80  |  92  |  106   |
| [Reckless](http://github.com/mattiasflodin/reckless)           |  62  |  75  |  79  |  84  |  94  |  103   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)        | 164  | 186  | 213  | 232  | 305  |  389   |         
| [spdlog](http://github.com/gabime/spdlog)                      | 694  | 761  | 838  | 887  | 996  |  1143  |    
| [g3log](http://github.com/KjellKod/g3log)                      | 5398 | 5639 | 5875 | 6025 | 6327 |  6691  |             

##### 4 Threads

| Library                                                                           | 50th | 75th | 90th | 95th | 99th | 99.9th |
|-----------------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v2.7.0 Unbounded Queue](http://github.com/odygrd/quill)                    |  20  |  22  |  24  |  26  |  28  |   35   |
| [Quill v2.7.0 Bounded Queue](http://github.com/odygrd/quill)                      |  17  |  19  |  21  |  22  |  26  |   36   |
| [fmtlog](http://github.com/MengRao/fmtlog)                                        |  16  |  19  |  21  |  23  |  26  |   35   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                              |  42  |  44  |  46  |  48  |  76  |  118   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)                      |  56  |  67  |  77  |  82  |  95  |  159   |
| [Reckless](http://github.com/mattiasflodin/reckless)                              |  46  |  62  |  78  |  92  | 113  |  155   |                      
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                           | 150  | 168  | 247  | 289  | 355  |  456   |
| [spdlog](http://github.com/gabime/spdlog)                                         | 728  | 828  | 907  | 959  | 1140 |  1424  |
| [g3log](http://github.com/KjellKod/g3log)                                         | 5103 | 5318 | 5525 | 5657 | 5927 |  6279  |

#### Log Numbers and Large Strings
The following message is logged 100'000 times per thread  ```LOG_INFO(logger, "Logging int: {}, int: {}, string: {}", i, j, large_string)```.
The large string is over 35 characters to avoid short string optimisation of `std::string`

##### 1 Thread

| Library                                                                           | 50th | 75th | 90th | 95th | 99th | 99.9th |
|-----------------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v2.7.0 Unbounded Queue](http://github.com/odygrd/quill)                    |  31  |  33  |  35  |  36  |  39  |   48   |
| [Quill v2.7.0 Bounded Queue](http://github.com/odygrd/quill)                      |  30  |  32  |  33  |  35  |  43  |   51   |
| [fmtlog](http://github.com/MengRao/fmtlog)                                        |  29  |  31  |  34  |  37  |  44  |   53   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                              |  50  |  51  |  53  |  56  |  77  |  127   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)                      |  71  |  86  | 105  | 117  | 136  |  158   |
| [Reckless](http://github.com/mattiasflodin/reckless)                              | 215  | 242  | 268  | 284  | 314  |  517   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                           | 172  | 191  | 218  | 238  | 312  |  401   |  
| [spdlog](http://github.com/gabime/spdlog)                                         | 653  | 708  | 770  | 831  | 950  |  1083  |    
| [g3log](http://github.com/KjellKod/g3log)                                         | 4802 | 4998 | 5182 | 5299 | 5535 |  5825  |

##### 4 Threads

| Library                                                                           | 50th | 75th | 90th | 95th | 99th | 99.9th |
|-----------------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill v2.7.0 Unbounded Queue](http://github.com/odygrd/quill)                    |  31  |  33  |  35  |  37  |  40  |   48   |
| [Quill v2.7.0 Bounded Queue](http://github.com/odygrd/quill)                      |  29  |  31  |  33  |  35  |  41  |   49   |
| [fmtlog](http://github.com/MengRao/fmtlog)                                        |  29  |  31  |  35  |  37  |  44  |   54   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                              |  50  |  52  |  54  |  58  |  86  |  130   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)                      |  69  |  82  |  99  | 111  | 134  |  194   |
| [Reckless](http://github.com/mattiasflodin/reckless)                              | 187  | 209  | 232  | 247  | 291  |  562   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                           | 159  | 173  | 242  | 282  | 351  |  472   |
| [spdlog](http://github.com/gabime/spdlog)                                         | 679  | 751  | 839  | 906  | 1132 |  1478  | 
| [g3log](http://github.com/KjellKod/g3log)                                         | 4739 | 4955 | 5157 | 5284 | 5545 |  5898  |

The benchmarks are done on `Ubuntu - Intel(R) Xeon(R) Gold 6254 CPU @ 3.10GHz` with GCC 12.2

Each thread is pinned on a **different** cpu. Unfortunately the cores are not isolated on this system.
If the backend logging thread is run in the same CPU as the caller hot-path threads, that slows down the log message processing on the backend logging thread and will cause the SPSC queue to fill faster and re-allocate.

Continuously logging messages in a loop makes the consumer (backend logging thread) unable to follow up and the queue will have to re-allocate or block for most logging libraries expect very high throughput binary loggers like PlatformLab Nanolog.

Therefore, a different approach was followed that suits more to a real time application:

1. 20 messages are logged in a loop.
2. calculate/store the average latency for those messages.
3. wait between 1-2 ms.
4. repeat for n iterations.

I run each logger benchmark 4 times and the above latencies are the second-best result.

The benchmark code and results can be found [here](http://github.com/odygrd/logger_benchmarks).

### Throughput

The main focus of the library is not throughput. The backend logging thread is a single thread responsible for
formatting, ordering the log messages from multiple hot threads and finally outputing everything as human readable text.
The logging thread always empties all the queues of the hot threads on the highest priority (to avoid allocating a new
queue or dropping messages on the hot path). To achieve that, it internally buffers the log messages and then
writes them later when the hot thread queues are empty or when a limit is reached `backend_thread_max_transit_events`.

I haven't found an easy way to compare the throughput against other logging libraries while doing asynchronous logging.
For example some libraries will drop the log messages ending in producing much smaller log files than the expected,
other libraries only offer an async flush meaning that you never really know when the logging thread has finished
processing everything.

Quill has a blocking `flush()` guaranteeing every single log message from the hot threads up to that point is flushed to
the file.
The maximum throughput is measured as the max log messages number the backend logging thread can write to the file per
second.
The code can be
found [here](https://github.com/odygrd/quill/blob/master/benchmarks/backend_throughput/quill_backend_throughput.cpp)

Measured on the same system as the latency benchmarks above for 4 million messages produces a log file of 476 mb

```
1.57 million msgs/sec average, total time elapsed 2550 ms, total log messages 4 million
```

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

## Design

![design.jpg](docs%2Fdesign.jpg)

## License

Quill is licensed under the [MIT License](http://opensource.org/licenses/MIT)

Quill depends on third party libraries with separate copyright notices and license terms.
Your use of the source code for these subcomponents is subject to the terms and conditions of the following licenses.

- ([MIT License](http://opensource.org/licenses/MIT)) {fmt} (http://github.com/fmtlib/fmt/blob/master/LICENSE.rst)
- ([MIT License](http://opensource.org/licenses/MIT)) doctest (http://github.com/onqtam/doctest/blob/master/LICENSE.txt)
