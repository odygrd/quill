- [v11.1.0](#v1110)
- [v11.0.2](#v1102)
- [v11.0.1](#v1101)
- [v11.0.0](#v1100)
- [v10.2.0](#v1020)
- [v10.1.0](#v1010)
- [v10.0.1](#v1001)
- [v10.0.0](#v1000)
- [v9.0.3](#v903)
- [v9.0.2](#v902)
- [v9.0.1](#v901)
- [v9.0.0](#v900)
- [v8.2.0](#v820)
- [v8.1.1](#v811)
- [v8.1.0](#v810)
- [v8.0.0](#v800)
- [v7.5.0](#v750)
- [v7.4.0](#v740)
- [v7.3.0](#v730)
- [v7.2.2](#v722)
- [v7.2.1](#v721)
- [v7.2.0](#v720)
- [v7.1.0](#v710)
- [v7.0.0](#v700)
- [v6.1.2](#v612)
- [v6.1.1](#v611)
- [v6.1.0](#v610)
- [v6.0.0](#v600)
- [v5.1.0](#v510)
- [v5.0.0](#v500)
- [v4.5.0](#v450)
- [v4.4.1](#v441)
- [v4.4.0](#v440)
- [v4.3.0](#v430)
- [v4.2.1](#v421)
- [v4.2.0](#v420)
- [v4.1.0](#v410)
- [v4.0.0](#v400)
- [v3.9.0](#v390)
- [v3.8.0](#v380)
- [v3.7.0](#v370)
- [v3.6.0](#v360)
- [v3.5.1](#v351)
- [v3.5.0](#v350)
- [v3.4.1](#v341)
- [v3.4.0](#v340)
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

## v11.1.0

- Fixed thread-local context duplication across shared libraries ([#890](https://github.com/odygrd/quill/issues/890))
- Added a `nullptr` check in macro-free log functions (`LogFunctions.h`) to allow calls with an uninitialized logger.
  Macro-based logging remains deliberately unchanged ([#894](https://github.com/odygrd/quill/issues/894))
- Suppress GCC false-positive warnings during LTO builds.
- Added backend worker poll loop hooks `backend_worker_on_poll_begin`/
  `backend_worker_on_poll_end` ([#897](https://github.com/odygrd/quill/issues/897))

## v11.0.2

- Fixed UBSAN warning when logging empty `std::string_view` ([#885](https://github.com/odygrd/quill/issues/885))
- Updated `ConsoleSink` constructor to accept an optional `FileEventNotifier` parameter for consistency with other
  sinks ([#886](https://github.com/odygrd/quill/issues/886))

## v11.0.1

- Fix `BacktraceStorage` `index` reset to prevent `SIGSEGV` when using `LOG_BACKTRACE`

## v11.0.0

- Update bundled `libfmt` to `v12.1.0`
- Minor correction to `_last_sink_flush_time update` when `fflush()` fails
- Added retry logic and shared access handling for file open and rotation on Windows
- Use `::WriteFile` instead of `fwrite` to prevent `\r\r\n` line endings on Windows
- Avoid file descriptor leaks by setting `O_CLOEXEC` on Unix and `HANDLE_FLAG_INHERIT` on Windows
- Added `SimpleSetup.h` convenience header for trivial program cases to easily setup a logger. For example
   ```c++
    #include "quill/SimpleSetup.h"
    #include "quill/LogMacros.h"
  
    int main()
    {
      auto* logger = quill::simple_logger();
      LOG_INFO(logger, "Hello from {}!", "Quill");
  
      auto* logger2 = quill::simple_logger("test.log");
      LOG_INFO(logger2, "This message goes to a file");
    }
  ```
- Fixed argument forwarding when encoding user-defined types with `DeferredFormatCodec` or STL containers to properly
  handle rvalue references. For example, the following move-only type will now work correctly:
    ```c++
    class MoveOnlyType {
    public:
      MoveOnlyType(std::string name, std::string value, uint32_t count)
        : name(std::move(name)), value(std::move(value)), count(count) {}
      MoveOnlyType(MoveOnlyType&&) = default;
      MoveOnlyType(MoveOnlyType const&) = delete;
      std::string name;
      std::string value;
      uint32_t count;
    };

    template <>
    struct fmtquill::formatter<MoveOnlyType> {
      constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
      auto format(MoveOnlyType const& obj, format_context& ctx) const {
        return fmtquill::format_to(ctx.out(), "MoveOnlyType(name: {}, value: {}, count: {})",
                                   obj.name, obj.value, obj.count);
      }
    };

    template <>
    struct quill::Codec<MoveOnlyType> : quill::DeferredFormatCodec<MoveOnlyType> {};

    MoveOnlyType obj{"Test", "Value1", 42};
    LOG_INFO(logger, "obj: {}", std::move(obj));  // Properly forwards and moves
    ```

## v10.2.0

- Added fuzzing tests to CI to catch memory and undefined behavior issues
- Fixed `PatternFormatter` automatic newline appending by making the suffix ('\n') configurable or optionally disabled
  via `PatternFormatterOptions`
- Fixed segmentation fault when `DirectFormatCodec` was used with enums
  types ([#848](https://github.com/odygrd/quill/issues/848))
- Fixed segmentation fault when `DirectFormatCodec` was used with STL containers of enums
- Fixed a compiler error when `LOG_DYNAMIC` is used with
  `QUILL_DISABLE_FILE_INFO` ([#847](https://github.com/odygrd/quill/issues/847))
- Fixed process ID capture in `BackendWorker` to support `fork()`
  correctly ([#860](https://github.com/odygrd/quill/issues/860))
- Fixed undefined behavior caused by passing `nullptr` to `memcpy` when encoding empty `std::vector`
- Updated `BackendOptions::check_printable_char` to allow tab (`\t`) and carriage return (`\r`) characters by
  default ([#856](https://github.com/odygrd/quill/issues/856))
- Increased `RdtscClock` resync lag thresholds to improve cross-system compatibility
- Added `QUILL_ENABLE_ASSERTIONS` CMake option and preprocessor flag to enable assertions in release builds
- Allow `RotatingSink` to rotate the file on creation with `rotation_on_creation()`
- Improved `SignalHandlerOptions` configurability by replacing hardcoded logger exclusion string with
  `excluded_logger_substrings` option
- Silence MSVC warnings (4324, 4996) in source code instead of CMake

## v10.1.0

- Fixed potential conflicts by adding unique prefixes to internal macro variables to prevent conflicts with user
  variable names in the same
  scope. ([#799](https://github.com/odygrd/quill/issues/799))
- Fixed potential `UBSan` warnings by adding overflow check when doubling resync interval in
  `RdtscClock::resync()` ([#809](https://github.com/odygrd/quill/issues/809))
- Minor improvements in `Utility::to_hex` function and `StringFromTime`
- Fixed an issue where `BackendWorker::_exit` was always executed during destruction, even when the backend thread had
  already stopped ([#815](https://github.com/odygrd/quill/issues/815))
- Fixed `RotatingFileSink` to correctly handle `FilenameAppendOption::StartDate` configuration ([#822](https://github.com/odygrd/quill/issues/822))
- Fixed unnecessary allocation caused by empty `std::vector` while `BackendWorker` is idle on
  Windows ([#827](https://github.com/odygrd/quill/issues/827))
- Fixed `FileSink::open_mode` string comparison for file mode flags
- Adjusted default `BackendOptions` values for broader system compatibility and typical usage patterns
  
## v10.0.1

- Fixed PatternFormatter test to work with any repository name instead of hardcoded `quill` ([#795](https://github.com/odygrd/quill/issues/795))
- Fixed Windows compiler warnings when clang-cl >= 19 is used

## v10.0.0

### New Features

- There is a new macro-free mode that allows logging without macros. You have two options: either
  `#include "quill/LogMacros.h"` or `#include "quill/LogFunctions.h"`. The macro mode still remains the recommended and
  main method for logging. The new macro-free log has higher overhead than using macros. To use the macro-free mode, for
  example:

  ```cpp
  quill::debug(logger, "A {} message with number {}", "test", 123);
  ```

  See macro-free mode documentation [here](https://quillcpp.readthedocs.io/en/latest/macro_free_mode.html) for details.

- Added `BinaryDataDeferredFormatCodec` for efficient binary data logging. This codec allows efficient logging of
  variable-sized binary data by copying the raw bytes on the hot path and deferring the formatting to the backend
  thread. This is particularly useful for logging binary protocol messages (like SBE or custom binary formats),
  network packets, and raw binary data without impacting application performance. See the
  example [sbe_logging](https://github.com/odygrd/quill/blob/master/examples/sbe_binary_data/sbe_logging.cpp) and
  [binary_protocol_logging](https://github.com/odygrd/quill/blob/master/examples/binary_protocol_logging.cpp)
  for details.
  For documentation, see [here](https://quillcpp.readthedocs.io/en/latest/binary_protocols.html).

- The immediate flush feature has been enhanced to support interval-based flushing and moved to runtime. This feature
  helps with debugging by ensuring log statements are flushed to the sink, blocking the caller
  thread. ([#660](https://github.com/odygrd/quill/issues/660))
  
- Added `source_location_path_strip_prefix` option in `PatternFormatterOptions` to customize the display of the
  `%(source_location)` attribute of `PatternFormatter`. When set, any paths that contain this prefix will have
  the prefix and everything before it stripped from the displayed path. For example, with prefix "projects",
  a source location like "/home/user/projects/app/main.cpp:5" would be displayed as "app/main.cpp:
  5". ([#772](https://github.com/odygrd/quill/issues/772))

- Added `source_location_remove_relative_paths` option in `PatternFormatterOptions` to remove relative path
  components from the `%(source_location)` attribute of `PatternFormatter`. When enabled, relative path
  components like "../" are processed and removed, simplifying paths from `__FILE__` which might contain
  relative paths like "../../../test/main.cpp". ([#778](https://github.com/odygrd/quill/issues/778))

- Added the `QUILL_DISABLE_FILE_INFO` preprocessor flag and CMake option.  
  This disables `__FILE__` and `__LINE__` information in log statements at compile time when location-related patterns
  (`%(file_name)`, `%(line_number)`, `%(short_source_location)`, `%(source_location)`)
  are not needed in the `PatternFormatter`. This removes embedded source path strings from built binaries from the
  security viewpoint.

- Added the `QUILL_DETAILED_FUNCTION_NAME` CMake option. When enabled, this option uses compiler-specific detailed
  function signatures (such as `__PRETTY_FUNCTION__` on GCC/Clang or `__FUNCSIG__` on MSVC) instead of the standard
  `__FUNCTION__` in log macros. This provides more complete function information, including return types, namespaces,
  and parameter types. This option is only relevant when `%(caller_function)` is used in the pattern
  formatter. ([#785](https://github.com/odygrd/quill/issues/785))

- Added `process_function_name` customisation point in `PatternFormatterOptions`. This function allows custom processing
  of the function signature before it's displayed in logs. This makes more sense to use when
  `QUILL_DETAILED_FUNCTION_NAME` is used. This provides flexibility to trim, format, or otherwise modify function
  signatures to improve readability in log output when using the `%(caller_function)`
  pattern. ([#785](https://github.com/odygrd/quill/issues/785))

- Added helper macros for easy logging of user-defined types. Two new macros are available in `quill/HelperMacros.h`:
  - `QUILL_LOGGABLE_DIRECT_FORMAT(Type)`: For types that contain pointers or have lifetime dependencies
  - `QUILL_LOGGABLE_DEFERRED_FORMAT(Type)`: For types that only contain value types and are safe to copy

  Note that these macros require you to provide either an `operator<<` for your type and they are just shortcuts to
  existing functionality. ([#777](https://github.com/odygrd/quill/issues/777))

  Example usage:
  ```cpp
  class User { /* ... */ };
  std::ostream& operator<<(std::ostream& os, User const& user) { /* ... */ }
  
  // For types with pointers - will format immediately
  QUILL_LOGGABLE_DIRECT_FORMAT(User)
  
  class Product { /* ... */ };
  std::ostream& operator<<(std::ostream& os, Product const& product) { /* ... */ }
  
  // For types with only value members - can format asynchronously
  QUILL_LOGGABLE_DEFERRED_FORMAT(Product)
  ```

### Improvements

- Internally, refactored how runtime metadata are handled for more flexibility, providing three macros for logging with
  runtime metadata:
  - `QUILL_LOG_RUNTIME_METADATA_DEEP` - Takes a deep copy of `fmt`, `file`, `function` and `tags`. Most flexible
    option, useful for forwarding logs from another logging library.
  - `QUILL_LOG_RUNTIME_METADATA_HYBRID` - Will take a deep copy of `fmt` and `tags` and will take `file` and
    `function` as reference. This is used for the new macro-free mode.
  - `QUILL_LOG_RUNTIME_METADATA_SHALLOW` - Will take everything as reference. This is used when logging with
    compile-time metadata and using, for example, a dynamic log-level such as `LOG_DYNAMIC`.

- When using a sink with overridden `PatternFormatterOptions`, the option `add_metadata_to_multi_line_logs` will now be
  correctly applied at the Sink level. Previously, this option was only available and effective at the Logger level
  `PatternFormatter`.

- When a Sink with override `PatternFormatterOptions` is used and if no other sink exists using the `Logger`
  `PatternFormatterOptions`, then the backend thread will no longer perform a redundant format log statement.

- When using a sink with overridden `PatternFormatterOptions`, the `log_statement` that is passed to the
  `Filter::filter()` will now be formatted based on the overridden options instead of using the `Logger`
  `PatternFormatterOptions`.

- Update bundled `libfmt` to `v11.2.0`

### API Changes

- If you were previously setting `QUILL_ENABLE_IMMEDIATE_FLUSH` to `1`, this functionality has been moved to runtime
  with more flexibility. Instead of using a boolean flag, you can now specify the flush interval by calling
  `logger->set_immediate_flush(flush_every_n_messages)` on each logger instance. Set it to `1` for per-message flushing,
  or to a higher value to flush after that many messages. Setting it to `0` disables
  flushing which is the default behaviour. `QUILL_ENABLE_IMMEDIATE_FLUSH` still exists as a compile-time preprocessor
  flag and is set to `1` by default. Setting `QUILL_ENABLE_IMMEDIATE_FLUSH 0` in the preprocessor will eliminate the
  `if` branch from the hot path and disable this feature entirely, regardless of the value passed to
  `set_immediate_flush(flush_every_n_messages)`.

- The `QUILL_LOG_RUNTIME_METADATA` macro requires `file`, `function` and `fmt` to be passed as `char const*` and
  `line_number` as `uint32_t`. This is a breaking change from the previous version.

## v9.0.3

- Add support for `void*` formatting ([#759](https://github.com/odygrd/quill/issues/759))
- Fix a bug in `RotatingJsonFileSink.h` where file size-based rotation wasn't triggering
  properly ([#767](https://github.com/odygrd/quill/issues/767))
- Fix false positive `-Wstringop-overread` warning in GCC ([#766](https://github.com/odygrd/quill/issues/766))

## v9.0.2

- Add missing namespace in `QUILL_LOG_RUNTIME_METADATA` ([#743](https://github.com/odygrd/quill/issues/743))

## v9.0.1

- Fix crash when `LOG_BACKTRACE` is used ([#744](https://github.com/odygrd/quill/issues/744))
- Add missing namespace in `QUILL_LOG_RUNTIME_METADATA` ([#743](https://github.com/odygrd/quill/issues/743))
- Check for `nullptr` `Logger*` before setting log level via `QUILL_LOG_LEVEL` environment
  variable ([#749](https://github.com/odygrd/quill/issues/749))
- Change default mode of `FileSink` `fopen` to `a` to avoid overwriting existing files

## v9.0.0

### API Changes

- Replaced the `bool huge_pages_enabled` flag in `FrontendOptions` with `quill::HugePagesPolicy huge_pages_policy` enum,
  allowing huge page allocation to be attempted with a fallback to normal pages if unavailable. If you are using a
  custom `FrontendOptions` type, you will need to update it to use the new
  flag. ([#707](https://github.com/odygrd/quill/issues/707))
- Previously, `QueueType::UnboundedDropping` and `QueueType::UnboundedBlocking` could grow up to 2 GB in size. This
  limit is now configurable via `FrontendOptions::unbounded_queue_max_capacity`, which defaults to 2 GB.
- `QueueType::UnboundedUnlimited` has been removed, as the same behavior can now be achieved by setting
  `FrontendOptions::unbounded_queue_max_capacity` to the maximum value.
- The `ConsoleSink` constructor now optionally accepts a `ConsoleSinkConfig`, similar to other sinks. If no
  `ConsoleSinkConfig` is provided, a default one is used, logging to `stdout` with `ColourMode::Automatic`. For example:
    ```c++
    Frontend::create_or_get_sink<ConsoleSink>("console_sink",
                                              []()
                                              {
                                                ConsoleSinkConfig config;
                                                config.set_colour_mode(ConsoleSinkConfig::ColourMode::Never);
                                                config.set_stream("stderr");
                                                return config;
                                              }());
    ```

### New Features

- The default log level for each `Logger` can now be configured using the environment variable `QUILL_LOG_LEVEL`.
  Supported values: `"tracel3"`, `"tracel2"`, `"tracel1"`, `"debug"`, `"info"`, `"notice"`, `"warning"`, `"error"`,
  `"critical"`, `"none"`. When set, the logger is initialized with the corresponding log level. If
  `logger->set_log_level(level)` is explicitly called in code, it will override the log level set via the environment
  variable.
- Added the `LOG_RUNTIME_METADATA(logger, log_level, file, line_number, function, fmt, ...)` macro.  
  This enables passing runtime metadata (such as file, line number, and function) along with a log message,  
  providing greater flexibility when forwarding logs from other logging
  libraries. ([#696](https://github.com/odygrd/quill/issues/696))

    ```c++
    LOG_RUNTIME_METADATA(logger, quill::LogLevel::Info, "main.cpp", 20, "foo()", "Hello number {}", 8);
    ```
- Added a runtime check to detect duplicate backend worker threads caused by inconsistent linkage  
  (e.g., mixing static and shared libraries). If needed, this check can be disabled using the  
  `check_backend_singleton_instance` flag in
  `BackendOptions`. ([#687](https://github.com/odygrd/quill/discussions/687#discussioncomment-12349621))
- Added the `QUILL_DISABLE_FUNCTION_NAME` preprocessor flag and CMake option.  
  This allows disabling `__FUNCTION__` in `LOG_*` macros when `%(caller_function)` is not used in `PatternFormatter`,  
  eliminating Clang-Tidy warnings when logging inside lambdas.
- It is now possible to override a logger's `PatternFormatter` on a per-sink basis. This allows the same logger to
  output different formats for different sinks. Previously, achieving this required creating a custom sink type, but
  this functionality is now built-in. See the
  example: [sink_formatter_override](https://github.com/odygrd/quill/blob/master/examples/sink_formatter_override.cpp).
- Added `Frontend::remove_logger_blocking(...)`, which blocks the caller thread until the specified logger is fully
  removed.
- Added `Frontend::shrink_thread_local_queue(capacity)` and `Frontend::get_thread_local_queue_capacity()`.
  These functions allow dynamic management of thread-local SPSC queues when using an unbounded queue configuration. They
  enable on-demand shrinking of a queue that has grown due to bursty logging, helping to reduce memory usage, although
  in typical scenarios they won't be required.
- Added the `SyslogSink`, which logs messages to the system's syslog.

    ```c++
    auto sink = quill::Frontend::create_or_get_sink<quill::SyslogSink>(
      "app", []()
      {
        quill::SyslogSinkConfig config;
        config.set_identifier("app");
        return config;
      }());
    ```
- Added the `SystemdSink`, which logs messages to systemd.

    ```c++
    auto sink = quill::Frontend::create_or_get_sink<quill::SystemdSink>(
      "app", []()
      {
        quill::SystemdSinkConfig config;
        config.set_identifier("app");
        return config;
      }());
    ```  
- Added the `AndroidSink`, which integrates with Android's logging system.

    ```c++
    auto sink = quill::Frontend::create_or_get_sink<quill::AndroidSink>(
      "s1", []()
      {
        quill::AndroidSinkConfig config;
        config.set_tag("app");
        config.set_format_message(true);
        return config;
      }());
    ```

### Improvements

- Updated bundled `libfmt` to `11.1.4`.
- When `add_metadata_to_multi_line_logs` in the `PatternFormatter` was set to false, fixed a bug where the last
  character of the log message was dropped and added protection for empty messages.
- Updated `LOG_EVERY_N` macros to log on the first occurrence (0th call) instead of waiting until the Nth call.
- Added a `nullptr` check for `char*` and `const char*` during encoding, ensuring the library handles `nullptr` values
  gracefully ([#735](https://github.com/odygrd/quill/discussions/735))
- The `CsvWriter` could previously be used with `RotatingFileSink` via the constructor that accepted  
  `std::shared_ptr<Sink>`, but rotated files did not include the CSV header.  
  This has now been improved—when using the new constructor that accepts `quill::RotatingFileSinkConfig`,  
  the CSV header is written at the start of each new rotated
  file. ([#700](https://github.com/odygrd/quill/discussions/700))

    ```c++
    quill::RotatingFileSinkConfig sink_config;
    sink_config.set_open_mode('w');
    sink_config.set_filename_append_option(FilenameAppendOption::None);
    sink_config.set_rotation_max_file_size(512);
    sink_config.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::Index);
    
    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writer{"orders.csv", sink_config};
    for (size_t i = 0; i < 40; ++i)
    {
      csv_writer.append_row(132121122 + i, "AAPL", i, 100.1, "BUY");
    }
    ```
- When using `CsvWriter` with `open_mode == 'a'`, the header will no longer be rewritten if the file already exists.
- On Linux, setting a long backend thread name now truncates it instead of
  failing. ([#691](https://github.com/odygrd/quill/issues/691))
- Fixed BSD builds. ([#688](https://github.com/odygrd/quill/issues/688))
- Added adaptive termination for stable measurements in `rdtsc` calibration during init
- Fixed `QUILL_ATTRIBUTE_HOT` and `QUILL_ATTRIBUTE_COLD` clang detection
- CMake improvements: switched to range syntax for minimum required version and bumped minimum required CMake version to
  `3.12`. ([#686](https://github.com/odygrd/quill/issues/686))
- Correct the installation location of pkg-config files. They are now properly placed in `/usr/local/lib`.
  ([#715](https://github.com/odygrd/quill/issues/715))
- Removed deprecated `TriviallyCopyableTypeCodec`

## v8.2.0

- Added `DeferredFormatCodec` and `DirectFormatCodec` for easier logging of user-defined types and smoother migration
  from pre-`v4` versions. Previously, users had to define a custom `Codec` for every non-trivially copyable user-defined
  type they wanted to log.

   ```c++
   template <>
   struct quill::Codec<UserTypeA> : quill::DeferredFormatCodec<UserTypeA>
   {
   };
      
   template <>
   struct quill::Codec<UserTypeB> : quill::DirectFormatCodec<UserTypeB>
   {
   };
   ```

    - `DeferredFormatCodec` now supports both trivially and non-trivially copyable types:
        - For trivially copyable types, it behaves the same as `TriviallyCopyableTypeCodec`.
        - For non-trivially copyable types, it works similarly to pre-`v4` by taking a copy of the object using the copy
          constructor and placement new.
    - `DirectFormatCodec` formats the object immediately in the hot path, serving as a shortcut to explicitly formatting
      the object when logging.
    - For advanced use cases, a custom `Codec` can still be defined for finer control over encoding/decoding.

  See:
    - [DeferredFormatCodec Usage](https://github.com/odygrd/quill/blob/master/examples/user_defined_types_logging_deferred_format.cpp)
    - [DirectFormatCodec Usage](https://github.com/odygrd/quill/blob/master/examples/user_defined_types_logging_direct_format.cpp)
    - [Documentation](https://quillcpp.readthedocs.io/en/latest/cheat_sheet.html#logging-user-defined-types)

- Added codec support for C-style arrays of user-defined types in `std/Array.h`
- Fixed warnings: `-Wimplicit-int-float-conversion`, `-Wfloat-equal`, and `-Wdocumentation`.
- Marked `TriviallyCopyableTypeCodec` as deprecated. `DeferredFormatCodec` should be used instead, requiring no further
  changes.
- Raised minimum `CMake` required version from `3.8` to `3.10` to avoid deprecation warnings.

## v8.1.1

- Updated bazel `rules_cc` to `0.1.1` ([#664](https://github.com/odygrd/quill/issues/664))

## v8.1.0

- Updated bundled `libfmt` to `11.1.3`
- Suppressed clang-19 warning when building the tests with C++17. ([#646](https://github.com/odygrd/quill/issues/646))
- Fixed windows linkage error when shared library is used.
- Fixed redefinition of `struct fmt_detail::time_zone` error ([#649](https://github.com/odygrd/quill/issues/649))

## v8.0.0

- Unified `JsonFileSink.h` and `JsonConsoleSink.h` into a single header, `JsonSink.h`, with both classes now sharing a
  common implementation
- Users can now inherit from `JsonFileSink` or `JsonConsoleSink` and override the `generate_json_message(...)` function
  to implement their own custom JSON log formats
- Removed `JsonFileSinkConfig`. Please rename it to `FileSinkConfig`, which retains the same API and is fully
  compatible.
- Added `RotatingJsonFileSink`. Functions like `RotatingFileSink`, but specifically designed for rotating JSON log
  files. ([#637](https://github.com/odygrd/quill/issues/637))
- Simplified `ConsoleSink` by applying ANSI colour codes universally across all platforms, including Windows. The
  previous Windows-specific implementation has been removed. Note that `quill::ConsoleColours` has been replaced with
  `quill::ConsoleSink::Colours`, and `quill::ConsoleColours::ColourMode` has been renamed to
  `quill::ConsoleSink::ColourMode`.
- Changed class member visibility in `FileSink`, `JsonSink`, and `RotatingSink` from private to protected, enabling
  easier customization through inheritance for user-defined implementations.
- Added a new `sink_min_flush_interval` option in `BackendOptions`, which specifies the minimum time interval (in
  milliseconds) before the backend thread flushes the output buffers calling `flush_sink()` for all sinks, with a
  default value of 200ms; The backend thread ensures sinks aren't flushed more frequently than this interval, while
  explicit calls to `logger->flush_log()` trigger an immediate flush, and flushing may occur less frequently if the
  backend thread is busy, with this setting applying globally to all
  sinks. Setting this value to 0 disables the feature. ([#641](https://github.com/odygrd/quill/issues/641))
- Updated bundled `libfmt` to `11.1.2`
- Added a `StopWatch` utility for easy logging of elapsed time. It can log the time elapsed since construction in
  various formats. You can use either `quill::StopWatchTsc` for high-resolution TSC-based timing or
  `quill::StopWatchChrono` for standard std::chrono-based timing. ([#640](https://github.com/odygrd/quill/issues/640))

  For example:
  ```c++
    #include "quill/StopWatch.h"

    quill::StopWatchTsc swt;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    LOG_INFO(logger, "After 1s, elapsed: {:.6}s", swt); // => After 1s, elapsed: 1.00849s
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    LOG_INFO(logger, "After 500ms, elapsed: {}s", swt); // => After 500ms, elapsed: 1.521880274s
    LOG_INFO(logger, "elapsed: {}", swt.elapsed_as<std::chrono::nanoseconds>()); // => elapsed: 1521807324ns
  ```
- Suppress `-Wredundant-decls` warning in GCC builds.
- Remove `-Wno-gnu-zero-variadic-macro-arguments` for GCC in CMake.

## v7.5.0

- In previous versions, logging on Windows automatically included `windows.h` in all components. The frontend will no
  longer include `windows.h`. By following the recommended usage example
  provided [here](https://github.com/odygrd/quill/blob/master/examples/recommended_usage/recommended_usage.cpp) as
  guidance, you can create a wrapper library around Quill for the backend, allowing you to
  log on Windows without including `windows.h` in the frontend or main
  program. ([#618](https://github.com/odygrd/quill/issues/618))

- The `LOG_LEVEL_LIMIT` time-based rate-limiting macros now log the count of how many times a message would be logged
  when throttled. For example, a log message may appear as `A log message with number 123 (21x)` to indicate that the
  message would have been logged 21 times. ([#616](https://github.com/odygrd/quill/issues/616))

- New macros `LOG_LEVEL_LIMIT_EVERY_N` have been added, allowing for count-based rate limiting and giving developers
  greater control over logging frequency. ([#616](https://github.com/odygrd/quill/issues/616))

- Renamed `PACKED` used in `libfmt` to `QUILLPACKED` to avoid naming
  collisions. ([#620](https://github.com/odygrd/quill/issues/620))

- The `set_thread_name` function has been fixed to provide accurate error reporting, ensuring that the correct error
  message is displayed in the event of a failure.

## v7.4.0

- Fixed a build issue when compiling with `-fno-rtti`. This ensures compatibility with projects that disable
  `RTTI`. ([#604](https://github.com/odygrd/quill/issues/604))
- Fixed an incorrectly triggered assertion in debug builds when `BackendOptions::log_timestamp_ordering_grace_period` is
  set to 0 ([#605](https://github.com/odygrd/quill/issues/605))
- Fixed a compile-time error in `CsvWriter` that occurred when passing a custom `FrontendOptions` type as a template
  parameter. ([#609](https://github.com/odygrd/quill/issues/609))
- Added accessors to `Logger` for sinks, user clock source, clock source type, and pattern formatter options that can be
  used to create another `Logger` with similar configuration.
- Added `ConsoleColours::ColourMode` to `ConsoleSink`, allowing colors to be explicitly forced or conditionally enabled
  based on the environment. Previously, colors were only conditionally
  enabled. ([#611](https://github.com/odygrd/quill/issues/611)).

  For example:
  ```cpp
    quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
      "sink_id_1", quill::ConsoleColours::ColourMode::Automatic);
   ```

## v7.3.0

- Added the option to explicitly specify the `Logger` used by the built-in `SignalHandler` for logging errors during
  application crashes. ([#590](https://github.com/odygrd/quill/issues/590))
- Prevented error logs from the `SignalHandler` from being output to CSV files when a `CsvWriter` is in
  use. ([#588](https://github.com/odygrd/quill/issues/588))
- Introduced `SignalHandlerOptions` to simplify and unify the API. `Backend::start_with_signal_handler` is now
  deprecated, replaced by a new `Backend::start` overload that accepts `SignalHandlerOptions` for enabling signal
  handling.
- Added a new `create_or_get_logger` overload that accepts a `std::vector<std::shared_ptr<Sink>>`, improving flexibility
  by allowing a variable number of sinks to be passed at runtime when creating a logger.
- Added a new overload to `create_or_get_logger` to create a logger that inherits configuration options from a specified
  logger. ([#596](https://github.com/odygrd/quill/issues/596))
- Implemented a workaround to resolve false positive warnings from `clang-tidy` on Windows.

## v7.2.2

- Fixed race condition during DLL unload by ensuring safe cleanup of `ThreadContext` when
  calling `flush_log()` ([#586](https://github.com/odygrd/quill/issues/586))

## v7.2.1

- Fixed an unused variable warning treated as an error on MSVC.

## v7.2.0

**Bug Fixes:**

- Fixed compile error in `BackendTscClock` ([#577](https://github.com/odygrd/quill/issues/577))
- Added a missing header include in `TriviallyCopyableCodec.h`. ([#560](https://github.com/odygrd/quill/issues/560))
- Fixed incorrect log level short codes introduced in v7 after adding the new log level `NOTICE`. Using
  `%(log_level_short_code)` in the pattern formatter could incorrectly map `LOG_ERROR` to `"C"` and LOG_WARNING
  to `"E"`. ([#564](https://github.com/odygrd/quill/issues/564))
- Fixed an overflow issue when logging more than `uint32_t::max()` bytes in a single log message. For example,
  attempting to log `std::string s(std::numeric_limits<uint32_t>::max(), 'a');` would previously cause a crash.

**Improvements:**

- Optimised dynamic log level handling and size calculation for fundamental types, `std::string` and `std::
  string_view` on the hot path.
- Several enhancements to the backend worker thread, resulting in an overall 10% backend throughput increase.
  Key optimizations include the simplification of `TransitEventBuffer`, reducing the memory footprint of `TransitEvent`,
  introducing support for custom buffer sizes in file streams and tuning `transit_events_soft_limit`
  and `transit_events_hard_limit` default values
- Improved readability of queue allocation notification messages. Capacities are now displayed in KiB,
  e.g.,
  `20:59:25 Quill INFO: Allocated a new SPSC queue with a capacity of 1024 KB (previously 512 KB) from thread 31158`.

**New Features:**

- Introduced support for custom buffer sizes in file streams for `FileSink` and `RotatingFileSink`. Buffer size is
  now configurable via `FileSinkConfig::set_write_buffer_size(size_value)` with a default of 64 KB.
- Added an optional `fsync` interval to control the minimum time between consecutive `fsync` calls, reducing disk wear
  from frequent fsync operations. This option is only applicable when `fsync` is
  enabled. ([#557](https://github.com/odygrd/quill/issues/557))
- Implemented support for appending a custom timestamp format to log filenames via `StartCustomTimestampFormat`.
  Example usage:
  ```cpp
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("logfile.log", []()
  {
    quill::FileSinkConfig cfg;
    cfg.set_filename_append_option(quill::FilenameAppendOption::StartCustomTimestampFormat, "%m%d");
    return cfg;
  }());
  ```
  This will create a log file named `logfile0919.log`, where `0919` represents the month and day.
- When using `%(named_args)` in the pattern formatter or logging in JSON format, extra
  arguments without key names are now included in JSON output with keys corresponding to their positional indexes.
  This allows additional details to be included in the JSON while keeping the log message clean. For
  example ([#563](https://github.com/odygrd/quill/discussions/563)):
  ```cpp
  LOG_INFO(hybrid_logger, "Operation {name} completed with code {code}", "Update", 123, "Data synced successfully");
  ```
  This will output:
  ```
  Operation Update completed with code 123
  ```
  And the corresponding JSON will be:
  ```
  {"timestamp":"1726582319816776867","file_name":"json_file_logging.cpp","line":"71","thread_id":"25462","logger":"hybrid_logger","log_level":"INFO","message":"Operation {name} completed with code {code}","name":"Update","code":"123","_2":"Data synced successfully"}
  ```

## v7.1.0

- Fixed crash when using `QueueType::BoundedDropping` or `QueueType::UnboundedDropping` after a message
  drops. ([#553](https://github.com/odygrd/quill/issues/553))
- Improved performance of `ForwardList` decoding.
- Corrected reported dropped message count; previously, log flush attempts were incorrectly included.
- Removed leftover files after running some unit tests.
- Stabilized regression tests.
- Suppressed false-positive `-Wstringop-overflow` warnings (e.g., with GCC 13).
- Fixed MinGW build and added MinGW builds to GitHub Actions.

## v7.0.0

- Simplified the log tags API. The `Tags` class has been removed. You now pass a `char const*` directly to the macros.
  Additionally, macros previously named `WITH_TAGS` have been renamed to `_TAGS`. For example, `LOG_INFO_WITH_TAGS` is
  now `LOG_INFO_TAGS`.
- Renamed `backend_cpu_affinity` to `cpu_affinity` in `BackendOptions` to improve consistency.
- Simplified project structure by removing the extra quill directory and made minor CMake improvements; `include/quill`
  is now directly in the root.
- Added support for `std::string` with custom allocator. ([#524](https://github.com/odygrd/quill/issues/524))
- Added a new log level `NOTICE`, for capturing significant events that aren't errors or warnings. It fits
  between `INFO` and `WARNING` for logging important runtime events that require
  attention. ([#526](https://github.com/odygrd/quill/pull/526))
- Enhanced static assert error message for unsupported codecs, providing clearer guidance for STL and user-defined
  types.
- Improved frontend performance by caching the `ThreadContext` pointer in `Logger` class to avoid repeated function
  calls. On Linux, this is now further optimised with `__thread` for thread-local storage, while other platforms still
  use `thread_local`.
- Minor performance enhancement in the frontend by replacing `std::vector<size_t>` with an `InlinedVector<uint32_t, 12>`
  for caching sizes (e.g. string arguments).
- Fixed order of evaluation for `Codec::pair<T1,T2>::compute_encoded_size()` to prevent side effects observed on MSVC
- Introduced the `add_metadata_to_multi_line_logs` option in `PatternFormatter`. This option, now enabled by default,
  appends metadata such as timestamps and log levels to every line of multiline log entries, ensuring consistent log
  output. To restore the previous behavior, set this option to false when creating a `Logger`
  using `Frontend::create_or_get_logger(...)`. Note that this option is ignored when logging JSON using named arguments
  in the format message. ([#534](https://github.com/odygrd/quill/pull/534))
- `JSON` sinks now automatically remove any `\n` characters from format messages, ensuring the emission of valid `JSON`
  messages even when `\n` is present in the format.
- Replaced `static` variables with `static constexpr` in the `ConsoleColours` class.
- Fixed compiler errors in a few rarely used macros. Added a comprehensive test for all macros to prevent similar issues
  in the future.
- Expanded terminal list for color detection in console applications on Linux
- Fixed an issue where `char*` and `char[]` types could be incorrectly selected by the Codec template in `Array.h`
- The library no longer defines `__STDC_WANT_LIB_EXT1__`, as the bounds-checking functions from the extensions are no
  longer needed.
- `StringFromTime` constructor no longer relies on the system's current time, improving performance in simulations where
  timestamps differ from system time. ([#541](https://github.com/odygrd/quill/issues/541))
- The `Frontend::create_or_get_logger(...)` function now accepts a `PatternFormatterOptions` parameter, simplifying the
  API. This is a breaking change. To migrate quickly, wrap the existing formatting parameters in a
  `PatternFormatterOptions` object.

  **Before:**
  ```c++
    quill::Logger* logger =
      quill::Frontend::create_or_get_logger("root", std::move(file_sink),
                                            "%(time) [%(thread_id)] %(short_source_location:<28) "
                                            "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                            "%H:%M:%S.%Qns", quill::Timezone::GmtTime);
  ```

  **After:**

  ```c++
    quill::Logger* logger =
      quill::Frontend::create_or_get_logger("root", std::move(file_sink), quill::PatternFormatterOptions {
                                            "%(time) [%(thread_id)] %(short_source_location:<28) "
                                            "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                            "%H:%M:%S.%Qns", quill::Timezone::GmtTime});
  ```

## v6.1.2

- Fix pkg-config file on windows

## v6.1.1

- Fix pkg-config file

## v6.1.0

- Fix various compiler warnings
- Minor serialisation improvements in `Array.h` and `Chrono.h`
- Introduced `Backend::acquire_manual_backend_worker()` as an advanced feature, enabling users to manage the backend
  worker on a custom thread. This feature is intended for advanced use cases where greater control over threading is
  required. ([#519](https://github.com/odygrd/quill/issues/519))
- Add new `CsvWriter` utility class for asynchronous CSV file writing. For example:
  ```c++
  #include "quill/Backend.h"
  #include "quill/core/FrontendOptions.h"
  #include "quill/CsvWriter.h"
  
  struct OrderCsvSchema
  {
    static constexpr char const* header = "order_id,symbol,quantity,price,side";
    static constexpr char const* format = "{},{},{},{:.2f},{}";
  };
  
  int main()
  {
    quill::BackendOptions backend_options;
    quill::Backend::start(backend_options);
    
    quill::CsvWriter<OrderCsvSchema, quill::FrontendOptions> csv_writer {"orders.csv"};
    csv_writer.append_row(13212123, "AAPL", 100, 210.32321, "BUY");
    csv_writer.append_row(132121123, "META", 300, 478.32321, "SELL");
    csv_writer.append_row(13212123, "AAPL", 120, 210.42321, "BUY");
  }
  ```

## v6.0.0

- Added a [Cheat Sheet](https://quillcpp.readthedocs.io/en/latest/cheat_sheet.html) to help users get the most out of
  the logging library

- Removed `ArgSizeCalculator<>`, `Encoder<>`, and `Decoder<>` classes. These have been consolidated into a
  single `Codec` class. Users who wish to pass user-defined objects should now specialize this single `Codec` class
  instead of managing three separate classes. For guidance, please refer to the updated advanced example

- Added `TriviallyCopyableCodec.h` to facilitate serialization for trivially copyable user-defined types. For example

  ```c++
    struct TCStruct
    {
      int a;
      double b;
      char c[12];
      
      friend std::ostream& operator<<(std::ostream& os, TCStruct const& arg)
      {
        os << "a: " << arg.a << ", b: " << arg.b << ", c: " << arg.c;
        return os;
      }
    };
    
    template <>
    struct fmtquill::formatter<TCStruct> : fmtquill::ostream_formatter
    {
    };
    
    template <>
    struct quill::Codec<TCStruct> : quill::TriviallyCopyableTypeCodec<TCStruct>
    {
    };
    
    int main()
    {
      // init code ...
      
      TCStruct tc;
      tc.a = 123;
      tc.b = 321;
      tc.c[0] = '\0';
      LOG_INFO(logger, "{}", tc);
    }
  ```

- Added support for passing arithmetic or enum c style arrays when `std/Array.h` is included. For example

  ```c++
    #include "quill/std/Array.h"
  
    int a[6] = {123, 456};
    LOG_INFO(logger, "a {}", a);
  ```

- Added support for `void const*` formatting. For example

  ```c++
      int a = 123;
      int* b = &a;
      LOG_INFO(logger, "{}", fmt::ptr(b));
  ```

- Added support for formatting `std::chrono::time_point` and `std::chrono::duration` with the inclusion
  of `quill/std/Chrono.h`

   ```c++
   #include "quill/std/Chrono.h"
  
   std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
   LOG_INFO(logger, "time is {}", now);
   ```

- Removed unused method from `ConsoleSink`

## v5.1.0

- Fix unit tests on FreeBSD ([#496](https://github.com/odygrd/quill/issues/496))
- Resolved unused variable warning on MSVC.
- Updated CMake to avoid adding `-fno-exceptions` to the entire target
  when `QUILL_NO_EXCEPTIONS=ON` ([#499](https://github.com/odygrd/quill/issues/499))
- Fix an issue where timestamps were incorrectly calculated when using `quill::Timezone::LocalTime`. This bug affected
  timezones that did not have an exact hour difference from UTC, leading to incorrect timestamp
  calculations. ([#498](https://github.com/odygrd/quill/issues/498))
- The newline character `\n` is now considered printable by default and will no longer be sanitized. Users can now
  include new lines in their logs directly. In versions `4.4.1` and earlier, `\n` was not sanitized, and this behavior
  is restored in this update, eliminating the need for a custom `check_printable_char` function in `BackendOptions`.
- On Windows, when colors are enabled in `ConsoleSink`, `GetConsoleScreenBufferInfo` may fail in the debug console.
  Previously, this would result in an error being displayed but no logs being written. This issue is now resolved: the
  error is reported once, and logs will be written to the console without colors.
- Improved performance of `StringFromTime` and `TimestampFormatter` used by the backend worker thread.
- Replaced `std::mutex` with a spinlock, resulting in minor performance improvement for backend worker. This change
  also avoids including `<mutex>` in the frontend, particularly when following the
  [recommended_usage](https://github.com/odygrd/quill/blob/master/examples/recommended_usage/recommended_usage.cpp)
  example
- Update bundled `libfmt` to `11.0.2`

## v5.0.0

- Fix build failure on Windows Arm64 ([#485](https://github.com/odygrd/quill/issues/485))
- Previously, wide string support was included in `Codec.h`. Wide string functionality has now been moved to a separate
  header file, `WideStrings.h`. On Windows, logging wide strings now requires the inclusion
  of `quill/std/WideStrings.h`.
- Added `QUILL_IMMEDIATE_FLUSH` preprocessor variable. This variable can be defined before including `LogMacros.h` or
  passed as a compiler flag. When `QUILL_IMMEDIATE_FLUSH` is defined, the library will flush the log on each log
  statement. This causes the caller thread to wait for the log to be processed and written to the log file by the
  backend thread before continuing, significantly impacting performance. This feature is useful for debugging the
  application when synchronized logs are required. ([#488](https://github.com/odygrd/quill/issues/488))
- Introduced `log_level_descriptions` and `log_level_short_codes` in `BackendOptions` to allow customization
  of `LogLevel` descriptions and short codes, replacing previously hardcoded values. This enhancement enables users to
  define their own descriptions and short codes for each log level. For instance, instead of displaying `LOG_WARNING`,
  it can now be configured to show `LOG_WARN`. ([#489](https://github.com/odygrd/quill/issues/489))

    ```c++  
    quill::BackendOptions backend_options;
    backend_options.log_level_descriptions[static_cast<uint32_t>(quill::LogLevel::Warning)] = "WARN";
    quill::Backend::start(backend_options);
    ```

- Introduced `LOGV_LEVEL`, `LOGV_LEVEL_LIMIT`, and `LOGV_LEVEL_WITH_TAGS` macros. These new macros simplify logging by
  automatically printing variable names and values without explicitly specifying each variable name or using `{}`
  placeholders in the format string. Each macro can handle up to 26 arguments. The format string is concatenated at
  compile time, there is no runtime overhead for using these macros. For example:

  ```c++
    int a = 123;
    double b = 3.17;
    LOGV_INFO(logger, "A message with two variables", a, b)
  ```
  outputs `A message with two variables [a: 123, b: 3.17]`

- Introduced `LOGJ_LEVEL`, `LOGJ_LEVEL_LIMIT`, and `LOGJ_LEVEL_WITH_TAGS` macros. These new macros simplify JSON logging
  by automatically embedding the name of each passed variable as a named argument in the format string. Each macro can
  handle up to 26 arguments. The format string is concatenated at compile time, there is no runtime overhead for using
  these macros. For example:

  ```c++
    int var_a = 123;
    std::string var_b = "test";
    LOGJ_INFO(logger, "A json message", var_a, var_b);
  ```
  outputs `{"log_level":"INFO","message":"A json message {var_a}, {var_b}","var_a":"123","var_b":"test"}`

- Enhanced the `filter` function to also receive the formatted `log_message` alongside the log_statement, enabling the
  comparison and filtering of `log_message` while disregarding elements like timestamps from the
  full `log_statement`. ([#493](https://github.com/odygrd/quill/issues/493))

- Renamed `log_message` to `log_statement` and `should_log_message` to `should_log_statement` in `Logger`
- Replaced `%(log_level_id)` with `%(log_level_short_code)` in the `PatternFormatter`.

- Fix a `CMakeLists` error for old `CMake` versions prior
  to `3.19`. ([#491](https://github.com/odygrd/quill/issues/491))

## v4.5.0

- The backend now automatically sanitizes non-printable characters in log messages by converting them to
  their hexadecimal representation. This feature ensures logs contain only safe, readable characters. You can customize
  or disable this feature through the backend options by modifying the `check_printable_char` callback
  in `BackendOptions`.

  ```c++  
  std::function<bool(char c)> check_printable_char = [](char c) { return c >= ' ' && c <= '~'; };
  ```

- Added `StringRef`, a utility for passing string arguments by reference without copying. Suitable for string literals
  or immutable strings with a guaranteed persistent lifetime. For example

  ```c++  
  #include "quill/StringRef.h"
  
  static constexpr std::string_view sv {"string_view"};
  LOG_INFO(logger, "{} {}", quill::utility::StringRef{sv}, quill::utility::StringRef{"string_literal"});
  ```

- Renamed `write_log_message` to `write_log` in `Sink`. The formatted `log_message` and `process_id` are now also
  provided. This enhancement supports use cases where the formatted `log_statement` passed to the `Sink` can be ignored
  and overwritten with a custom format, allowing a single `Logger` to output different formats to various Sinks.
  ([#476](https://github.com/odygrd/quill/issues/476))

- Fixed a bug in JSON logging where previously cached named arguments could erroneously append to subsequent log
  statements. ([#482](https://github.com/odygrd/quill/issues/482))

## v4.4.1

- Fixed multiple definitions of `quill::detail::get_error_message` ([#469](https://github.com/odygrd/quill/issues/469))
- Fixed an issue causing a `SIGABRT` when creating directories with a symlink folder path using GCC versions 8 or
  9 ([#468](https://github.com/odygrd/quill/issues/468))
- Added an assertion to prevent the use of custom `FrontendOptions` together with
  default `FrontendOptions` ([#453](https://github.com/odygrd/quill/issues/453))

## v4.4.0

- Introduced `log_timestamp_ordering_grace_period parameter`, replacing `enable_strict_log_timestamp_order` in
  `BackendOptions`. Enables strict timestamp ordering with configurable grace period.
- Fixed an issue where symbols were not properly exported with hidden visibility when compiling as a shared
  library. ([#463](https://github.com/odygrd/quill/issues/463))
- Move version info into quill namespace ([#465](https://github.com/odygrd/quill/issues/465))
- Upstreamed `Meson` build integration. See details [here](https://github.com/odygrd/quill?tab=readme-ov-file#meson)
- Upstreamed `Bazel` build integration. See details [here](https://github.com/odygrd/quill?tab=readme-ov-file#bazel)

## v4.3.0

- Refactored `BacktraceStorage` to simplify the code.
- Fixed multiple definitions of `on_alarm` in `SignalHandler.h`
- Fixed a bug in the backend thread where `flush()` and `run_periodic_tasks()` were skipped for certain sinks. All sinks
  are now correctly processed.

## v4.2.1

- Added `-Wno-gnu-zero-variadic-macro-arguments` as an interface compiler flag in CMake

## v4.2.0

- Fixed the compile-time exclusion of log levels. Renamed the `QUILL_COMPILE_OUT_LOG_LEVEL` preprocessor
  flag to `QUILL_COMPILE_ACTIVE_LOG_LEVEL`.
- Fixed build error when `UnboundedDropping` queue is used.
- Fixed a bug introduced in `v4.1.0`, which resulted in messages being logged out of order when
  the `transit_events_soft_limit` was reached. Additionally, this issue affected the behavior of `flush_log()`,
  prematurely unblocking the thread before all messages were flushed.
- Fixed `-Wno-unused-parameter` and `-Wdocumentation` warnings.
- Improved backend worker `_exit()` functionality and reduced code duplication in other areas of the backend worker
  code.
- Added `signal_handler_timeout_seconds` parameter, which controls the timeout duration for the signal handler. Only
  available on Linux platforms.
- Added `sleep_duration_ns` parameter to the `flush_log(...)` function. This parameter specifies the duration in
  nanoseconds to sleep between retries between checks for the flush completion and when a blocking queue is used,
  and it is full. The default sleep duration is 100 nanoseconds, but users can now customize this duration according to
  their needs. If a zero sleep duration is passed, the thread might yield instead.
- Removed uses of `std::this_thread::sleep_for(...)`, `std::string`, `std::vector` in the signal handler when waiting
  for
  the log to be flushed.

## v4.1.0

- Following the transition from a compiled to a header-only library, the `target_compile_options` previously applied to
  the compiled library were mistakenly propagated to all programs linking against the header-only library.
  This issue is now fixed by removing those flags and explicitly adding them to tests and examples. As a result,
  executable targets no longer inherit flags from the library.
- Removed unnecessary template specializations and merged their logic into the primary template
  for `ArgSizeCalculator`, `Encoder`, and `Decoder` using if constexpr.
- Eliminated `<functional>` header dependency in the frontend
- Replaced `%(structured_keys)` with `%(named_args)` in the `PatternFormatter`. This change now appends the
  entire key-value pair of named args to the message, not just the names.
- Relocated certain classes to the `detail` namespace
- Replaced `sprintf` with `snprintf` to fix macOS warning.
- Reviewed and removed gcc cold attribute from a few functions.
- Minor backend thread optimisations when logging c style strings or char arrays
- Improved backend thread variable and function names and fixed a bug for an edge case when the transit event hard limit
  is reached

## v4.0.0

This version represents a major revamp of the library, aiming to simplify and modernize it, resulting in the removal
of a few features. Please read through the changes carefully before upgrading, as it is not backwards compatible with
previous versions and some effort will be required to migrate.

I understand that these changes may inconvenience some existing users. However, they have been made with good
intentions, aiming to improve and refine the logging library. This involved significant effort and dedication.

Bug fixes and releases for `v3` will continue to be supported under the `v3.x.x` branch.

#### Comparison

- This version significantly improves compile times. Taking a look at some compiler profiling for a `Release` build with
  clang 15, we can see the difference. Below are the two compiler flamegraphs for building the `recommended_usage`
  example from the new version and the `wrapper_lib` example from the previous version.

The below flamegraph shows the difference in included headers between the two versions

| Version |                                                          Compiler FlameGraph                                                           |
|---------|:--------------------------------------------------------------------------------------------------------------------------------------:|
| v4.0.0  | ![quill_v4_compiler_profile.speedscope.png](https://github.com/odygrd/quill/blob/master/docs/quill_v4_compiler_profile.speedscope.png) |
| v3.8.0  | ![quill_v3_compiler_profile.speedscope.png](https://github.com/odygrd/quill/blob/master/docs/quill_v3_compiler_profile.speedscope.png) |

A new compiler benchmark has been introduced. A Python script generates 2000 distinct log statements with various
arguments. You can find the
benchmark [here](https://github.com/odygrd/quill/blob/master/benchmarks/compile_time/compile_time_bench.cpp).
Compilation now takes only about 30 seconds, whereas the previous version required over 4 minutes.

| Version |                                                         Compiler FlameGraph                                                          |
|---------|:------------------------------------------------------------------------------------------------------------------------------------:|
| v4.0.0  |  ![quill_v4_compiler_bench.speedscope.png](https://github.com/odygrd/quill/blob/master/docs/quill_v4_compiler_bench.speedscope.png)  |
| v3.8.0  | ![quill_v3_compiler_bench.speedscope.png](https://github.com/odygrd/quill/blob/master/docs/quill_v4_compiler_profile.speedscope.png) |

- Minor increase in backend thread throughput compared to the previous version.

| Version |                                 Backend Throughput                                 |
|---------|:----------------------------------------------------------------------------------:|
| v4.0.0  | 4.56 million msgs/sec average, total time elapsed: 876 ms for 4000000 log messages |
| v3.8.0  | 4.39 million msgs/sec average, total time elapsed: 910 ms for 4000000 log messages |

- Significant boost in hot path latency when logging complex types such as `std::vector`.
  The performance remains consistent when logging only primitive types or strings in both versions. Refer
  [here](https://github.com/odygrd/quill?tab=readme-ov-file#performance) for updated and detailed benchmarks.

#### Changes

- **Improved compile times**

The library has been restructured to minimize the number of required headers. Refactoring efforts have focused on
decoupling the frontend from the backend, resulting in reduced dependencies. Accessing the frontend logging functions
now does not demand inclusion of any backend logic components.

     "quill/Backend.h" - It can be included once to start the backend logging thread, typically in main.cpp 
                         or in a wrapper library.
     
     "quill/Frontend.h"` - Used to create or obtain a `Logger*` or a `Sink`. It can be included in limited 
                           files, since an obtained `Logger*` has pointer stability and can be passed around.
     
     "quill/Logger.h", "quill/LogMacros.h" - These two files are the only ones needed for logging and will have 
                                             to be included in every file that requires logging functionality.

- **Backend formatting for user-defined and standard library types**

One of the significant changes lies in the support for formatting both user-defined and standard library types.
Previously, the backend thread handled the formatting of these types sent by the frontend. It involved making a copy for
any object passed to the `LOG_` macros as an argument using the copy constructor of a complex type instead of directly
serializing the data to the SPSC queue. While this method facilitated logging copy-constructible user-defined types with
ease, it also posed numerous challenges for asynchronous logging:

- Error-Prone Asynchronous Logging: Copying and formatting user-defined types on the backend thread in an
  asynchronous logging setup could lead to errors. Previous versions attempted to address this issue with type
  trait checks, which incurred additional template instantiations and compile times.
- Uncertainty in Type Verification: It is challenging to confidently verify types, as some trivially copiable
  types, such as `struct A { int* m; }`, could still lead to issues due to potential modifications by the user
  before formatting.
- Hidden Performance Penalties: Logging non-trivially copiable types could introduce hidden cache coherence
  performance penalties due to memory allocations and deallocations across threads. For instance,
  consider `std::vector<int>` passed as a log argument. The vector is emplaced into the SPSC queue by the frontend,
  invoking the copy constructor dynamically allocating memory as the only members copied to SPSC queue
  are `size`, `capacity`, and `data*`. The backend thread reads the object, formats it, and then invokes the destructor,
  which in turn synchronizes the
  freed memory back to the frontend.

Additionally, after years of professional use and based on experience, it has been observed that user-defined types
are often logged during program initialization, with fewer occurrences on the hot path where mostly built-in types are
logged. In such scenarios, the overhead of string formatting on the frontend during initialization is not an issue.

In this new version, the use of the copy constructor for emplacing objects in the queue has been abandoned. Only POD
types are copied, ensuring that only raw, tangible data is handled without any underlying pointers pointing to other
memory locations. The only exception to this are the pointers to `Metadata`, `LoggerBase` and `DecodeFunction`
that are passed internally for each log message. Log arguments sent from the frontend must undergo
serialization beforehand. While this approach resolves the above issues, it does introduce more complexity when
dealing with user-defined or standard library types.

Built-in types and strings are logged by default, with the formatting being offloaded to the backend. Additionally,
there is built-in support for most standard library types, which can also be directly passed to the logger by
including the relevant header from `quill/std`.

The recommendation for user-defined types is to format them into strings before passing them to the `LOG_` macros using
your preferred method. You can find an example of
this [here](https://github.com/odygrd/quill/blob/master/examples/user_defined_types_logging.cpp).

It's also possible to extend the library by providing template specializations to serialize the user-defined types
and offload their formatting to the backend. However, this approach should only be pursued if you cannot tolerate the
formatting overhead in that part of your program. For further guidance, refer
to [this example](https://github.com/odygrd/quill/blob/master/examples/advanced/advanced.cpp).

- **Header-Only library**

The library is now header-only. This change simplifies exporting the library as a C++ module in the future. See
[here](https://github.com/odygrd/quill/blob/master/examples/recommended_usage/recommended_usage.cpp) on how to build a
wrapper static library which includes the backend and will minimise the compile times.

- **Preprocessor flags moved to template parameters**

Most preprocessor flags have been moved to template parameters, with only a few remaining as `CMake` options. This
change simplifies exporting the library as a C++ module in the future.

- **Renamed Handlers to Sinks**

To enhance clarity, handlers have been renamed to sinks.

- **PatternFormatter moved to Logger**

The `PatternFormatter` has been relocated from `Sink` to `Logger`, enabling a logger object to log in a specific
format. This allows for different formats within the same output file, a feature not previously possible.

- **Split Configuration**

The configuration settings have been divided into `FrontendOptions` and `BackendOptions`.

- **Refactoring of backend classes**

`MacroMetadata` and many backend classes have undergone refactoring, resulting in reduced memory requirements.

- **Improved wide strings handling on Windows**

The library now offers significant performance enhancements for handling wide strings on Windows platforms.
It's important to note that only wide strings containing ASCII characters are supported. Previously, wide strings were
converted to narrow strings at the frontend, impacting the critical path of the application.
With this update, the underlying wide char buffer is copied and the conversion to UTF-8 encoding is deferred to
the backend logging thread. Additionally, this update adds support for logging STL containers consisting of
wide strings

- **Default logger removal**

The default logger, along with the configuration inheritance feature during logger creation, has been removed. Now, when
creating a new logger instance, configurations such as the `Sink` and log pattern format must be explicitly specified
each time. This simplifies the codebase.

- **Global logger removal**

The static global logger* variable that was initialised during `quill::start()` used to obtain the default logger has
been removed. It is possible to add this on the user side. If you require a global logger you can have a look
at [this example](https://github.com/odygrd/quill/blob/master/examples/recommended_usage/recommended_usage.cpp)

- **Removal of printf style formatting support**

The support for `printf` style formatting has been removed due to its limited usage and the increased complexity. Users
requiring this feature should stay on `v3.x.x` versions to maintain compatibility.

- **Removal of external libfmt usage**

The option to build the library with external `libfmt` has been removed. It becomes difficult to maintain and backwards
support previous versions of `libfmt`. Instead, `libfmt` is now an internal component of the library, accessible under
the namespace `fmtquill`. You can use the bundled version of `fmtquill` by including the necessary headers from
`quill/bundled/fmt`. Alternatively, you have the freedom to integrate your own version. Since `libfmt` is encapsulated
within a distinct namespace, there are no conflicts even if you link your own `libfmt` alongside the logging library.

#### Migration Guidance

- Revise include files to accommodate the removal of `Quill.h`
- Update the code that starts the backend thread and the logger/sink creation. You can refer to any of the
  updated examples, such as [this one](https://github.com/odygrd/quill/blob/master/examples/file_logging.cpp)
- When logging statements involving user-defined types, make sure these types are formatted into strings using
  your preferred method. Refer to
  [this link](https://github.com/odygrd/quill/blob/master/examples/user_defined_types_logging.cpp) for guidance.
  Alternatively, if you prefer delaying the conversion to strings until the backend thread and only passing a
  binary copy of the user-defined type on the hot path, you can provide the necessary class template
  specializations for each user-defined type. See an example
  [here](https://github.com/odygrd/quill/blob/master/examples/advanced/user_quill_codec.h)

## v3.9.0

- Fix bug in `ConsoleHandler` when dynamic log level is used ([#421](https://github.com/odygrd/quill/pull/421))
- Fix bug in `TransitEvent` when dynamic log level is used ([#427](https://github.com/odygrd/quill/pull/427))
- Fix build error for Intel compiler classic ([#414](https://github.com/odygrd/quill/pull/414))
- Added `JsonConsoleHandler` ([#413](https://github.com/odygrd/quill/issues/413))
- Fix fold expression argument evaluation. This bug could occur when logging c style strings

## v3.8.0

- Refactored `MacroMetadata` class to reduce its size.
- Renamed some attributes in the `PatternFormatter` class for clarity. If you are using a custom format pattern, update
  the attribute names in your code to match the new names.
- Improved accuracy of log statement timestamps. Previously, the timestamp was taken after checking if the queue had
  enough space to push the message, which could make it less accurate. Additionally, in the case of a blocking queue,
  the timestamp could be later in time. Now, the timestamp is taken and stored right after the log statement is issued,
  before checking for the queue size.
- Reduced template instantiations during logging operations on the hot path. Fold expressions are now used for
  encoding/decoding arguments, minimizing template recursion overhead.
- Removed compile-time format checks due to their significant impact on template instantiations, especially considering
  that only a few cases are invalid. For instance, while `fmt::format("{}", 1, 2)` is considered valid,
  `fmt::format("{} {}", 1)` is deemed invalid. In cases where an invalid format string is detected, the backend worker
  thread catches the generated exception and logs an error.
- The throughput of the backend worker thread has been improved by approximately 5%. This enhancement is reflected in
  the new throughput value of 4.20 million msgs/sec, compared to the previous throughput of 3.98 million msgs/sec.
- Detect `tmux` as colour terminal. ([#410](https://github.com/odygrd/quill/issues/410))

## v3.7.0

- Fixed crash triggered by insufficient space in the queue upon invocation
  of ``flush()``. ([#398](https://github.com/odygrd/quill/pull/398))
- Fixed windows clang-cl build error. ([#400](https://github.com/odygrd/quill/pull/400))
- Fixed compilation errors encountered on FreeBSD and extended ``get_thread_id()`` support to various other BSD
  operating systems. ([#401](https://github.com/odygrd/quill/pull/401))
- Fix open_file in the FileHandler to also create the parent path before opening the
  file. ([#395](https://github.com/odygrd/quill/issues/395))
- Enhance logic for backend thread's flush() invocation; it now triggers only if the handler has previously written
  data. ([#395](https://github.com/odygrd/quill/issues/395))
- Address an uncaught exception in the backend thread that could occur when a user manually removes the log file from
  the terminal while the logger is running. ([#395](https://github.com/odygrd/quill/issues/395))
- Ensure that after a logger is removed, there are no subsequent calls to the Handler's flush() or run_loop(), provided
  the Handler is not shared. ([#395](https://github.com/odygrd/quill/issues/395))
- Ignore the virtual destructor missing warning for the `CustomTags`
  class. ([#402](https://github.com/odygrd/quill/pull/402))
- Update bundled `libfmt` to `v10.2.1`

## v3.6.0

- Fixed `QUILL_LOGGER_CALL_NOFN_LIMIT` macros. ([#381](https://github.com/odygrd/quill/pull/381))
- Resolved a bug that caused reading destructed arguments when structured logging format was used.
- Modified member access from `private` to `protected` in `ConsoleHandler` for potential inheritance purposes.
- Eliminated redundant whitespaces within `JsonFileHandler`.
- Fixed `JsonFileHandler` to notify the file event notifier before log message writes.
- Implemented a new attribute called `%(structured_keys)` within the `PatternFormatter` to facilitate the inclusion
  of keys in messages when using structured log formatting. This addition is useful for instances where logging occurs
  in both JSON and regular log formats, enabling the display of keys within the regular log-formatted messages.
  See
  updated [example_json_structured_log.cpp](https://github.com/odygrd/quill/blob/master/examples/example_json_structured_log.cpp)

## v3.5.1

- Resolved issue with accessing the `name()` method within the `Logger`
  class. ([#378](https://github.com/odygrd/quill/pull/378))
- Fixed a compilation error in `SignalHandler` specific to Windows when `QUILL_DISABLE_NON_PREFIXED_MACROS` is
  defined. ([#380](https://github.com/odygrd/quill/pull/380))

## v3.5.0

- Fixed `LOG_TRACE_CFORMAT` macros.
- Added support for compile-time custom tags in `quill::MacroMetadata` to enhance message filtering and incorporate
  static information. New log macros suffixed with `_WITH_TAGS` introduced for this feature.
  Additionally, `%(custom_tags)` parameter added
  to `PatternFormatter`. ([#349](https://github.com/odygrd/quill/issues/349))
  See [example_custom_tags.cpp](https://github.com/odygrd/quill/blob/master/examples/example_custom_tags.cpp)
- Improvements to reduce compilation time

## v3.4.1

- Reduce backend worker unnecessary allocation. ([#368](https://github.com/odygrd/quill/issues/368))
- Adjusted handling for empty `std::string_view` instances, addressing an issue where logging empty strings triggered an
  unintended `memcpy` with zero size and a nullptr, leading to address sanitizer warnings.
- Fix clang build error when using `-DQUILL_NO_EXCEPTIONS:BOOL=ON`. ([#357](https://github.com/odygrd/quill/issues/357))

## v3.4.0

- Resolved `bad_variant_access` error occurring when using Quill as a pre-compiled library with a distinct queue
  type. ([#276](https://github.com/odygrd/quill/pull/276))

- Resolved a bug in `RotatingFileHandler` associated with logfiles located outside the working directory,
  specifically when used with open_mode `a`. ([#340](https://github.com/odygrd/quill/pull/340))

- Added a `name()` method to the Logger class which provides the logger
  name. ([#345](https://github.com/odygrd/quill/pull/345))

- Fixed library and include paths in the pkg-config configuration. ([#352](https://github.com/odygrd/quill/pull/352))

- Move `get_root_logger()` definition from cpp to the header file ([#348](https://github.com/odygrd/quill/issues/348))

- Introduced support for logging character arrays. You can now log character arrays, even when they don't contain a
  null-terminating character.
  Additionally, character arrays with null characters in the middle are supported, and the logger will
  capture the content until the null character is encountered. ([#353](https://github.com/odygrd/quill/pull/353))

  For example

  ```c++
      union
    {
      char no_0[2];
      char mid_0[6]{'1', '2', '3', '4', '\0', 6};
    } char_arrays;

    // only output "12" even if there's no '\0' at the end
    LOG_INFO(logger, R"(This is a log info example for char array without '\0': {})", char_arrays.no_0);

    // output "1234" until the '\0'
    LOG_INFO(logger, R"(This is a log info example for char array with '\0' in middle: {})",
             char_arrays.mid_0);
  ```

- Minor improvements in the bounded queue and throughput. ([#362](https://github.com/odygrd/quill/pull/362))

  Previous: 2.21 million msgs/sec average, total time elapsed: 1809 ms for 4000000 log messages.

  New:      2.24 million msgs/sec average, total time elapsed: 1787 ms for 4000000 log messages.

- Disable `fmt::join(data, "")` at compile time. ([#356](https://github.com/odygrd/quill/issues/356))
- Fix compile error in Apple Clang 12. ([#360](https://github.com/odygrd/quill/issues/360))
- Add guards for redefined preprocessor variables.
- Fix `uint64_t` to `time_t` implicit conversion error in Clang 18.
- Update bundled `libfmt` to `v10.1.1`

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

- Fixed a race condition and potential crash in `quill::remove_logger(Logger*)` when called without
  prior `quill::flush()`.

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
  resulting in optimised memory allocation.

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
