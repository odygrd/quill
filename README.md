# Quill

#### Asynchronous Low Latency Logging Library

[![language][badge.language]][language]
[![license][badge.license]][license]

[badge.language]: https://img.shields.io/badge/language-C%2B%2B14-red.svg
[badge.license]: https://img.shields.io/badge/license-MIT-blue.svg

[language]: https://en.wikipedia.org/wiki/C%2B%2B14
[license]: http://opensource.org/licenses/MIT

- [Features](#features)
- [Supported Compilers](#supported-compilers)
- [Design Goals](#design-goals)
- [Integration](#integration)
  - [CMake](#cmake)
  - [Package Managers](#package-managers)
- [Examples](#examples)
  - [Basic Usage](#basic-usage)
- [Documentation](#documentation)
- [License](#license)

 ## Features
 * Python style formatting by the excellent [{fmt}](https://github.com/fmtlib/fmt) library
 * Custom LogRecord formatting, with attributes similar to python logging (https://docs.python.org/3/library/logging.html)
 * Various log targets (Handlers)
    * Console logging 
    * Rotating log files [Work in progress]
    * Daily log files [Work in progress]
 * Multiple thread-safe Loggers

## Supported Compilers 
(TODO: Update compiler list)
- GCC 4.8 - 9.2 (and possibly later)
- Clang 3.4 - 9.0 (and possibly later)
- Microsoft Visual C++ 2015 / Build Tools 14.0.25123.0 (and possibly later)
- Microsoft Visual C++ 2017 / Build Tools 15.5.180.51428 (and possibly later)
- Microsoft Visual C++ 2019 / Build Tools 16.3.1+1def00d3d (and possibly later)

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

## Documentation

## License
Quill is licensed under the [MIT License](http://opensource.org/licenses/MIT)

Quill depends on third party libraries with separate copyright notices and license terms. 
Your use of the source code for these subcomponents is subject to the terms and conditions of the following licenses.

   * ([MIT License](http://opensource.org/licenses/MIT)) {fmt} (https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)
   * ([MIT License](http://opensource.org/licenses/MIT)) invoke.hpp (https://github.com/BlackMATov/invoke.hpp/blob/master/LICENSE.md)

