- [v1.2.0](#v1.2.0)
- [v1.1.0](#v1.1.0)
- [v1.0.0](#v1.0.0)

## v1.2.0
* Linking and including an external version of `fmt` is now supported. See TweakMe.h
* Fixed compiler warnings when using clang's `-Wdocumentation`. ([#12](https://github.com/odygrd/quill/issues/12))
* Fixed a bug that wouldn't report a compile-time error for invalid format strings. ([#13](https://github.com/odygrd/quill/issues/13))
* Added process ID to Formatter. ([#14](https://github.com/odygrd/quill/issues/14))
* Enhanced timestamp formatting. The `timestamp_format` string passed in `handler->set_pattern(format_pattern, timestamp_format, timezone)` now accepts three additional specifiers `%Qms`, `%Qus`, `%Qus` that can be used to format the fractional seconds. See [here](https://github.com/odygrd/quill/wiki/3.-Formatters). ([#15](https://github.com/odygrd/quill/issues/15))

## v1.1.0
* Daily file handler. The file handler rollover every 24 hours
* Rotating file handler. The file handler will rollover based on the size of the file
* MinGW compatibility
* Added a CMake option `QUILL_VERBOSE_MAKEFILE`. Building Quill as a master project now defaults to non verbose makefile output unless `-DQUILL_VERBOSE_MAKEFILE=ON` is passed to CMake. ([#6](https://github.com/odygrd/quill/issues/6))
* Flush policy improvement. Previously Quill backend worker thread would never `flush`. This made watching the live log of the application harder because the user has to wait for the operating system to flush or `quill::flush()` had to be called on the caller threads. This has now been fixed, when the backend thread worker has no more log messages to process it will automatically `flush`. ([#8](https://github.com/odygrd/quill/issues/8))
* The log level names have been changed from `"LOG_INFO"`, `"LOG_DEBUG"`, etc to `"INFO"`, `"DEBUG"`, etc .. The default formatter string is now using `"LOG_"%(level_name)` instead of `%(level_name)` therefore there is now change in the behaviour. This change gives a lot of more flexibility to users who prefer to see e.g. `INFO` instead of `LOG_INFO` in the logs. ([#7](https://github.com/odygrd/quill/issues/7))
* An option has been added to append the date to the filename when using a FileHandler `quill::file_handler(filename, mode, FilenameAppend);`. ([#7](https://github.com/odygrd/quill/issues/7))
* It is now possible to specify the timezone of each handler timestamp. A new parameter is added to `file_handler->set_pattern(...)`. See `PatternFormatter::Timezone`. ([#7](https://github.com/odygrd/quill/issues/7))
* Rename `emit` as it can confict with Qt macros. ([#4](https://github.com/odygrd/quill/issues/4))
* Upgraded `libfmt` to `6.2.0`.

## v1.0.0
* Initial release.
* Using `libfmt` to `6.1.2`.
