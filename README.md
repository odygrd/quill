<h2> <img src="https://i.postimg.cc/FssWB25k/quill-logo.png" alt="Quill logo" width="140"><br>Asynchronous Low Latency Logging Library</h2>

[![travis][badge.travis]][travis]
[![appveyor][badge.appveyor]][appveyor]
[![codecov][badge.codecov]][codecov]
[![language][badge.language]][language]
[![license][badge.license]][license]
[![project_status: The project has reached a stable, usable state and is being actively developed.][badge.project_status]][project_status]

[badge.travis]: https://img.shields.io/travis/odygrd/quill/master.svg?logo=travis
[badge.appveyor]: https://img.shields.io/appveyor/ci/odygrd/quill/master.svg?logo=appveyor
[badge.codecov]: https://img.shields.io/codecov/c/gh/odygrd/quill/master.svg?logo=codecov 
[badge.language]: https://img.shields.io/badge/language-C%2B%2B14-red.svg
[badge.license]: https://img.shields.io/badge/license-MIT-blue.svg
[badge.project_status]: https://www.repostatus.org/badges/latest/active.svg
 
[travis]: https://travis-ci.org/odygrd/quill
[appveyor]: https://ci.appveyor.com/project/odygrd/quill
[codecov]: https://codecov.io/gh/odygrd/quill
[language]: https://en.wikipedia.org/wiki/C%2B%2B14
[license]: http://opensource.org/licenses/MIT
[project_status]: https://www.repostatus.org/#active

- [Design Rationale](#design-rationale)
- [Features](#features)
- [Performance](#performance)
- [Supported Platforms And Compilers](#supported-platforms-and-compilers)
- [Integration](#integration)
  - [CMake](#cmake)
  - [Package Managers](#package-managers)
- [Basic Usage](#basic-usage)
- [Documentation](#documentation)
- [License](#license)

## Design Rationale
The library aims to make logging significantly easier for the application developer while at the same time reduce the overhead of logging in the critical path as much as possible.

The main goals of the library are:

- **Simplicity** A small example code snippet should be enough to get started and use most of features.
- **Performance** Ultra low latency for the caller threads, no string formatting on the fast-path, no heap allocations after initialisation, asynchronous only mode.
- **Convenience** While keeping low latency on the fast-path, the library aims to assist the developer in debugging the application by providing a nicely formatted direct textual output with all log statements ordered by timestamp

## Features
 * Clean warning-free codebase even on high warning levels
 * Safety. Extensive set of unit tests. Tested with Adress Sanitizer, Thread Sanitizer, Valgrind
 * Thread and Type safe with compile time checks
 * Optimised. Locality and cache friendly, minimal false sharing when threads are running on different cores
 * Python style formatting with build in support for logging STL containers, std::pair, std::tuple, std::chrono, user defined types and much more by using the excellent [{fmt}](https://github.com/fmtlib/fmt) library
 * Configurable
 * Custom log patterns. Log statements can be formatted by providing a simple pattern
 * Log levels can be stripped out at compile time in release builds
 * Log records are written in timestamp order even if they were created by different threads
 * Guaranteed logging. Log messages are never dropped. If in any case the internal queue gets full a new queue is created. Therefore, the caller will suffer aa very small performance penanalty instead of blocking.
 * Support for wide character logging and wide character filenames (Windows only)
 * Various log targets (Handlers)
   * Console logging 
   * File Logging
   * Rotating log files
   * Daily log files

## Performance

#### 1 Thread

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[Quill](https://github.com/odygrd/quill)   |  25  |  26  |  52  |  57  |  66  |  117  |  1600  |
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)   |  25  |  27  |  29  |  55  |  70  |  108  |  1702  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)   |  41  |  43  |  49  |  86  |  129  |  1156  |  2979  |
|[Reckless](https://github.com/mattiasflodin/reckless)          |  176  |  202  |  230  |  260  |  331  |  407  |  608480978  |
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)       |  167  |  172  |  182  |  1097  |  1230  |  2515  |  473816  |
|[spdlog](https://github.com/gabime/spdlog)                     |  1676  |  1753  |  1818  |  1883  |  2980  |  3665  |  12644  |
|[g3log](https://github.com/KjellKod/g3log)                     |  2798  |  2966  |  4270  |  4976  |  6359  |  8771  |  13862  |
#### 4 Threads

| Library            | 50th     | 75th     | 90th     | 95th     |  99th    | 99.9th   | Worst     |
|--------------------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:---------:|
|[Quill](https://github.com/odygrd/quill)   |  25  |  27  |  55  |  63  |  100  |  155  |  3828  |
|[PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)   |  25  |  27  |  38  |  55  |  93  |  167  |  2451  |
|[MS BinLog](https://github.com/Morgan-Stanley/binlog)   |  41  |  45  |  79  |  105  |  158  |  1183  |  4392  |
|[Reckless](https://github.com/mattiasflodin/reckless)   |  247  |  355  |  563  |  696  |  1021  |  1720  |  1281074  |
|[Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)   |  259  |  320  |  1129  |  1364  |  1806  |  3648  |  53129  |
|[spdlog](https://github.com/gabime/spdlog)                 |  1327  |  2012  |  2693  |  3538  |  6011  |  8932  |  17584  |
|[g3log](https://github.com/KjellKod/g3log)                 |  1685  |  2456  |  3747  |  5086  |  9896  |  16366  |  46392  |


The benchmarks are done on Linux (Ubuntu/RHEL) with GCC 9.1.  
The following message is logged 100'000 times per thread  
```LOG_INFO(logger, "Logging str: {}, int: {}, double: {}", str, i, d)```  
all reported latencies are in nanoseconds.  

Each thread is pinned on a different cpu. Note that running the backend logger thread in the same CPU as the caller threads, slows down the log messaage processing and will cause Quill's queue to fill faster performing a new allocation. Therefore, you will see bigger worst latencies.

Logging messages in a loop will make the consumer unable to follow up and the queue will have to re-allocate or block for most logging libraries expect very high throughput binary loggers like PlatformLab Nanolog. 
Therefore, a different approach was followed that suits more to a real time application - a log message per caller thread is logged between 1 to 3 microseconds.

I ran each logger benchmark three times and the above latencies are the second best result.

### Verdict
PlatformLab NanoLog is a very fast logger with very low latency and high throughput. However, this comes at the cost of having to decompress a binary file and the use of a non-type safe printf API where only primitive times can be passed. 
e.g. To log a custom type or a ```std::vector``` via NanoLog you would first have to convert it to a string in the caller thread suffering a performance loss. Instead, Quill copies the object and any string formatting is performed by the backend thread

Quill is not as high throughput as NanoLog but in terms of latency it is faster than NanoLog in almost every case. It is much more feature rich with custom formatting, several logger objects, human readable log files and a most importantly a superior format API with custom types support.

The benchmark code can be found [here](https://github.com/odygrd/logger_benchmarks).  
More benchmarks can be found [here](https://github.com/odygrd/logger_benchmarks/blob/master/results_thread_affinity_set.txt).

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

## Integration

### CMake

#### External

##### Building and Installing Quill as Static Library
```
git clone https://github.com/odygrd/quill.git
mkdir cmake_build
cd cmake_build
make install
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

### Package Managers

**TODO**

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

## Documentation
Advanced usage and additional documentation can be found in the [wiki](https://github.com/odygrd/quill/wiki) pages.

## License
Quill is licensed under the [MIT License](http://opensource.org/licenses/MIT)

Quill depends on third party libraries with separate copyright notices and license terms. 
Your use of the source code for these subcomponents is subject to the terms and conditions of the following licenses.

   * ([MIT License](http://opensource.org/licenses/MIT)) {fmt} (https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)
   * ([MIT License](http://opensource.org/licenses/MIT)) invoke.hpp (https://github.com/BlackMATov/invoke.hpp/blob/master/LICENSE.md)
