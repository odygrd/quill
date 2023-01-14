- [v2.6.1](#v2.6.1)
- [v2.6.0](#v2.6.0)
- [v2.6.0](#v2.6.0)
- [v2.5.1](#v2.5.1)
- [v2.5.0](#v2.5.0)
- [v2.4.2](#v2.4.2)
- [v2.4.1](#v2.4.1)
- [v2.4.0](#v2.4.0)
- [v2.3.4](#v2.3.4)
- [v2.3.3](#v2.3.3)
- [v2.3.2](#v2.3.2)
- [v2.3.1](#v2.3.1)
- [v2.3.0](#v2.3.0)
- [v2.2.0](#v2.2.0)
- [v2.1.0](#v2.1.0)
- [v2.0.2](#v2.0.2)
- [v2.0.1](#v2.0.1)
- [v2.0.0](#v2.0.0)
- [v1.7.3](#v1.7.3)
- [v1.7.2](#v1.7.2)
- [v1.7.1](#v1.7.1)
- [v1.7.0](#v1.7.0)
- [v1.6.3](#v1.6.3)
- [v1.6.2](#v1.6.2)
- [v1.6.1](#v1.6.1)
- [v1.6.0](#v1.6.0)
- [v1.5.2](#v1.5.2)
- [v1.5.1](#v1.5.1)
- [v1.5.0](#v1.5.0)
- [v1.4.1](#v1.4.1)
- [v1.4.0](#v1.4.0)
- [v1.3.3](#v1.3.3)
- [v1.3.2](#v1.3.2)
- [v1.3.1](#v1.3.1)
- [v1.3.0](#v1.3.0)
- [v1.2.3](#v1.2.3)
- [v1.2.2](#v1.2.2)
- [v1.2.1](#v1.2.1)
- [v1.2.0](#v1.2.0)
- [v1.1.0](#v1.1.0)
- [v1.0.0](#v1.0.0)

## v2.6.1

**Fixes**

- Remove references to build directory path from the compiled library's
  symbols ([#221](https://github.com/odygrd/quill/pull/221))
- Fix when compiled as shared library with hiden visibility ([#222](https://github.com/odygrd/quill/pull/222))
- Fix equal timestamp log messages appearing out of order ([#223](https://github.com/odygrd/quill/pull/223))

## v2.6.0

**Fixes**

- Fix filepath on Windows when MinGW is used. ([#212](https://github.com/odygrd/quill/pull/212))

**Improvements**

- Removed the creation of `static Metadata` objects during initialisation time.
- `#define QUILL_QUEUE_CAPACITY` has been removed.
- Added Config option `default_queue_capacity` that can be used to specify the initial capacity of the queue.
- When Unbounded queue is used the newly allocated queue will now have enough space to fit any
  object. ([#215](https://github.com/odygrd/quill/pull/215))

## v2.5.1

**Improvements**

- Reduced the allocations performed by the backend worker thread as the same objects are now being reused rather than
  destroyed.

**Summary of changes since v2.3.2**

In version `2.3.2` when multiple threads performed heavy logging, the backend logging thread incorrectly gave
priority to the logs of the same threads. That made logs from the remaining threads to appear much later or sometimes
never in the log files.

There was a series of fixes and releases to address this.

Below is the summary of the changes from `v2.3.2`

- Previously when multiple threads were logging, the backend logging thread would first try to read the log messages of
  the same thread until its queue was completely empty before reading the log messages of the next thread.
  When one of the threads was logging a lot, it could result in only displaying the log of that thread, hiding the
  logs of the other threads. This has now been fixed and all log messages from all threads are read fairly.

- Optimise the backend logging thread to read all log messages from each queue. Ensure all queues
  from all active threads are fairly read.

- `fmt::basic_memory_buffer` buffer stack size has been reduced. The backend thread shows better performance with
  a reduced stack size. This also reduces the risk of a stack overflow when too many log messages are cached.lllllllllll

- Reduced the allocations performed by the backend worker thread as the same objects are now being reused rather than
  destroyed.

- Added a config option `backend_thread_strict_log_timestamp_order`. This option enables an extra timestamp
  check on the backend logging thread when each message is popped from the queues. It prevents a rare
  situation where log messages from different threads could appear in the log file in the wrong order. This flag
  is now enabled by default.

- Added a config option `backend_thread_empty_all_queues_before_exit`. This option makes the backend logging thread
  to wait until all the queues are empty before exiting. This ensures no log messages are lost when the application
  exists. This flag is now enabled by default.
-

## v2.5.0

**Improvements**

- Performance improvements for the backend logging thread

## v2.4.2

**Fixes**

- Fixes an assertion that was triggered in debug mode due to changes in v2.4.1

## v2.4.1

**Improvements**

- Previously the backend worker thread would read all the log messages from the queue but not read the log messages when
  the buffer had wrapped around. It will now read all the messages.
- Removed the `min_available_bytes` cache from the SPSC queue as an optimisation. It is not needed anymore as we now
  read all messages at once instead of reading message by message.

## v2.4.0

**Improvements**

- Added a config option `backend_thread_strict_log_timestamp_order`. This option enables an extra timestamp
  check on the backend logging thread when each message is popped from the queues. It prevents a rare
  situation where log messages from different threads could appear in the log file in the wrong order. This flag
  is now enabled by default.

- Added a config option `backend_thread_empty_all_queues_before_exit`. This option makes the backend logging thread
  to wait until all the queues are empty before exiting. This ensures no log messages are lost when the application
  exists. This flag is now enabled by default.

## v2.3.4

**Improvements**

- Optimise the backend logging thread to read multiple log messages from the same queue, but still fairly read each
  queue from all active threads.

## v2.3.3

**Fixes**

- Previously when multiple threads were logging, Quill backend logging thread would first try reading the log messages of
  one thread until the queue was completely empty before reading the log messages of the next thread.
  When one of the threads was logging a lot, it could result in only displaying the log of that thread, hiding the
  logs of the other threads. This has now been fixed and all log messages from all threads are read fairly.

## v2.3.2

**Fixes**

- Fix code not compiling with treat warnings as errors set on
  Windows. ([#198](https://github.com/odygrd/quill/pull/198))

## v2.3.1

**Fixes**

- Optimise logging queue cache alignment of variables. It seems that v2.3.0 made the hot path slower by ~5 ns per message. This has been fixed in this version and the performance is now the same as in the previous versions.

## v2.3.0

**Improvements**

- Cache the available bytes for reading in the logging queue. This is meant to offer some minor performance
  improvement to the backend logging thread. [#185](https://github.com/odygrd/quill/issues/185)

- Fixed static code analysis and clang '-Wdocumentation' warnings.

- The `Handler.h` API has changed in this version to support structured logs. If you have implemented your own custom
  `Handler` you will have to change it to follow the new API.

- This version adds support for writing structured logs. Structured logs provide easier search through events.
  Structured logging is automatically enabled when named arguments are provided to the format string. Structured logs
  are only supported by the new `quill::JsonFileHandler` handler. The already existing `FileHandler` and
  `ConsoleHandler` are compatible with named arguments, but they will ignore them and output the log in its
  original format, as defined by the pattern formatter.
  Structured logs are not supported for wide characters at the moment.
  See [example_json_structured_log.cpp](https://github.com/odygrd/quill/blob/master/examples/example_json_structured_log.cpp)

For example :

```c++
  quill::start();

  quill::Handler* json_handler =
    quill::create_handler<quill::JsonFileHandler>("json_output.log", "w");

  // create another logger tha logs e.g. to stdout and to the json file at the same time
  quill::Logger* logger = quill::create_logger("dual_logger", {quill::stdout_handler(), json_handler});
  for (int i = 2; i < 4; ++i)
  {
    LOG_INFO(logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
  }
```

1) Will write to stdout (stdout_handler) :

````
23:37:19.850432433 [11811] example_json_structured_log.cpp:39 LOG_INFO      dual_logger  - POST to http:// took 20 ms
23:37:19.850440154 [11811] example_json_structured_log.cpp:39 LOG_INFO      dual_logger  - POST to http:// took 30 ms
````

2) Will produce a JSON file (json_handler) :

```
{ "timestamp": "23:37:19.850432433", "file": "example_json_structured_log.cpp", "line": "39", "thread_id": "11811", "logger": "dual_logger", "level": "Info", "message": "{method} to {endpoint} took {elapsed} ms", "method": "POST", "endpoint": "http://", "elapsed": "20" }
{ "timestamp": "23:37:19.850440154", "file": "example_json_structured_log.cpp", "line": "39", "thread_id": "11811", "logger": "dual_logger", "level": "Info", "message": "{method} to {endpoint} took {elapsed} ms", "method": "POST", "endpoint": "http://", "elapsed": "30" }
```

## v2.2.0

**Improvements**

- Previously storing the default root logger by calling `quill::get_logger()` followed by `quill::configure(cfg)`
  would invalidate the pointer to the default root logger returned by the former function. This has now been fixed and
  the obtained `Logger*` pointer is still valid.
- Disable `fmt::streamed()`. ([#189](https://github.com/odygrd/quill/issues/189))
- Update bundled fmt to 9.1.0
- `logger->should_log(level)` is removed. A compile time check was added to `logger->should_log<level>()`
  . ([#187](https://github.com/odygrd/quill/issues/187))

## v2.1.0

**Improvements**

This version includes breaking changes to the API. Those changes are related to how quill is configured,
before calling `quill::start()` to start the backend thread.

Check the updated [examples](https://github.com/odygrd/quill/blob/master/examples).

[Config.h](https://github.com/odygrd/quill/blob/master/quill/include/quill/Config.h) - contains runtime configuration options

[TweakMe.h](https://github.com/odygrd/quill/blob/master/quill/include/quill/TweakMe.h) - contains compile time configuration

For example `quill::set_default_logger_handler(...)` has been removed. To set a default filehandler :

```cpp
  // create a handler
  quill::Handler* file_handler = quill::file_handler("test.log", "w");

  file_handler->set_pattern(
    "%(ascii_time) [%(thread)] %(fileline:<28) %(level_name) %(logger_name:<12) - %(message)",
    "%Y-%m-%d %H:%M:%S.%Qms", quill::Timezone::GmtTime);

  // set the handler as the default handler for any newly created logger in the config
  quill::Config cfg;
  cfg.default_handlers.emplace_back(file_handler);

  // configure must always be called prior to `start()`
  quill::configure(cfg);
  quill::start();
```

- Removed some API functions from `Quill.h` that were previously used for configuration. Instead, `quill::Config` object
  has to be created. For example `quill::config::set_backend_thread_cpu_affinity(1);` has been removed and instead the following code is needed :

```cpp
  quill::Config cfg;
  cfg.backend_thread_cpu_affinity = 1;
  quill::configure(cfg);
```

- `QUILL_CHRONO_CLOCK` has been moved from `TweakMe.h` to `Config.h`. It is now possible to switch between `rdtsc` and `system`
  clocks without re-compiling.
  See [example_trivial_system_clock.cpp](https://github.com/odygrd/quill/blob/master/examples/example_trivial_system_clock.cpp)
- `QUILL_RDTSC_RESYNC_INTERVAL` has been moved from `TweakMe.h` to `Config.h`.
- It is now possible to log user timestamps rather than the system's. This feature is useful for time simulations.
  See [example_custom_clock.cpp](https://github.com/odygrd/quill/blob/master/examples/example_custom_clock.cpp)
  and [example_custom_clock_advanced.cpp](https://github.com/odygrd/quill/blob/master/examples/example_custom_clock_advanced.cpp)
- Previously the logger names were limited to a maximum of 22 characters. This limitation has been removed.
- Added support for gcc 7.5.0. ([#178](https://github.com/odygrd/quill/issues/178))
- Updated bundled fmt to 9.0.0

## v2.0.2

**Fixes**

- Fix crash when a `std::string` containing null-terminated characters is passed to the
  logger. ([#176](https://github.com/odygrd/quill/issues/176))

## v2.0.1

**Improvements**

- Add a flag to RotatingFileHandler to disable removing the old files when `w` mode is used.

## v2.0.0

From version `v2` and onwards only c++17 is supported.

This version is a major refactor.

**Fixes**

- RotatingFileHandler will now correctly rotate the files when append mode is
  used ([#123](https://github.com/odygrd/quill/issues/123))

**Improvements**

- Reduced and simplified codebase.
- Improved backend worker thread performance.
- `QUILL_DUAL_QUEUE_MODE` has been removed. A single queue now handles every case.
- `QUILL_STRING` has been removed. That macro is no longer required when passing a format string to the
  PatternFormatter.

**Differences**

- `v1.7` compiles with c++14, `v2` only compiles for c++17.
- `v1.7` on Windows supports wide character logging, `v2` has limited wide character support such as logging `wchar_t`
  , `std::wstring`, `std::wstring_view`. For example, logging `std::vector<std::wstring>` is not supported.
- `v1.7` on Windows requires the filepath used for the handlers as a wide strings, `v2` supports only filenames as
  narrow strings.

## v1.7.3

**Improvements/Fixes**

- Fix crash on windows when a long wstring (>500 chars) is logged ([#173](https://github.com/odygrd/quill/issues/173))
- Fix compiler error when trying to compile with
  -DQUILL_DISABLE_NON_PREFIXED_MACROS ([#174](https://github.com/odygrd/quill/issues/174))
- Fix a compile warning in clang ([#175](https://github.com/odygrd/quill/issues/175))

## v1.7.2

**Improvements/Fixes**

- Fix compile error when C++20 is used on windows ([#162](https://github.com/odygrd/quill/issues/162))

## v1.7.1

**Improvements/Fixes**

- Fix support for wide characters on Windows ([#168](https://github.com/odygrd/quill/issues/168))
- Fix compilation error when `Quill::Logger*` is stored as a class member in templated classes
- Add `FilenameAppend::DateTime` as an option when creating a file handler

## v1.7.0

**New Features**

- Add a new function `quill::get_all_loggers()` that returns all the existing
  loggers. ([#114](https://github.com/odygrd/quill/issues/114))
- Add `%(level_id)` to pattern formatter. ([#136](https://github.com/odygrd/quill/issues/136))
- Users can now specialise `copy_loggable<T>` to mark user defined types as safe to
  copy. ([#132](https://github.com/odygrd/quill/issues/132))

**Improvements/Fixes**

- Fix initializations for C++17.
- Fix compiler warning in `check_format()` function.
- Replace `QUILL_DUAL_QUEUE_MODE` with `QUILL_DISABLE_DUAL_QUEUE_MODE`.
- Update bundled fmt to 8.1.1
- Minor performance and accuracy improvements to rdtsc clock used by the backend thread.
- Fix compile error when C++20 is used. ([#162](https://github.com/odygrd/quill/issues/162))
- Fix `get_page_size()` to only call sysconf once. ([#160](https://github.com/odygrd/quill/issues/160))
- Fix incorrect timestamps in the log file when the system clock is
  updated. ([#127](https://github.com/odygrd/quill/issues/127))
- Previously if `quill:start(true)` was called more than once in the application, the signal handlers would get
  initialised again. Now any subsequent calls to `quill:start(true)` will now have no effect
  ([#167](https://github.com/odygrd/quill/issues/167))
- Previously when the max limit of rotated files in `RotatingFileHandler` was reached, quill would stop rotating and
  instead keep logging everything into the last log file. Now when the maximum limit of files is reached,
  quill will now keep rotating by replacing the oldest logs. ([#157](https://github.com/odygrd/quill/issues/157))
- Improve the backend logging thread responsiveness when variables are logged in loops without any delay
  from multiple threads. ([#116](https://github.com/odygrd/quill/issues/116))
- Fix some undefined behaviour issues reported via the AddressSantizer on the backend logging
  thread. ([#166](https://github.com/odygrd/quill/issues/166))

## v1.6.3

**Improvements/Fixes**

- Add support for `%(thread_name)` in PatternFormatter. ([#97](https://github.com/odygrd/quill/issues/97))
- Add missing header needed for recent versions of fmt. ([#95](https://github.com/odygrd/quill/issues/95))
- Force flush all active handlers on application exit.
- Update bundled fmt to 8.0.1

## v1.6.2

**Fixes**

- Fix WIN32 compilation error when `NOMINMAX` is already defined.
- Fix `string` to `wstring` MinGW conversion. ([#92](https://github.com/odygrd/quill/issues/92))
- Log enums via the main queue. ([#90](https://github.com/odygrd/quill/issues/90))
- Fix windows compiler error when `min/max` macros are defined. ([#94](https://github.com/odygrd/quill/issues/94))

## v1.6.1

**Improvements/Fixes**

- Fix windows C++20 build. ([#83](https://github.com/odygrd/quill/issues/83))
- Fix ARM build on windows.
- Fix `example_backtrace` and minor bug when destructing with empty backtrace.

## v1.6.0

**New Features**

- Dual queue mode offering even lower latencies on hot paths.
  See [Dual Queue Mode](https://github.com/odygrd/quill/wiki/9.-Dual-Queue-Mode).
- Added a signal handler for linux and windows. The signal handler flushes the log when the app crashes or
  exits. ([#1](https://github.com/odygrd/quill/issues/1))
- Added support for custom handlers. ([#75](https://github.com/odygrd/quill/issues/75))
- Quill now compiles and runs on Cygwin.

**Improvements/Fixes**

- The queue from the caller to the backend worker thread has been reworked. The new queue generates slightly better
  assembly than the previous one. Quill does no longer depend on mapping the same region of physical memory twice.
- Replaced an assertion check that could trigger incorrectly. ([#68](https://github.com/odygrd/quill/issues/68))
- Fixed build on `ARM_ARCH < 6`. ([#78](https://github.com/odygrd/quill/issues/78))
- Fixed compile errors when `QUILL_NOEXCEPTIONS`, `CMAKE_CXX_STANDARD 20`, `QUILL_USE_BOUNDED_QUEUE` are set.
- The unit tests have been moved to a separate binary with their own `main()`. This increased build times when building
  the tests, but the failures are now easier to debug on different CI platforms and the tests can also run faster in
  parallel.
- Fixed minor compiler warnings on windows.
- Upgraded bundled libfmt to `7.1.3`

**Note**

- If a custom queue capacity is defined using `#define QUILL_QUEUE_CAPACITY` after `1.6.0` the whole library needs to be
  recompiled.

## v1.5.2

- Removed the use of `fmt::format()` in `FileUtilities.cpp` as a workaround to the link errors in fmt v7. Use the header
  only version of libfmt when external libfmt is defiend is no longer required.

## v1.5.1

- When QUILL_FMT_EXTERNAL is defined, `quill` will use the header only version of `libfmt`. This is a workaround to the
  link errors after libftm v7

## v1.5.0

- Upgraded bundled libfmt to `7.1.2`
- Added `Filters`. The filter class can be used for filtering log records. Filters can be added to handler instances.
  See [example_filters.cpp](https://github.com/odygrd/quill/blob/master/examples/example_filters.cpp)
- It is now possible to set the log level severity on the handler objects.
  See [example_filters.cpp](https://github.com/odygrd/quill/blob/master/examples/example_handler_log_levels.cpp) ([#49](https://github.com/odygrd/quill/issues/49))
- Timestamp formatting optimisation for the backend worker thread.
- Free list allocator optimisation for the backend worker thread.
- Fixed PatternFormatter ignoring a portion of the pattern was ignored, when no format specifiers were
  present. ([#56](https://github.com/odygrd/quill/issues/56))
- When `%(function_name)` is used in PatternFormatter the namespace delimiter is replaced from `::` to `.` (Windows
  only). ([#61](https://github.com/odygrd/quill/issues/61))
- Arguments passed to the logger are no longer being evaluated when the log statement is not
  logged. ([#67](https://github.com/odygrd/quill/issues/67))
- PatternFormatter enhancement. It is now possible to pass [{fmt} string syntax](https://fmt.dev/latest/syntax.html)
  to `QUILL_STRING`. The default PatternFormatter string has been changed
  to: `"%(ascii_time) [%(thread)] %(fileline:<28) LOG_%(level_name) %(logger_name:<12) - %(message)"`. This results to
  the following log being properly aligned despite the different lengths of each filename and logger name.

```
22:31:07.995438465 [2666041] file1.h:11                   LOG_INFO      logger1      - Log from file.
22:31:07.995445699 [2666041] long_file2.h:11              LOG_INFO      logger_fl2   - Log from other file.
22:31:07.995457144 [2666041] a_longer_file_3.hpp:11       LOG_INFO      logger_fl2_l - Log from other file.
22:31:07.995462471 [2666041] example_trivial.cpp:30       LOG_TRACE_L3  root         - This is a log trace l3 example 1
```

## v1.4.1

- Do not force `quill` to always build as `static` library in cmake.
- Minor fix when `quill` is compiled with no exceptions.
- Add the option to disable the non prefixed macro definitions if `QUILL_DISABLE_NON_PREFIXED_MACROS` is
  defined. ([#40](https://github.com/odygrd/quill/issues/40))

## v1.4.0

- Added support for printing colour codes in the terminal.
  See [ConsoleHandler](https://github.com/odygrd/quill/wiki/2.-Handlers#consolehandler)
- RotatingFileHandler improvements and minor change in API.
  See [RotatingFileHandler](https://github.com/odygrd/quill/wiki/2.-Handlers#rotatingfilehandler)
- DailyFileHandler is removed and replaced by TimeRotatingFileHandler.
  See [TimeRotatingFileHandler](https://github.com/odygrd/quill/wiki/2.-Handlers#timerotatingfilehandler)
- Added backtrace logging. Log messages can be stored in a buffer and flushed later on demand.
  See [Backtrace Logging](https://github.com/odygrd/quill/wiki/6.-Backtrace-Logging)
- Added bundled `doctest` `2.4.0`
- Migrated all tests from `gtest` to `doctest`.
- Upgraded bundled libfmt to `7.0.3`

## v1.3.3

- Upgraded bundled libfmt to `7.0.2`
- Fixed compile error with libfmt versions > `7.0.0`

## v1.3.2

- Add a CMake option `QUILL_USE_BOUNDED_QUEUE` for bounded queue.
- Fixed a clang 10 warning
- Fixed MinGw build

## v1.3.1

- Minor CMake fixes when `QUILL_FMT_EXTERNAL` option is used.

## v1.3.0

**New Features**

- Added option `QUILL_NO_EXCEPTIONS` to disable exceptions, std::abort() is called instead of an
  exception. ([#16](https://github.com/odygrd/quill/issues/16))
- Exceptions thrown in the backend worker thread, will now call a user provided error handler callback to handle the
  error. ([#21](https://github.com/odygrd/quill/issues/21))
- Compile time checks for unsafe to copy user defined types. Non trivial user defined types must be explicitly tagged as
  safe to copy with the use of `QUILL_COPY_LOGGABLE;`. Otherwise they have to be formatted and passed as a string to the
  logger by the user. The old unsafe mode is still usable
  by `#define QUILL_MODE_UNSAFE` ([#20](https://github.com/odygrd/quill/issues/20))
- Added `QUILL_USE_BOUNDED_QUEUE`. In this mode no new queues get allocated but instead log messages get lost. Number of
  lost messages is reported to stderr.
- Minor hot path optimisation. The pointer to the metadata for each log message is no logger copied to the queue but
  passed as a template argument instead.
- Added a latency benchmark, easily extendable for any logger

**Improvements/Fixes**

- `QUILL_RDTSC_CLOCK` option is replaced by `QUILL_CHRONO_CLOCK` which is by OFF by default.
- Improve compiler error message when trying to log a non copy constructible user defined type
- Fix buffer reallocation bug on TimestampFormatter. In previous versions any timestamp format set to 'set_pattern'
  expanding to a string longer than 32 bytes would cause a crash. ([#24](https://github.com/odygrd/quill/issues/24))
- The backend logging thread will now copy all messages from the SPSC queue to a local priority queue. This keeps the
  SPSC less empty avoiding a potential allocation on the hot path.
- `std::string_view` is now promoted to `std::string` to take a deep copy
- The queue capacity has been moved from `config` to `Tweakme.h`.
- Multiple formats patterns support for `stdout` and `stderr` handlers.
  See [example_stdout_multiple_formatters.cpp](https://github.com/odygrd/quill/blob/master/examples/example_custom_formatter.cpp)
- `quill::start()` will now block until the backend worker has started.
- Upgraded bundled libfmt to `6.2.1`

## v1.2.3

- CMake changes to support package installation in conan.

## v1.2.2

- Support for `arm/arm64`. ([#19](https://github.com/odygrd/quill/issues/19))
- Add a cmake option `QUILL_ENABLE_INSTALL` to enable cpack.

## v1.2.1

- Improved `QUILL_RDTSC_CLOCK` tweak option. It is now possible to switch between using `rdtsc` or `std::chrono` clock
  without having to recompile quill as library.

## v1.2.0

- Linking and including an external version of `fmt` is now supported. See `TweakMe.h`
- Fixed compiler warnings when using clang's `-Wdocumentation`. ([#12](https://github.com/odygrd/quill/issues/12))
- Fixed a bug that wouldn't report a compile-time error for invalid format
  strings. ([#13](https://github.com/odygrd/quill/issues/13))
- Added process ID to Formatter. ([#14](https://github.com/odygrd/quill/issues/14))
- Enhanced timestamp formatting. The `timestamp_format` string passed
  in `handler->set_pattern(format_pattern, timestamp_format, timezone)` now accepts three additional specifiers `%Qms`
  , `%Qus`, `%Qus` that can be used to format the fractional seconds.
  See [here](https://github.com/odygrd/quill/wiki/3.-Formatters). ([#15](https://github.com/odygrd/quill/issues/15))

## v1.1.0

- Daily file handler. The file handler rollover every 24 hours
- Rotating file handler. The file handler will rollover based on the size of the file
- MinGW compatibility
- Added a CMake option `QUILL_VERBOSE_MAKEFILE`. Building Quill as a master project now defaults to non verbose makefile
  output unless `-DQUILL_VERBOSE_MAKEFILE=ON` is passed to CMake. ([#6](https://github.com/odygrd/quill/issues/6))
- Flush policy improvement. Previously Quill backend worker thread would never `flush`. This made watching the live log
  of the application harder because the user has to wait for the operating system to flush or `quill::flush()` had to be
  called on the caller threads. This has now been fixed, when the backend thread worker has no more log messages to
  process it will automatically `flush`. ([#8](https://github.com/odygrd/quill/issues/8))
- The log level names have been changed from `"LOG_INFO"`, `"LOG_DEBUG"`, etc to `"INFO"`, `"DEBUG"`, etc .. The default
  formatter string is now using `"LOG_"%(level_name)` instead of `%(level_name)` therefore there is now change in the
  behaviour. This change gives a lot of more flexibility to users who prefer to see e.g. `INFO` instead of `LOG_INFO` in
  the logs. ([#7](https://github.com/odygrd/quill/issues/7))
- An option has been added to append the date to the filename when using a
  FileHandler `quill::file_handler(filename, mode, FilenameAppend);`. ([#7](https://github.com/odygrd/quill/issues/7))
- It is now possible to specify the timezone of each handler timestamp. A new parameter is added
  to `file_handler->set_pattern(...)`. See `PatternFormatter::Timezone`
  . ([#7](https://github.com/odygrd/quill/issues/7))
- Rename `emit` as it can conflict with Qt macros. ([#4](https://github.com/odygrd/quill/issues/4))
- Upgraded `libfmt` to `6.2.0`.

## v1.0.0

- Initial release.
- Using `libfmt` to `6.1.2`.

