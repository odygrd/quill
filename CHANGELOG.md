-  [v1.4.0](#v1.4.0)
-  [v1.3.3](#v1.3.3)
-  [v1.3.2](#v1.3.2)
-  [v1.3.1](#v1.3.1)
-  [v1.3.0](#v1.3.0)
-  [v1.2.3](#v1.2.3)
-  [v1.2.2](#v1.2.2)
-  [v1.2.1](#v1.2.1)
-  [v1.2.0](#v1.2.0)
-  [v1.1.0](#v1.1.0)
-  [v1.0.0](#v1.0.0)

## v1.4.0
- RotatingFileHandler improvements and minor change in API. See [RotatingFileHandler](https://github.com/odygrd/quill/wiki/2.-Handlers#rotatingfilehandler)
- DailyFileHandler is removed and replaced by TimeRotatingFileHandler. See [TimeRotatingFileHandler](https://github.com/odygrd/quill/wiki/2.-Handlers#timerotatingfilehandler)
- Added backtrace logging. Log messages can be stored in a buffer and flushed later on demand. See [Backtrace Logging](https://github.com/odygrd/quill/wiki/6.-Backtrace-Logging)
- Added bundled `doctest` `2.4.0`
- Migrated all tests from `gtest` to `doctest`.

## v1.3.3
- Upgraded bundled libfmt to `7.0.2`
- Fixed compile error with libfmt versions > `7.0.0`

## v1.3.2
-  Add a CMake option `QUILL_USE_BOUNDED_QUEUE` for bounded queue.
-  Fixed a clang 10 warning
-  Fixed MinGw build

## v1.3.1
-  Minor CMake fixes when `QUILL_FMT_EXTERNAL` option is used.

## v1.3.0
**New Features**
-  Added option `QUILL_NO_EXCEPTIONS` to disable exceptions, std::abort() is called instead of an expection. ([#16](https://github.com/odygrd/quill/issues/16))
-  Exceptions thrown in the backend worker thread, will now call a user provided error handler callback to handle the error. ([#21](https://github.com/odygrd/quill/issues/21))
-  Compile time checks for unsafe to copy user defined types. Non trivial user defined types must be explicitly tagged as safe to copy with the use of `QUILL_COPY_LOGGABLE;`. Otherwise they have to be formatted and passed as a string to the logger by the user. The old unsafe mode is still usable by `#define QUILL_MODE_UNSAFE` ([#20](https://github.com/odygrd/quill/issues/20))
-  Added `QUILL_USE_BOUNDED_QUEUE`. In this mode no new queues get allocated but instead log messages get lost. Number of lost messages is reported to stderr.
-  Minor hot path optimisation. The pointer to the metadata for each log message is no logger copied to the queue but passed as a template argument instead.
-  Added a latency benchmark, easily extendable for any logger

**Improvements/Fixes**
-  `QUILL_RDTSC_CLOCK` option is replaced by `QUILL_CHRONO_CLOCK` which is by OFF by default.
-  Improve compiler error message when trying to log a non copy constructible user defined type
-  Fix buffer reallocation bug on TimestampFormatter. In previous versions any timestamp format set to 'set_pattern' expanding to a string longer than 32 bytes would cause a crash. ([#24](https://github.com/odygrd/quill/issues/24))
-  The backend logging thread will now copy all messages from the SPSC queue to a local priority queue. This keeps the SPSC less empty avoiding a potential allocation on the hot path.
-  `std::string_view` is now promoted to `std::string` to take a deep copy
-  The queue capacity has been moved from `config` to `Tweakme.h`.
-  Multiple formats patterns support for `stdout` and `stderr` handlers. See [example_stdout_multiple_formatters.cpp](https://github.com/odygrd/quill/blob/master/examples/example_custom_formatter.cpp)
-  `quill::start()` will now block until the backend worker has started.
-  Upgraded bundled libfmt to `6.2.1`

## v1.2.3
-  CMake changes to support package installation in conan.

## v1.2.2
-  Support for `arm/arm64`. ([#19](https://github.com/odygrd/quill/issues/19))
-  Add a cmake option `QUILL_ENABLE_INSTALL` to enable cpack.

## v1.2.1
-  Improved `QUILL_RDTSC_CLOCK` tweak option. It is now possible to switch between using `rdtsc` or `std::chrono` clock without having to recompile quill as library.

## v1.2.0
-  Linking and including an external version of `fmt` is now supported. See `TweakMe.h`
-  Fixed compiler warnings when using clang's `-Wdocumentation`. ([#12](https://github.com/odygrd/quill/issues/12))
-  Fixed a bug that wouldn't report a compile-time error for invalid format strings. ([#13](https://github.com/odygrd/quill/issues/13))
-  Added process ID to Formatter. ([#14](https://github.com/odygrd/quill/issues/14))
-  Enhanced timestamp formatting. The `timestamp_format` string passed in `handler->set_pattern(format_pattern, timestamp_format, timezone)` now accepts three additional specifiers `%Qms`, `%Qus`, `%Qus` that can be used to format the fractional seconds. See [here](https://github.com/odygrd/quill/wiki/3.-Formatters). ([#15](https://github.com/odygrd/quill/issues/15))

## v1.1.0
-  Daily file handler. The file handler rollover every 24 hours
-  Rotating file handler. The file handler will rollover based on the size of the file
-  MinGW compatibility
-  Added a CMake option `QUILL_VERBOSE_MAKEFILE`. Building Quill as a master project now defaults to non verbose makefile output unless `-DQUILL_VERBOSE_MAKEFILE=ON` is passed to CMake. ([#6](https://github.com/odygrd/quill/issues/6))
-  Flush policy improvement. Previously Quill backend worker thread would never `flush`. This made watching the live log of the application harder because the user has to wait for the operating system to flush or `quill::flush()` had to be called on the caller threads. This has now been fixed, when the backend thread worker has no more log messages to process it will automatically `flush`. ([#8](https://github.com/odygrd/quill/issues/8))
-  The log level names have been changed from `"LOG_INFO"`, `"LOG_DEBUG"`, etc to `"INFO"`, `"DEBUG"`, etc .. The default formatter string is now using `"LOG_"%(level_name)` instead of `%(level_name)` therefore there is now change in the behaviour. This change gives a lot of more flexibility to users who prefer to see e.g. `INFO` instead of `LOG_INFO` in the logs. ([#7](https://github.com/odygrd/quill/issues/7))
-  An option has been added to append the date to the filename when using a FileHandler `quill::file_handler(filename, mode, FilenameAppend);`. ([#7](https://github.com/odygrd/quill/issues/7))
-  It is now possible to specify the timezone of each handler timestamp. A new parameter is added to `file_handler->set_pattern(...)`. See `PatternFormatter::Timezone`. ([#7](https://github.com/odygrd/quill/issues/7))
-  Rename `emit` as it can confict with Qt macros. ([#4](https://github.com/odygrd/quill/issues/4))
-  Upgraded `libfmt` to `6.2.0`.

## v1.0.0
-  Initial release.
-  Using `libfmt` to `6.1.2`.

