<h2> <img src="https://i.postimg.cc/FssWB25k/quill-logo.png" alt="Quill logo" width="140"><br>Asynchronous Low Latency Logging Library</h2>

[![language][badge.language]][language]
[![license][badge.license]][license]

[badge.language]: https://img.shields.io/badge/language-C%2B%2B14-red.svg
[badge.license]: https://img.shields.io/badge/license-MIT-blue.svg

[language]: https://en.wikipedia.org/wiki/C%2B%2B14
[license]: http://opensource.org/licenses/MIT

- [Supported Compilers](#supported-compilers)
- [Features](#features)
- [Design Goals](#design-goals)
- [Integration](#integration)
  - [CMake](#cmake)
  - [Package Managers](#package-managers)
- [Examples](#examples)
  - [Basic Usage](#basic-usage)
- [Documentation](#documentation)
- [License](#license)

## Supported Compilers 
(TODO: Update compiler list)
- GCC 4.8 - 9.2 (and possibly later)
- Clang 3.4 - 9.0 (and possibly later)
- Microsoft Visual C++ 2015 / Build Tools 14.0.25123.0 (and possibly later)
- Microsoft Visual C++ 2017 / Build Tools 15.5.180.51428 (and possibly later)
- Microsoft Visual C++ 2019 / Build Tools 16.3.1+1def00d3d (and possibly later)

## Features
 * Python style formatting by the excellent [{fmt}](https://github.com/fmtlib/fmt) library
 * Custom LogRecord formatting, with attributes similar to python logging (https://docs.python.org/3/library/logging.html)
 * Various log targets (Handlers)
    * Console logging 
    * Rotating log files [Work in progress]
    * Daily log files [Work in progress]
 * Multiple thread-safe Loggers

## Design Goals
There are many C++ logging libraries out there. Quill had these design goals:

- **Low latency not high throughput**. The main priority is set on reducing the latency on caller threads as much as possible. There is only one backend consumer thread responsible for writing the log file and how quickly we right to the file comes as a second priority.

## Integration

### CMake

#### External

To use this library from a CMake project, you can locate it directly with `find_package()` and use the namespaced imported target from the generated package configuration:

```cmake
# CMakeLists.txt
find_package(quill 3.2.0 REQUIRED)
...
add_library(foo ...)
...
target_link_libraries(foo PRIVATE quill::quill)
```

#### Embedded

To embed the library directly into an existing CMake project, place the entire source tree in a subdirectory and call `add_subdirectory()` in your `CMakeLists.txt` file:

```cmake
add_subdirectory(quill)
...
add_library(foo ...)
...
target_link_libraries(foo PRIVATE quill::quill)
```

### Package Managers

## Examples

### Basic usage

```c++
#include "quill/Quill.h"

int main()
{
  // Start the logging backend thread
  quill::start();
  
  // Get a pointer to the default logger
  quill::Logger* default_logger = quill::get_logger();

  LOG_INFO(default_logger, "Welcome to Quill!");
  LOG_ERROR(default_logger, "An error message with error code {}, error message {}", 123, "system_error");

  LOG_WARNING(default_logger, "Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
  LOG_CRITICAL(default_logger, "Easy padding in numbers like {:08d}", 12);

  LOG_DEBUG(default_logger, "This message and any message below this log level will not be displayed..");

  // Enable additional log levels on this logger
  default_logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_DEBUG(default_logger, "The answer is {}", 1337);
  LOG_TRACE_L1(default_logger, "{:>30}", "right aligned");
  LOG_TRACE_L2(default_logger, "Positional arguments are {1} {0} ", "too", "supported");
  LOG_TRACE_L3(default_logger, "Support for floats {:03.2f}", 1.23456);
}
```

#### Output
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

## License
Quill is licensed under the [MIT License](http://opensource.org/licenses/MIT)

Quill depends on third party libraries with separate copyright notices and license terms. 
Your use of the source code for these subcomponents is subject to the terms and conditions of the following licenses.

   * ([MIT License](http://opensource.org/licenses/MIT)) {fmt} (https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)
   * ([MIT License](http://opensource.org/licenses/MIT)) invoke.hpp (https://github.com/BlackMATov/invoke.hpp/blob/master/LICENSE.md)

