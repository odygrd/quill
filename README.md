# Quill

#### Low Latency Asynchronous Logging Library

[![language][badge.language]][language]
[![license][badge.license]][license]

[badge.language]: https://img.shields.io/badge/language-C%2B%2B14-red.svg
[badge.license]: https://img.shields.io/badge/license-MIT-blue.svg

[language]: https://en.wikipedia.org/wiki/C%2B%2B14
[license]: https://en.wikipedia.org/wiki/MIT_License

## Install

## Platforms
 * Linux
 * Windows
 * macOS
 
 ## Package managers
 
 ## Features
 * Python style formatting by the excellent [{fmt}](https://github.com/fmtlib/fmt) library
 * Custom LogRecord formatting, with attributes similar to python logging (https://docs.python.org/3/library/logging.html)
 * Various log targets (Handlers)
    * Console logging 
    * Rotating log files [Work in progress]
    * Daily log files [Work in progress]
 * Multiple thread-safe Loggers

## Design
Quill focuses on low latency, not high throughput. The main priority is set on reducing the latency on caller threads as much as possible. There is only one backend consumer thread responsible for log output

## Usage samples

#### Basic usage
