- [v3.3.2](#v332)
- [v3.3.1](#v331)
- [v3.3.0](#v330)
- [v3.2.0](#v320)
- [v3.1.0](#v310)
- [v3.0.2](#v302)
- [v3.0.1](#v301)
- [v3.0.0](#v300)
- [v2.9.2](#v292)
- [v2.9.1](#v291)
- [v2.9.0](#v290)
- [v2.8.0](#v280)
- [v2.7.0](#v270)
- [v2.6.0](#v260)
- [v2.5.1](#v251)
- [v2.5.0](#v250)
- [v2.4.2](#v242)
- [v2.4.1](#v241)
- [v2.4.0](#v240)
- [v2.3.4](#v234)
- [v2.3.3](#v233)
- [v2.3.2](#v232)
- [v2.3.1](#v231)
- [v2.3.0](#v230)
- [v2.2.0](#v220)
- [v2.1.0](#v210)
- [v2.0.2](#v202)
- [v2.0.1](#v201)
- [v2.0.0](#v200)
- [v1.7.3](#v173)
- [v1.7.2](#v172)
- [v1.7.1](#v171)
- [v1.7.0](#v170)
- [v1.6.3](#v163)
- [v1.6.2](#v162)
- [v1.6.1](#v161)
- [v1.6.0](#v160)
- [v1.5.2](#v152)
- [v1.5.1](#v151)
- [v1.5.0](#v150)
- [v1.4.1](#v141)
- [v1.4.0](#v140)
- [v1.3.3](#v133)
- [v1.3.2](#v132)
- [v1.3.1](#v131)
- [v1.3.0](#v130)
- [v1.2.3](#v123)
- [v1.2.2](#v122)
- [v1.2.1](#v121)
- [v1.2.0](#v120)
- [v1.1.0](#v110)
- [v1.0.0](#v100)

## v3.3.2

- Resolved "bad_variant_access" error occurring when using Quill as a pre-compiled library with a distinct queue
  type. ([#276](https://github.com/odygrd/quill/pull/276))

- Resolved a bug in `RotatingFileHandler` associated with logfiles located outside the working directory,
  specifically when used with open_mode `a`. ([#340](https://github.com/odygrd/quill/pull/340))

## v3.3.1

- Fixed `RotatingFileHandler` to prevent accidental removal of non-log files when using open mode `w`
  and `set_remove_old_files(true)`

## v3.3.0

- Added a `quill::get_handler(handler_name)` function that allows easy lookup of an existing `Handler` by name. This
  function proves helpful when you want to retrieve a handler and pass it to a new logger.

- Fix build failure of Intel Compiler Classic. ([#332](https://github.com/odygrd/quill/pull/332))

- Introduced `QUILL_BLOCKING_QUEUE_RETRY_INTERVAL_NS` option for user-configurable retry interval in the blocking queue.
  Default value is 800 nanoseconds. ([#330](https://github.com/odygrd/quill/pull/330))

- Improved backend thread handling. Now verifies that all producer SPSC queues are empty before entering `sleep`.

- Fixed a race condition and potential crash in `quill::remove_logger(Logger*)` when called without prior `quill::flush()`.

- Added protection to prevent removal of the root logger with `quill::remove_logger(Logger*)`.

- Improved exception handling on the backend thread when calling `fmt::format()`.

  While compile-time checks ensure that the format string and arguments match, runtime errors can still occur.
  Previously, such exceptions would affect and drop subsequent log records. Now, exceptions are caught and logged
  in the log file and reported via the backend thread notification handler (default is `cerr`).

  For example, if a dynamic precision is used (`LOG_INFO(logger, "Support for floats {:.{}f}", 1.23456, 3.321312)`),
  the log file will show the following error message:

  ```
  LOG_INFO root [format: "Support for floats {:.{}f}", error: "precision is not integer"]
  ```

  Additionally, an error message will be printed to `cerr`

  ```
  Quill ERROR: [format: "Support for floats {:.{}f}", error: "precision is not integer"]
  ```

- Fixed a bug in timestamp formatting that occasionally displayed an hour component of 0 as
  24. ([#329](https://github.com/odygrd/quill/pull/329))

- Added support for specifying a runtime log level, allowing dynamic log level configuration at runtime.
  The new runtime log level feature provides flexibility when needed, with a minor overhead cost.
  It is recommended to continue using the existing static log level macros for optimal
  performance. ([#321](https://github.com/odygrd/quill/pull/321))

  For example

  ```c++
    std::array<quill::LogLevel, 4> const runtime_log_levels = {quill::LogLevel::Debug,
                                                               quill::LogLevel::Info,
                                                               quill::LogLevel::Warning,
                                                               quill::LogLevel::Error};
  
    for (auto const& log_level : runtime_log_levels)
    {
      LOG_DYNAMIC(logger, log_level, "Runtime {} {}", "log", "level");
    }
  ```

- Added support for printf-style formatting with `_CFORMAT` macros. These macros use the `printf` format string syntax,
  simplifying the migration of legacy codebases using `printf` statements.

  For example

  ```c++
    std::array<uint32_t, 4> arr = {1, 2, 3, 4};
    LOG_INFO(logger, "This is a log info example using fmt format {}", arr);
    
    LOG_INFO_CFORMAT(logger, "printf style %s supported %d %f", "also", 5, 2.32);
  ```

- Added a `metadata()` member function to the `TransitEvent` class. It provides access to the `Metadata` object
  associated with the log record, simplifying syntax for retrieving log record metadata in custom Handlers.

  For example

  ```c++
  void CustomHandler::write(fmt_buffer_t const& formatted_log_message, quill::TransitEvent const& log_event)
  {
    MacroMetadata const macro_metadata = log_event.metadata();
  }
  ```

- Simplified file handler configuration. Now, instead of passing multiple arguments to the constructor,
  you only need to provide a single `FileHandlerConfig` object. This change makes creating file handlers objects
  much easier and more flexible.

  For example

  ```c++
  quill::FileHandlerConfig file_handler_cfg;
  file_handler_cfg.set_open_mode('w');
  file_handler_cfg.set_append_to_filename(quill::FilenameAppend::StartDateTime);
  
  std::shared_ptr<quill::Handler> file_handler = quill::file_handler("application.log", file_handler_cfg);
  quill::Logger* logger_foo = quill::create_logger("my_logger", std::move(file_handler));
  
  LOG_INFO(my_logger, "Hello from {}", "application");
  ```

- Combined the functionalities of `RotatingFileHandler` (rotating based on file size) and `TimeRotatingFileHandler`
  (rotating on a time interval) into a single, more versatile `RotatingFileHandler`. Users can now conveniently rotate
  logs based on both file size and time intervals simultaneously. The updated `RotatingFileHandler` offers a variety of
  customization options for improved flexibility. For more information on available configurations,
  refer to the `RotatingFileHandlerConfig` documentation.

  For example

  ```c++
    // Create a rotating file handler which rotates daily at 18:30 or when the file size reaches 2GB
  std::shared_ptr<quill::Handler> file_handler =
    quill::rotating_file_handler(filename,
                                 []()
                                 {
                                   quill::RotatingFileHandlerConfig cfg;
                                   cfg.set_rotation_time_daily("18:30");
                                   cfg.set_rotation_max_file_size(2'000'000'000);
                                   return cfg;
                                 }());

  // Create a logger using this handler
  quill::Logger* logger_bar = quill::create_logger("daily_logger", std::move(file_handler));
  ```

- Improved compatibility with older versions of external `libfmt`. Quill now compiles for all versions
  of `libfmt >= 8.0.0`.

## v3.2.0

- Addition of std::is_trivially_copyable<T> to default copy loggable
  types. ([#318](https://github.com/odygrd/quill/pull/318))
- By default, the static library now builds with '-fPIC' to generate position-independent code.
  To disable this feature, you can use the CMake option 'QUILL_DISABLE_POSITION_INDEPENDENT_CODE'.
- The `LOG_<LEVEL>_LIMIT` macros now support using `std::chrono` duration types for specifying the log interval.
  Instead of providing a raw number, you can use:

  ```c++
      LOG_INFO_LIMIT(std::chrono::milliseconds {100} , quill::get_logger(), "log message");
  ```

## v3.1.0

- It is now possible to set a minimum logging interval for specific logs. For example:

  ```c++
    for (uint64_t i = 0; i < 10; ++i)
    {
      LOG_INFO_LIMIT(2000, default_logger, "log in a loop with limit 1 message every 2000 micros for i {}", i);
      std::this_thread::sleep_for(std::chrono::microseconds{1000});
    }
  ```

- `quill::utility::to_string()` now uses `fmt::to_string()`

- Quill now utilizes a custom namespace (`fmtquill`) for the bundled fmt library. This enables smooth integration with
  your own external fmt library, even if it's a different version.

## v3.0.2

- Add missing header on clang when `QUILL_X86ARCH` is defined.

## v3.0.1

- Enhanced the reported message for reallocation of the unbounded queue to include the thread id.

## v3.0.0

- The previous unbounded queue constantly reallocated memory, risking system memory exhaustion, especially when handling
  intensive logging from multiple threads. Starting from `v3.0.0`, the default behavior has been improved to limit
  the queue capacity to 2 GB. When this limit is reached, the queue blocks the hot thread instead of further
  reallocation.
  To modify the default behavior, there is no need to recompile the `quill` library. Recompile your application
  with one of the following header-only flags.

  ```shell
  # Previous behavior in v2.*.*: Reallocates new queues indefinitely when max capacity is reached
  -DCMAKE_CXX_FLAGS:STRING="-DQUILL_USE_UNBOUNDED_NO_MAX_LIMIT_QUEUE"
  
  # Default behavior in v3.*.*: Starts small, reallocates up to 2GB, then hot thread blocks
  -DCMAKE_CXX_FLAGS:STRING="-DQUILL_USE_UNBOUNDED_BLOCKING_QUEUE"
  
  # Starts small, reallocates up to 2GB, then hot thread drops log messages
  -DCMAKE_CXX_FLAGS:STRING="-DQUILL_USE_UNBOUNDED_DROPPING_QUEUE"
  
  # Fixed queue size, no reallocations, hot thread drops log messages
  -DCMAKE_CXX_FLAGS:STRING="-DQUILL_USE_BOUNDED_QUEUE"         
  
  # Fixed queue size, no reallocations, hot thread blocks
  -DCMAKE_CXX_FLAGS:STRING="-DQUILL_USE_BOUNDED_BLOCKING_QUEUE"
  ```

- Added support for huge pages on Linux. Enabling this feature allows bounded or unbounded queues to utilize huge pages,
  resulting in optimized memory allocation.

  ```c++
    quill::Config cfg;
    cfg.enable_huge_pages_hot_path = true;
    
    quill::configure(cfg);
    quill::start();
  ```

- Added support for logging `std::optional`, which is also now supported in `libfmt` `v10.0.0`.

  ```c++
    LOG_INFO(default_logger, "some optionals [{}, {}]", std::optional<std::string>{},
             std::optional<std::string>{"hello"});
  ```

- Introduced a new function `run_loop` in the `Handler` base class, which allows users to override and execute periodic
  tasks. This enhancement provides users with the flexibility to perform various actions at regular intervals,
  such as batch committing data to a database.
- In scenarios where a hot thread is blocked and unable to push messages to the queue in blocking mode, this situation
  will now be reported through the `backend_thread_notifications_handler` to the standard error stream `cerr`.

## v2.9.2

- Fix increased compile times due to `x86intrin` headers. ([#298](https://github.com/odygrd/quill/pull/298))
- Fix compile error when using `QUILL_X86ARCH` on windows.
- Fix bugs when quill is build as a shared library on windows. ([#302](https://github.com/odygrd/quill/pull/302))

## v2.9.1

- Removed `CMAKE_INSTALL_RPATH` from cmake. ([#284](https://github.com/odygrd/quill/pull/284))
- Fix compile warning on Apple M1. ([#291](https://github.com/odygrd/quill/pull/291))
- Update bundled `libfmt` to `v10.0.0`
- Fix for `CMAKE_MODULE_PATH` ([#295](https://github.com/odygrd/quill/pull/295))
- Fixed a bug in `TimeRotatingFileHandler` when `quill::FilenameAppend::None` is
  used. ([#296](https://github.com/odygrd/quill/pull/296))
- Fixed `TimeRotatingFileHandler` and `RotatingFileHandler` to work when `/dev/null` is used as a
  filename ([#297](https://github.com/odygrd/quill/pull/297))
- Added `NullHandler` that can be used to discard the logs. For example:

  ```c++
  int main()
  {
    quill::start();
    
    std::shared_ptr<quill::Handler> file_handler =
      quill::null_handler();
  
    quill::Logger* logger_bar = quill::create_logger("nullhandler", std::move(file_handler));
  
    for (uint32_t i = 0; i < 150; ++i)
    {
      LOG_INFO(logger_bar, "Hello");
    }
  ```

## v2.9.0

**Fixes**

- Fixed a bug in TimeRotatingFileHandler. ([#287](https://github.com/odygrd/quill/pull/287))

**Improvements**

- Renamed `backend_thread_error_handler` to `backend_thread_notifications_handler` in `Config.h`. Previously this
  handler was used only to report errors from the backend worker thread to the user. This callback will also now report
  info messages to the user.
- Report unbounded spsc queue reallocation via
  the `backend_thread_notifications_handler`. ([#286](https://github.com/odygrd/quill/pull/286))
- Report bounded spsc queue dropped messages via the `backend_thread_notifications_handler`.

## v2.8.0

**Breaking Changes**
(see `improvements` section for more details)

- If you were previously compiling with `-DQUILL_USE_BOUNDED_QUEUE` or `QUILL_X86ARCH` you should now pass the
  flag to you target as it is not propagated by CMake anymore.
- There is a change in the API in `Quill.h` instead of `quill::Handler*` you should now use
  `std::shared_ptr< quill::Handler >` and also move it to the created logger.

**Improvements**

- Add `append_to_filename` parameter when creating `quill::time_rotating_file_handler`
  and `quill::rotating_file_handler`
- Fix `Handlers` failing to find the file when the working directory of the application is changed in
  runtime. ([#247](https://github.com/odygrd/quill/pull/247))
- When the given output directory of a log file passed to a `Handler` does not exist, it will now get automatically
  created.
- Support Windows 10 LTSB 2016, 1607 and Server 2016. ([#251](https://github.com/odygrd/quill/pull/251))
- Add back `backend_thread_sleep_duration` in `Config.h` ([#256](https://github.com/odygrd/quill/pull/256))
- For `quill::rotating_file_handler(...)` and  `quill::time_rotating_file_handler(...)` the `backup_count` argument is
  now default to `std::numeric_limits<std::uint32_t>::max()`
- When the logging file is deleted from the command line while the logger is still using it, then a new file will be
  reopened for writing.
- Added `quill::Clock` which enables taking and converting TSC timestamps to system clock timestamps.
  When `TimestampClockType::Tsc` is used as the default clock type in `Config.h` this class
  can also be used to generate timestamps that are in sync with the timestamps in the log
  file. ([#264](https://github.com/odygrd/quill/pull/264))
- Both `Unbounded` and `Bounded` queue modes can now be used without having to recompile `quill` library. This is still
  not a runtime option, you still need to recompile your target and pass `QUILL_USE_BOUNDED_QUEUE` as a flag.
  See [example_bounded_queue_message_dropping.cpp](https://github.com/odygrd/quill/blob/master/examples/example_bounded_queue_message_dropping.cpp)
- Added `QUILL_USE_BOUNDED_BLOCKING_QUEUE` option that makes possible to use a bounded queue which blocks the hot
  thread rather than dropping messages ([#270](https://github.com/odygrd/quill/pull/270))
  See [example_bounded_queue_blocking.cpp](https://github.com/odygrd/quill/blob/master/examples/example_bounded_queue_blocking.cpp)
- Renamed `backend_thread_max_transit_events` to `backend_thread_transit_events_soft_limit` in
  Config.h ([#270](https://github.com/odygrd/quill/pull/270))
- Added `backend_thread_transit_events_hard_limit` in Config.h ([#270](https://github.com/odygrd/quill/pull/270))
- Added `backend_thread_use_transit_buffer` in Config.h ([#270](https://github.com/odygrd/quill/pull/270))
- CMake: `QUILL_X86ARCH` and `QUILL_USE_BOUNDED_QUEUE` options have been removed. The users can decide on enabling these
  options on their side and quill doesn't need to be recompiled as a library. For example :
  ```cmake
     target_compile_definitions(<target> PUBLIC QUILL_X86ARCH QUILL_USE_BOUNDED_QUEUE)
  ```
- Added `quill::remove_logger(Logger* logger)` in `Quill.h`. This makes it possible to remove a logger in a thread safe
  way. When a logger is removed any associated `FileHandlers` with that logger will also be removed and the files will
  also be closed as long as they are not being used by another logger. The logger is asynchronously removed by the
  logging
  thread after all the messages are written. To achieve this the API had to change to return a
  `std::shared_ptr< quill::Handler >` instead of `quill::Handler*`. See
  [example_file_callbacks.cpp](https://github.com/odygrd/quill/blob/master/examples/example_file_callbacks.cpp)
- Added `quill::wake_up_logging_thread()` in `Quill.h`. This thread safe function can be used to wake up the backend
  logging thread on demand. ([#280](https://github.com/odygrd/quill/pull/280))
- Round up queue capacity to the nearest power of 2. ([#282](https://github.com/odygrd/quill/pull/282))

## v2.7.0

**Fixes**

- Remove references to build directory path from the compiled library's
  symbols. ([#221](https://github.com/odygrd/quill/pull/221))
- Fix when compiled as shared library with hidden visibility. ([#222](https://github.com/odygrd/quill/pull/222))
- Fix equal timestamp log messages appearing out of order. ([#223](https://github.com/odygrd/quill/pull/223))
- Reduce padding in some structs.
- Fix 'rename_file' throwing an exception while being marked
  as `noexcept`. ([#230](https://github.com/odygrd/quill/pull/230))
- Fix crash with `std::bad_alloc` and compiler warnings in
  gcc `7.3.1`. ([#235](https://github.com/odygrd/quill/pull/235))
- The additional compiler definitions will now be propagated to the parent targets when enabling options in
  CMake. ([#235](https://github.com/odygrd/quill/pull/235))

**Improvements**

- Improved performance and throughput of the backend logging thread by approximately ~25%
- Add missing `quill::json_file_handler(...)` that creates a `JsonFileHandler` in `Quill.h`.
- Simplified and refactored the logic in `BoundedQueue`.
- Added the option `do_fsync` which also calls `fsync()` during the handler flush to all file handlers.
- Replace `backend_thread_sleep_duration` with `backend_thread_yield` in `Config.h`
- Remove trailing spaces in log levels strings. ([#237](https://github.com/odygrd/quill/pull/237))
- The default log pattern has changed
  to `"%(ascii_time) [%(thread)] %(fileline:<28) LOG_%(level_name:<9) %(logger_name:<12) %(message)")`
- Added file event notifiers, to get callbacks from quill before/after log file has been opened or
  closed. ([#193](https://github.com/odygrd/quill/pull/193))
  This is useful for cleanup procedures or for adding something to the start/end of the log files.
  for example

  ```c++
  int main()
  {
    quill::start();
  
    quill::FileEventNotifier fen;
  
    fen.before_open = [](quill::fs::path const& filename)
    { std::cout << "before opening " << filename << std::endl; };
  
    fen.after_open = [](quill::fs::path const& filename, FILE* f)
    { std::cout << "after opening " << filename << std::endl; };
  
    fen.before_close = [](quill::fs::path const& filename, FILE* f)
    { std::cout << "before closing " << filename << std::endl; };
  
    fen.after_close = [](quill::fs::path const& filename)
    { std::cout << "after closing " << filename << std::endl; };
  
    quill::Handler* file_handler =
      quill::file_handler("myfile.log", "w", quill::FilenameAppend::None, std::move(fen));
  
    quill::Logger* mylogger = quill::create_logger("mylogger", file_handler);
  
    LOG_INFO(mylogger, "Hello world");
  }
  ```

- Added `QUILL_X86ARCH` in `Tweakme.h`. When enabled it will attempt to minimize the cache pollution on x86 cpus that
  support the instructions `_mm_prefetch `, `_mm_clflush` and `_mm_clflushopt`.

  To compile when this flag is enabled you should also pass `-march` to the compiler which is required,
  you can set this to your oldest cpu architecture among your systems.

  To enable this option, `DQUILL_X86ARCH` must always be defined in quill library and also in your executable,
  for example

  ```shell
  cmake -DCMAKE_CXX_FLAGS:STRING="-DQUILL_X86ARCH -march=native"
  ```

- Added `quill:get_root_logger()` which gives quick access to the root logger object and can be used directly in the hot
  path.
  This gives applications that only wish to use the root logger the convenience of not having to store and
  pass `Logger*` objects anymore.
  for example quill existing log macros can be overwritten to not require a `Logger*` anymore

  ```c++
  #define MY_LOG_INFO(fmt, ...) QUILL_LOG_INFO(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  ``````

- Added `QUILL_ROOT_LOGGER_ONLY` in `Tweakme.h`. Define ths if you only plan to use the single `root` logger object,
  When this is defined it will replace the LOG_ macros with the equivalent LOG_ macros but without the need of
  passing `Logger*` objects anymore.
  for example

  ```c++
  #define QUILL_ROOT_LOGGER_ONLY
  #include "quill/Quill.h"
  
  int main()
  {
    quill::start();
  
    // because we defined QUILL_ROOT_LOGGER_ONLY we do not have to pass a logger* anymore, the root logger is always used
    LOG_INFO("Hello {}", "world");
    LOG_ERROR("This is a log error example {}", 7);
  }
  ```

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

- Previously when multiple threads were logging, Quill backend logging thread would first try reading the log messages
  of
  one thread until the queue was completely empty before reading the log messages of the next thread.
  When one of the threads was logging a lot, it could result in only displaying the log of that thread, hiding the
  logs of the other threads. This has now been fixed and all log messages from all threads are read fairly.

## v2.3.2

**Fixes**

- Fix code not compiling with treat warnings as errors set on
  Windows. ([#198](https://github.com/odygrd/quill/pull/198))

## v2.3.1

**Fixes**

- Optimise logging queue cache alignment of variables. It seems that v2.3.0 made the hot path slower by ~5 ns per
  message. This has been fixed in this version and the performance is now the same as in the previous versions.

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

[Config.h](https://github.com/odygrd/quill/blob/master/quill/include/quill/Config.h) - contains runtime configuration
options

[TweakMe.h](https://github.com/odygrd/quill/blob/master/quill/include/quill/TweakMe.h) - contains compile time
configuration

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
  has to be created. For example `quill::config::set_backend_thread_cpu_affinity(1);` has been removed and instead the
  following code is needed :

```cpp
  quill::Config cfg;
  cfg.backend_thread_cpu_affinity = 1;
  quill::configure(cfg);
```

- `QUILL_CHRONO_CLOCK` has been moved from `TweakMe.h` to `Config.h`. It is now possible to switch between `rdtsc`
  and `system`
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
