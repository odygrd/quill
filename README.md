<div align="center">
  <!--
  <meta name="description" content="C++ logging library. High-performance, asynchronous logging for low-latency real-time applications.">
  <meta name="keywords" content="C++, logging library, asynchronous logging, high performance, low latency">
  -->

  <h1>
    <img src="docs/quill_logo.png" alt="Quill C++ Logging Library" width="100" /><br>
    Quill
  </h1>
  <p><b>Ultra-Low-Latency Asynchronous C++17 Logging and Metrics Library</b></p>

  <div>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Afedora">
      <img src="https://img.shields.io/github/actions/workflow/status/odygrd/quill/fedora.yml?branch=master&label=Fedora&style=flat-square&logo=fedora" alt="fedora-ci" />
    </a>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Aubuntu">
      <img src="https://img.shields.io/github/actions/workflow/status/odygrd/quill/ubuntu.yml?branch=master&label=Ubuntu&style=flat-square&logo=ubuntu" alt="ubuntu-ci" />
    </a>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Absd">
      <img src="https://img.shields.io/github/actions/workflow/status/odygrd/quill/bsd.yml?branch=master&label=BSD&style=flat-square&logo=openbsd" alt="bsd-ci" />
    </a>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Amacos">
      <img src="https://img.shields.io/github/actions/workflow/status/odygrd/quill/macos.yml?branch=master&label=macOS&logoColor=white&style=flat-square&logo=apple" alt="macos-ci" />
    </a>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Awindows">
      <img src="https://img.shields.io/github/actions/workflow/status/odygrd/quill/windows.yml?branch=master&label=Windows&logoColor=blue&style=flat-square&logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIGhlaWdodD0iODgiIHdpZHRoPSI4OCIgeG1sbnM6dj0iaHR0cHM6Ly92ZWN0YS5pby9uYW5vIj48cGF0aCBkPSJNMCAxMi40MDJsMzUuNjg3LTQuODYuMDE2IDM0LjQyMy0zNS42Ny4yMDN6bTM1LjY3IDMzLjUyOWwuMDI4IDM0LjQ1M0wuMDI4IDc1LjQ4LjAyNiA0NS43em00LjMyNi0zOS4wMjVMODcuMzE0IDB2NDEuNTI3bC00Ny4zMTguMzc2em00Ny4zMjkgMzkuMzQ5bC0uMDExIDQxLjM0LTQ3LjMxOC02LjY3OC0uMDY2LTM0LjczOXoiIGZpbGw9IiMwMGFkZWYiLz48L3N2Zz4=" alt="windows-ci" />
    </a>
    <a href="https://github.com/odygrd/quill/actions?query=workflow%3Aintelllvm">
      <img src="https://img.shields.io/github/actions/workflow/status/odygrd/quill/intelllvm.yml?branch=master&label=Intel%20LLVM&style=flat-square&logo=intel" alt="intel-llvm-ci" />
    </a>
  </div>

  <div>
    <a href="https://codecov.io/gh/odygrd/quill">
      <img src="https://img.shields.io/codecov/c/gh/odygrd/quill/master.svg?logo=codecov&style=flat-square" alt="Codecov" />
    </a>
    <a href="https://github.com/odygrd/quill/actions/workflow/status/odygrd/quill/fuzz.yml">
      <img src="https://img.shields.io/github/actions/workflow/status/odygrd/quill/fuzz.yml?branch=master&label=Fuzz&style=flat-square" alt="fuzz-ci" />
    </a>
    <a href="https://app.codacy.com/gh/odygrd/quill/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade">
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
      <img src="https://img.shields.io/badge/language-C%2B%2B17-red.svg?style=flat-square" alt="language" />
    </a>
  </div>

  <h4>
    <a href="https://godbolt.org/z/n68asK7bY" title="Try Quill live on Compiler Explorer">🔬 Try It Online</a>
    <span> · </span>
    <a href="#-performance" title="Benchmark methodology and results">🚀 Benchmarks</a>
    <span> · </span>
    <a href="https://quillcpp.readthedocs.io" title="Explore the full documentation">📚 Documentation</a>
    <span> · </span>
    <a href="https://quillcpp.readthedocs.io/en/latest/recipes.html" title="Quick reference for common tasks">⚡ Recipes</a>
    <span> · </span>
    <a href="https://quillcpp.readthedocs.io/en/latest/faq.html" title="Frequently asked questions">❓ FAQ</a>
    <br />
    <a href="https://github.com/odygrd/quill/issues/new?assignees=&labels=&projects=&template=bug-report.md&title=" title="Report a bug or issue">🐛 Report Bug</a>
    <span> · </span>
    <a href="https://github.com/odygrd/quill/issues/new?assignees=&labels=&projects=&template=feature_request.md&title=">💡 Request Feature</a>
  </h4>

  <div align="center"><img src="docs/quill_demo.gif" alt="Logging Demo" width="75%" /></div>
</div>

---

## 🧭 Table of Contents

- [Introduction](#-introduction)
- [Quick Start](#-quick-start)
- [Features](#-features)
- [Performance](#-performance)
- [Usage](#-usage)
- [Design](#-design)
- [Caveats](#-caveats)
- [License](#-license)

---

## ✨ Introduction

**Quill** is an asynchronous logging and metrics library for **C++17 and later**. It keeps formatting and I/O away from
latency-sensitive application threads by encoding log arguments on the frontend and processing them on a dedicated
backend worker.

- **Low Frontend Latency**: Log arguments are encoded and queued for asynchronous processing, minimizing work on the
  calling thread. See the [latency benchmarks](#latency) for measured results and methodology.
- **Deferred Formatting**: Expensive formatting is performed by the backend worker instead of the calling thread.
- **Logging and Metrics**: Publish pre-registered metrics through the same asynchronous backend. The bundled Prometheus
  sink handles common metric types, while custom sinks can route samples to StatsD, OpenTelemetry, or other collectors.
  See the
  [Metrics guide](https://quillcpp.readthedocs.io/en/latest/metrics.html).
- **Highly Customizable**: Tune frontend queues and memory policy at compile time; configure backend idle behaviour,
  CPU affinity, buffering, timestamp handling, flushing, and callbacks at runtime; and compose loggers from built-in or
  custom sinks with per-sink filters. See
  [Frontend Options](https://quillcpp.readthedocs.io/en/latest/frontend_options.html),
  [Backend Options](https://quillcpp.readthedocs.io/en/latest/backend_options.html), and
  [Sinks](https://quillcpp.readthedocs.io/en/latest/sinks.html).
- **Production-Focused Testing**: Continuously tested across Linux, macOS, Windows, and BSD, with sanitizers and fuzzing.

> Using Quill? Click **Star** at the top of the [GitHub repository](https://github.com/odygrd/quill) to help other C++
> developers discover it.

---

## ⏩ Quick Start

Getting started is easy and straightforward. Follow these steps to integrate the library into your project:

### Installation

You can install Quill using the package manager of your choice:

| Package Manager |              Installation Command              |
|:---------------:|:----------------------------------------------:|
|      vcpkg      |             `vcpkg install quill`              |
|      Conan      |             `conan install quill`              |
|    Homebrew     |              `brew install quill`              |
|  Meson WrapDB   |           `meson wrap install quill`           |
|      Conda      |      `conda install -c conda-forge quill`      |
|     Bzlmod      | `bazel_dep(name = "quill", version = "x.y.z")` |
|      xmake      |             `xrepo install quill`              |
|       nix       |            `nix-shell -p quill-log`            |
|     build2      |                   `libquill`                   |

### Setup

#### Quickest Setup

For the shortest path from zero to working logs, use `simple_logger()`:

```c++
#include "quill/SimpleSetup.h"
#include "quill/LogMacros.h"

int main()
{
  // log to the console
  auto* logger = quill::simple_logger();
  LOG_INFO(logger, "Hello from {}!", "Quill");

  // log to a file
  auto* logger2 = quill::simple_logger("test.log");
  LOG_WARNING(logger2, "This message goes to a file");
}
```

**Console output:**

```
20:07:18.423476231 [48917] main.cpp:8                    LOG_INFO      Hello from Quill!
```

#### Detailed Setup

If you want explicit control over backend options, logger names, sinks, or formatters, use the
`Backend` and `Frontend` APIs directly:

```c++
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include <string_view>

int main()
{
  quill::Backend::start();

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

  LOG_INFO(logger, "Hello from {}!", std::string_view{"Quill"});
}
```

**Output:**

```
20:07:18.423476231 [48917] main.cpp:15                   LOG_INFO      root         Hello from Quill!
```

You can also use the macro-free mode. The macro API (`LOG_INFO`) is the lowest-latency path.
The function API (`quill::info`) reads more like ordinary code but is slightly slower.
See [here](https://quillcpp.readthedocs.io/en/latest/macro_free_mode.html) for the trade-offs.

```c++
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include <string_view>

int main()
{
  quill::Backend::start();

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

  quill::info(logger, "Hello from {}!", std::string_view{"Quill"});
}
```

### Publishing Metrics

Register `MetricMetadata` once, then publish `double` samples from hot threads through the same asynchronous backend
used for logs. The bundled `PrometheusSink` handles counters, gauges, histograms, and summaries; custom sinks can route
samples to StatsD, OpenTelemetry, or any in-process collector via `Sink::write_metric()`.

```c++
// One-time registration — returns a stable pointer valid for program lifetime.
quill::MetricMetadata const* requests_total = quill::Frontend::create_metric(
  "requests_total_post_200", "requests_total", {{"method", "POST"}, {"status", "200"}});

// Hot path — no label serialization, just a pointer and a double.
logger->publish_metric(requests_total, 1.0);
```

See the [Metrics guide](https://quillcpp.readthedocs.io/en/latest/metrics.html) for sink setup, custom sinks, and
Prometheus integration.

---

## 🎯 Features

- **High-Performance**: Ultra-low latency performance.
- **Asynchronous Processing**: Background thread handles formatting and I/O, keeping your main thread responsive.
- **Metric Publishing**: Publish pre-registered metric samples to Prometheus, StatsD, OpenTelemetry, or any in-process
  collector through the same asynchronous backend.
  See [Metrics](https://quillcpp.readthedocs.io/en/latest/metrics.html).
- **Minimal Header Includes**:
  - **Frontend**: Only `Logger.h` and `LogMacros.h` needed for logging. Lightweight with minimal dependencies.
  - **Backend**: Single `.cpp` file inclusion. No backend code injection into other translation units.
- **Compile-Time Optimization**: Eliminate specific log levels at compile time.
- **Custom Formatters**: Define your own log output patterns.
  See [Formatters](https://quillcpp.readthedocs.io/en/latest/formatters.html).
- **Cross-Thread Timestamp Handling**: The backend compares available events across frontend queues by timestamp, with
  a configurable grace window for delayed producers and optional sink-visible monotonic timestamp correction.
  See [Timestamp Types](https://quillcpp.readthedocs.io/en/latest/timestamp_types.html).
- **Flexible Timestamps**: Support for `rdtsc`, `chrono`, or `custom clocks` - ideal for simulations and more.
- **Backtrace Logging**: Store messages in a ring buffer for on-demand display.
  See [Backtrace Logging](https://quillcpp.readthedocs.io/en/latest/backtrace_logging.html)
- **Multiple Output Sinks**: Console (with color), files (with rotation), JSON, ability to create custom sinks and more.
- **Log Filtering**: Process only relevant messages.
  See [Filters](https://quillcpp.readthedocs.io/en/latest/filters.html).
- **JSON Logging**: Structured log output.
  See [JSON Logging](https://quillcpp.readthedocs.io/en/latest/json_logging.html)
- **Mapped Diagnostic Context (MDC)**: Thread-local key/value context attached automatically to subsequent log lines.
  See [MDC](https://quillcpp.readthedocs.io/en/latest/mdc.html).
- **Rate-Limited Macros**: `LOG_*_LIMIT` / `LOGV_*_LIMIT` emit at most once per configured interval per call site.
- **Configurable Queue Modes**: `bounded/unbounded` and `blocking/dropping` options with monitoring on dropped messages,
  queue reallocations, and blocked hot threads.
- **Crash Handling**: Built-in signal handler for log preservation during crashes.
- **Huge Pages Support (Linux)**: Leverage huge pages on the hot path for optimized performance.
- **Wide Character Support (Windows)**: Logs wide strings by converting them to UTF-8 on the backend, with support for
  STL containers consisting of wide strings.
- **Exception-Free Option**: Configurable builds with or without exception handling.
- **Clean Codebase**: Maintained to high standards, warning-free even at strict levels.
- **Type-Safe API**: Built on [{fmt}](https://github.com/fmtlib/fmt) library.

---

## 🚀 Performance

### System Configuration

- **Quill Version:** v13.0.0
- **OS:** Linux RHEL 9.4
- **CPU:** Intel Core i5-12600 (12th Gen) @ 4.8 GHz
- **Compiler:** GCC 14.2
- **Build:** Release with `-march=x86-64-v3`
- **Benchmark-Tuned System:** The system is specifically tuned for benchmarking.

- **Command Line Parameters:**
  ```shell
  $ cat /proc/cmdline
  BOOT_IMAGE=(hd0,gpt2)/vmlinuz-5.14.0-427.13.1.el9_4.x86_64 root=/dev/mapper/rhel-root ro crashkernel=1G-4G:192M,4G-64G:256M,64G-:512M resume=/dev/mapper/rhel-swap rd.lvm.lv=rhel/root rd.lvm.lv=rhel/swap rhgb quiet nohz=on nohz_full=1-5 rcu_nocbs=1-5 isolcpus=1-5 mitigations=off transparent_hugepage=never intel_pstate=disable nosoftlockup irqaffinity=0 processor.max_cstate=1 nosoftirqd sched_tick_offload=0 spec_store_bypass_disable=off spectre_v2=off iommu=pt
  ```

You can find the benchmark code on the [logger_benchmarks](https://github.com/odygrd/logger_benchmarks) repository.

### Latency

The results presented in the tables below are measured in nanoseconds (ns).

The tables are sorted by the 90th percentile (lower is better).

#### Logging Numbers

`LOG_INFO(logger, "Logging int: {}, int: {}, double: {}", i, j, d)`.

##### 1 Thread Logging

| Library                                                                   |  50th  |  75th  |  90th  |  95th  |  99th  | 99.9th |
|---------------------------------------------------------------------------|:------:|:------:|:------:|:------:|:------:|:------:|
| [fmtlog](https://github.com/MengRao/fmtlog)                               |   6    |   6    |   6    |   6    |   7    |   10   |
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill)           |   6    |   6    |   6    |   6    |   8    |   9    |
| [XTR](https://github.com/choll/xtr)                                       |   6    |   6    |   6    |   6    |   9    |   10   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)                  |   6    |   6    |   6    |   7    |   8    |   10   |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)             |   8    |   8    |   9    |   10   |   10   |   11   |
| [Quill - Macro Free Mode](https://github.com/odygrd/quill)                |   11   |   12   |   14   |   15   |   16   |   18   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                     |   18   |   18   |   18   |   19   |   73   |  119   |
| [Reckless](https://github.com/mattiasflodin/reckless)                     |   26   |   28   |   31   |   33   |   35   |   41   |
| [BqLog](https://github.com/Tencent/BqLog)                                 |  125   |  133   |  138   |  141   |  151   |  190   |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                  |  106   |  116   |  155   |  163   |  392   |  491   |
| [spdlog](https://github.com/gabime/spdlog)                                |  271   |  280   |  296   |  309   |  337   |  360   |
| [g3log](https://github.com/KjellKod/g3log)                                |  1066  |  1081  |  1095  |  1103  |  1120  |  1143  |
| [Boost.Log](https://www.boost.org)                                        |  3093  |  3149  |  3259  |  3297  |  3464  |  3621  |

![Logging numbers 1-thread latency chart](docs/charts/numbers_1_thread_logging.svg)

##### 4 Threads Logging Simultaneously

| Library                                                                   |  50th  |  75th  |  90th  |  95th  |  99th  | 99.9th |
|---------------------------------------------------------------------------|:------:|:------:|:------:|:------:|:------:|:------:|
| [fmtlog](https://github.com/MengRao/fmtlog)                               |   8    |   8    |   8    |   8    |   10   |   14   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)                  |   8    |   8    |   8    |   8    |   11   |   17   |
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill)           |   8    |   8    |   8    |   8    |   17   |   18   |
| [XTR](https://github.com/choll/xtr)                                       |   8    |   8    |   8    |   9    |   17   |   18   |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)             |   14   |   14   |   14   |   14   |   16   |   21   |
| [Quill - Macro Free Mode](https://github.com/odygrd/quill)                |   13   |   14   |   17   |   21   |   25   |   32   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                     |   29   |   29   |   29   |   31   |  243   |  445   |
| [Reckless](https://github.com/mattiasflodin/reckless)                     |   27   |   37   |   44   |   47   |   59   |   96   |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                  |   73   |   78   |  267   |  280   |  396   |  1419  |
| [BqLog](https://github.com/Tencent/BqLog)                                 |  109   |  395   |  410   |  419   |  447   |  621   |
| [spdlog](https://github.com/gabime/spdlog)                                |  557   |  585   |  614   |  640   |  741   |  1106  |
| [g3log](https://github.com/KjellKod/g3log)                                |  1188  |  1300  |  1402  |  1484  |  1631  |  1936  |
| [Boost.Log](https://www.boost.org)                                        |  1582  |  2644  |  3148  |  3177  |  3878  |  5050  |

![Logging numbers 4-thread latency chart](docs/charts/numbers_4_thread_logging.svg)

#### Logging Large Strings

Logging `std::string` over 35 characters to prevent the short string optimization.

`LOG_INFO(logger, "Logging int: {}, int: {}, string: {}", i, j, large_string)`.

##### 1 Thread Logging

| Library                                                                   |  50th  |  75th  |  90th  |  95th  |  99th  | 99.9th |
|---------------------------------------------------------------------------|:------:|:------:|:------:|:------:|:------:|:------:|
| [fmtlog](https://github.com/MengRao/fmtlog)                               |   8    |   8    |   9    |   10   |   12   |   13   |
| [XTR](https://github.com/choll/xtr)                                       |   7    |   8    |   9    |   10   |   12   |   15   |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)             |   11   |   11   |   12   |   13   |   14   |   16   |
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill)           |   9    |   10   |   12   |   13   |   15   |   19   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)                  |   11   |   12   |   14   |   16   |   18   |   21   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                     |   20   |   20   |   21   |   22   |   77   |  122   |
| [Quill - Macro Free Mode](https://github.com/odygrd/quill)                |   17   |   19   |   21   |   23   |   25   |   28   |
| [Reckless](https://github.com/mattiasflodin/reckless)                     |   88   |  104   |  111   |  114   |  120   |  135   |
| [BqLog](https://github.com/Tencent/BqLog)                                 |  125   |  132   |  137   |  141   |  156   |  191   |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                  |  104   |  113   |  153   |  161   |  381   |  469   |
| [spdlog](https://github.com/gabime/spdlog)                                |  247   |  253   |  260   |  266   |  278   |  290   |
| [g3log](https://github.com/KjellKod/g3log)                                |  838   |  848   |  856   |  861   |  870   |  892   |
| [Boost.Log](https://www.boost.org)                                        |  2844  |  2992  |  3019  |  3050  |  3140  |  3263  |

![Logging large strings 1-thread latency chart](docs/charts/large_strings_1_thread_logging.svg)

##### 4 Threads Logging Simultaneously

| Library                                                                   |  50th  |  75th  |  90th  |  95th  |  99th  | 99.9th |
|---------------------------------------------------------------------------|:------:|:------:|:------:|:------:|:------:|:------:|
| [fmtlog](https://github.com/MengRao/fmtlog)                               |   8    |   9    |   10   |   13   |   16   |   23   |
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill)           |   8    |   9    |   14   |   16   |   21   |   26   |
| [XTR](https://github.com/choll/xtr)                                       |   8    |   9    |   16   |   17   |   21   |   27   |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)             |   15   |   15   |   17   |   20   |   22   |   27   |
| [Quill - Macro Free Mode](https://github.com/odygrd/quill)                |   10   |   11   |   18   |   24   |   28   |   35   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)                  |   17   |   18   |   19   |   22   |   27   |   29   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                     |   30   |   31   |   35   |   37   |  250   |  450   |
| [Reckless](https://github.com/mattiasflodin/reckless)                     |   44   |   85   |  132   |  144   |  165   |  183   |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                  |   74   |   86   |  279   |  292   |  471   |  1457  |
| [BqLog](https://github.com/Tencent/BqLog)                                 |  137   |  396   |  412   |  424   |  463   |  647   |
| [spdlog](https://github.com/gabime/spdlog)                                |  529   |  558   |  588   |  613   |  695   |  1074  |
| [g3log](https://github.com/KjellKod/g3log)                                |  950   |  1029  |  1085  |  1202  |  1356  |  1577  |
| [Boost.Log](https://www.boost.org)                                        |  1322  |  2512  |  2923  |  3096  |  3737  |  4761  |

![Logging large strings 4-thread latency chart](docs/charts/large_strings_4_thread_logging.svg)

#### Logging Complex Types

Logging `std::vector<std::string>` containing 16 large strings, each ranging from 50 to 60 characters.

Note: some of the previous loggers do not support passing a `std::vector` as an argument.

`LOG_INFO(logger, "Logging int: {}, int: {}, vector: {}", i, j, v)`.

##### 1 Thread Logging

| Library                                                                   |  50th  |  75th  |  90th  |  95th  |  99th  | 99.9th |
|---------------------------------------------------------------------------|:------:|:------:|:------:|:------:|:------:|:------:|
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill)           |   53   |   58   |   63   |   69   |   99   |  120   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                     |   60   |   62   |   64   |   67   |   72   |  369   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)                  |  113   |  123   |  132   |  138   |  147   |  157   |
| [XTR](https://github.com/choll/xtr)                                       |  752   |  788   |  826   |  849   |  895   |  959   |
| [fmtlog](https://github.com/MengRao/fmtlog)                               |  774   |  812   |  849   |  869   |  911   |  958   |
| [Boost.Log](https://www.boost.org)                                        |  4009  |  4083  |  4145  |  4250  |  4411  |  4705  |
| [spdlog](https://github.com/gabime/spdlog)                                |  6906  |  7013  |  7120  |  7189  |  7447  |  8140  |

![Logging complex types 1-thread latency chart](docs/charts/vector_1_thread_logging.svg)

##### 4 Threads Logging Simultaneously

| Library                                                                   |  50th  |  75th  |  90th  |  95th  |  99th  | 99.9th |
|---------------------------------------------------------------------------|:------:|:------:|:------:|:------:|:------:|:------:|
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                     |   74   |   80   |   90   |   99   |  304   |  531   |
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill)           |   67   |   75   |   93   |  104   |  117   |  130   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)                  |   78   |   89   |  103   |  112   |  129   |  147   |
| [fmtlog](https://github.com/MengRao/fmtlog)                               |  674   |  701   |  726   |  740   |  770   |  808   |
| [XTR](https://github.com/choll/xtr)                                       |  674   |  713   |  752   |  776   |  816   |  849   |
| [Boost.Log](https://www.boost.org)                                        |  2624  |  3590  |  4342  |  4439  |  5457  |  7219  |
| [spdlog](https://github.com/gabime/spdlog)                                |  7047  |  7284  |  7581  |  7943  |  8669  |  9711  |

![Logging complex types 4-thread latency chart](docs/charts/vector_4_thread_logging.svg)

Each latency observation is the average of 20 log calls made in a tight loop. The benchmark waits approximately 2
milliseconds between observations and repeats this process for the configured number of iterations.

_For `Quill Bounded Dropping Queue`, the queue size is `262,144` bytes, twice the default size of `131,072` bytes._

### Throughput

Throughput measures how many log messages the backend logging thread can write to a log file per second (higher is
better). These tests use the same system configuration as the latency benchmarks.

The comparison is limited to asynchronous libraries with a flush-and-wait mechanism, ensuring that elapsed time covers
processing every message. Binary-output modes are labelled and included in the table for reference, but are not directly
comparable with human-readable text output, so the chart omits them.

Each benchmark logs 4 million instances of `"Iteration: {} int: {} double: {}"`.

| Library                                                            | million msg/second | elapsed time |
|--------------------------------------------------------------------|:------------------:|:------------:|
| [MS BinLog (binary log)](https://github.com/Morgan-Stanley/binlog) |              61.79 |    64 ms     |
| [BqLog (binary log)](https://github.com/Tencent/BqLog)             |              12.86 |    311 ms    |
| [XTR](https://github.com/choll/xtr)                                |               7.73 |    517 ms    |
| [Quill](https://github.com/odygrd/quill)                           |               6.44 |    620 ms    |
| [BqLog](https://github.com/Tencent/BqLog)                          |               5.49 |    728 ms    |
| [Quill - Macro Free Mode](https://github.com/odygrd/quill)         |               5.13 |    779 ms    |
| [fmtlog](https://github.com/MengRao/fmtlog)                        |               2.69 |   1485 ms    |
| [Reckless](https://github.com/mattiasflodin/reckless)              |               2.58 |   1548 ms    |
| [spdlog](https://github.com/gabime/spdlog)                         |               2.57 |   1557 ms    |
| [Boost.Log](https://www.boost.org)                                 |               0.33 |   12164 ms   |

![Throughput comparison chart](docs/charts/throughput.svg)

### Compilation Time

Compile times are measured on the system above using clean `Release` builds of
[`BENCHMARK_quill_compile_time`](https://github.com/odygrd/quill/blob/master/benchmarks/compile_time/compile_time_bench.cpp),
which compiles `2000` auto-generated log statements with varied argument types.

The measurements below were taken with `-march=x86-64-v3` for `Release`, running one clean build
at a time with `-j4`.
Clang builds additionally enable `-ftime-trace`.

Quill intentionally keeps call-site metadata such as file, line, format string, and tags out of the
frontend template identity. In the common macro-based path, that information is stored in a
`MacroMetadata` object and passed as a regular function argument. As a result, multiple log statements
with the same argument type pack can reuse the same `log_statement` instantiation; changing only the
call-site metadata does not create a new frontend template instantiation.

| Compiler       | Clean Build Time | Benchmark Binary | Main TU Object |
|:---------------|-----------------:|-----------------:|---------------:|
| `clang 17.0.6` |        `30.64 s` |        `5.87 MB` |     `10.10 MB` |
| `gcc 13.3.1`   |        `61.20 s` |        `6.22 MB` |      `9.28 MB` |

**Header include profile** — shows the additional headers pulled in when logging, following
the [recommended_usage](https://github.com/odygrd/quill/blob/master/examples/recommended_usage/recommended_usage.cpp)
example:

> [**Open in Speedscope ↗**](https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/master/docs/traces/recommended_usage.cpp.json)

**Compile-time benchmark** — measures compilation
of [2000 auto-generated log statements](https://github.com/odygrd/quill/blob/master/benchmarks/compile_time/compile_time_bench.cpp)
with various arguments:

> [**Open in Speedscope ↗**](https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/master/docs/traces/compile_time_bench.cpp.json)

To generate these profiles yourself:

```bash
cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release \
  -DQUILL_BUILD_BENCHMARKS=ON -DQUILL_ENABLE_TIME_TRACE=ON \
  -DCMAKE_CXX_FLAGS='-march=x86-64-v3' ..
cmake --build . --target BENCHMARK_quill_compile_time -j 4
# Load the resulting .cpp.json files into https://www.speedscope.app
```

### Verdict

Quill combines very low frontend latency with competitive text-output throughput while retaining a broad feature set.

The human-readable log files facilitate easier debugging and analysis. While initially larger, they compress
efficiently, with the size difference between human-readable and binary logs becoming minimal once zipped.

For example, for the same number of messages:

```
ms_binlog_backend_total_time.blog (binary log): 177 MB
ms_binlog_backend_total_time.zip (zipped binary log): 35 MB
```

```
quill_backend_total_time.log (human-readable log): 448 MB
quill_backend_total_time.zip (zipped human-readable log): 47 MB
```

If you prefer a binary-log workflow, MS BinLog is a strong alternative. It delivers excellent hot-path latency and
smaller raw files, but it trades away immediate readability and requires offline processing tools.

---

## 🧩 Usage

Also, see the [Quick Start Guide](https://quillcpp.readthedocs.io/en/latest/quick_start.html) for a brief introduction.

```c++
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/std/Array.h"

#include <string>
#include <utility>

int main()
{
  // Backend  
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  // A log message with number 123
  int a = 123;
  std::string l = "log";
  LOG_INFO(logger, "A {} message with number {}", l, a);

  // libfmt formatting language is supported 3.14e+00
  double pi = 3.141592653589793;
  LOG_INFO(logger, "libfmt formatting language is supported {:.2e}", pi);

  // Logging STD types is supported [1, 2, 3]
  std::array<int, 3> arr = {1, 2, 3};
  LOG_INFO(logger, "Logging STD types is supported {}", arr);

  // Logging STD types is supported [arr: [1, 2, 3]]
  LOGV_INFO(logger, "Logging STD types is supported", arr);

  // A message with two variables [a: 123, b: 3.17]
  double b = 3.17;
  LOGV_INFO(logger, "A message with two variables", a, b);

  for (uint32_t i = 0; i < 10; ++i)
  {
    // Will only log the message once per second
    LOG_INFO_LIMIT(std::chrono::seconds{1}, logger, "A {} message with number {}", l, a);
    LOGV_INFO_LIMIT(std::chrono::seconds{1}, logger, "A message with two variables", a, b);
  }

  LOG_TRACE_L3(logger, "Support for floats {:03.2f}", 1.23456);
  LOG_TRACE_L2(logger, "Positional arguments are {1} {0} ", "too", "supported");
  LOG_TRACE_L1(logger, "{:>30}", std::string_view {"right aligned"});
  LOG_DEBUG(logger, "Debugging foo {}", 1234);
  LOG_INFO(logger, "Welcome to Quill!");
  LOG_WARNING(logger, "A warning message.");
  LOG_ERROR(logger, "An error message. error code {}", 123);
  LOG_CRITICAL(logger, "A critical error.");
}
```

### Output

![example output](docs/example_output.svg)

### External CMake

#### Building and Installing Quill

To get started with Quill, clone the repository and install it using CMake:

```bash
git clone https://github.com/odygrd/quill.git
cd quill
mkdir cmake_build
cd cmake_build
cmake ..
make install
```

- **Custom Installation**: Specify a custom directory with `-DCMAKE_INSTALL_PREFIX=/path/to/install/dir`.
- **Build Examples**: Include examples with `-DQUILL_BUILD_EXAMPLES=ON`.

Next, add Quill to your project using `find_package()`:

```cmake
find_package(quill REQUIRED)
target_link_libraries(your_target PUBLIC quill::quill)
```

#### Sample Directory Structure

Organize your project directory like this:

```
my_project/
├── CMakeLists.txt
├── main.cpp
```

#### Sample CMakeLists.txt

Here is a minimal `CMakeLists.txt`:

```cmake
# If Quill is in a non-standard directory, specify its path.
set(CMAKE_PREFIX_PATH /path/to/quill)

# Find and link the Quill library.
find_package(quill REQUIRED)
add_executable(example main.cpp)
target_link_libraries(example PUBLIC quill::quill)
```

### Embedded CMake

If you prefer to vendor Quill directly, add it as a subdirectory:

#### Sample Directory Structure

```
my_project/
├── quill/            # Quill repo folder
├── CMakeLists.txt
├── main.cpp
```

#### Sample CMakeLists.txt

Use this `CMakeLists.txt` to include Quill directly:

```cmake
cmake_minimum_required(VERSION 3.8)
project(my_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(quill)
add_executable(my_project main.cpp)
target_link_libraries(my_project PUBLIC quill::quill)
```

### Android NDK

Android usually works without special handling. If your toolchain does not support thread names,
configure with:

```bash
-DQUILL_NO_THREAD_NAME_SUPPORT:BOOL=ON
```

For timestamps, use `quill::ClockSourceType::System`. Quill also includes an `AndroidSink` for
Android's logging system.

#### Minimal Example to Start Logging on Android

```c++
quill::Backend::start();

auto sink = quill::Frontend::create_or_get_sink<quill::AndroidSink>("app", [](){
    quill::AndroidSinkConfig asc;
    asc.set_tag("app");
    asc.set_format_message(true);
    return asc;
}());

auto logger = quill::Frontend::create_or_get_logger("root", std::move(sink),
                                                    quill::PatternFormatterOptions {}, 
                                                    quill::ClockSourceType::System);

LOG_INFO(logger, "Test {}", 123);
```

### Meson

#### Using WrapDB

Install Quill from Meson's `wrapdb` with:

```bash
meson wrap install quill
```

#### Manual Integration

Or copy the repository into `subprojects` and add the following to `meson.build`:

```meson
quill = subproject('quill')
quill_dep = quill.get_variable('quill_dep')
my_build_target = executable('name', 'main.cpp', dependencies : [quill_dep], install : true)
```

### Bazel

#### Using Bzlmod

Quill is available on `Bzlmod`.

#### Manual Integration

For manual setup, add Quill to your `BUILD.bazel` file like this:

```bazel
cc_binary(name = "app", srcs = ["main.cpp"], deps = ["//quill_path:quill"])
```

---

## 📐 Design

Quill is split into a **hot frontend** and a **cold backend**.

- Each frontend thread owns a lock-free SPSC queue. `LOG_*` macros binary-serialize arguments
  directly into that queue — no shared state, no contention between threads, no formatting work
  on the caller.
- A single backend worker drains all queues, merges events in timestamp order, invokes the
  per-argument-pack decode function to reconstruct arguments, runs `{fmt}` formatting and the
  `PatternFormatter`, and writes the resulting log lines or metric samples to the attached
  `Sink`s.

### Frontend (caller-thread)

When invoking a `LOG_` macro:

1. Creates a static constexpr metadata object containing the format string and source location.

2. Pushes the event into the SPSC lock-free queue. For each log message, Quill enqueues:

| Variable   |                                                  Description                                                   |
|------------|:--------------------------------------------------------------------------------------------------------------:|
| timestamp  |                                               Current timestamp                                                |
| Metadata*  |                                        Pointer to metadata information                                         |
| Logger*    |                                         Pointer to the logger instance                                         |
| DecodeFunc | A pointer to a templated function containing all the log message argument types, used for decoding the message |
| Args...    |           A serialized binary copy of each log message argument that was passed to the `LOG_` macro            |

When invoking `METRIC(...)` or `logger->publish_metric()`:

1. Reuses pre-registered `MetricMetadata`, so metric names and labels are not serialized again on the hot path.

2. Pushes a compact fixed-size sample record to the same SPSC queue.

| Variable        |                              Description                              |
|-----------------|:---------------------------------------------------------------------:|
| timestamp       |                           Current timestamp                           |
| MetricMetadata* |         Pointer to the pre-registered metric name and labels          |
| Logger*         |                    Pointer to the logger instance                     |
| value           | The actual sample value as a `double` (counter delta, latency, gauge) |

### Backend

The backend thread drains the SPSC queue, reconstructs log events, forwards metric samples to
`Sink::write_metric()`, and fans each log or metric event out to the sinks attached to the logger.

### Architecture Overview

The diagram below shows the end-to-end flow from hot frontend threads to the backend worker and sinks.

![design diagram](docs/design.drawio.svg)

---

## 🚨 Caveats

**Do not log from destructors of static or global objects.** Quill's internal singletons are
function-local statics destroyed in reverse construction order. If a static object's constructor
triggers the first log call, the library singletons are constructed *after* that object and
destroyed *before* it. Logging from that destructor will then touch already-destroyed state.

**Use `fork()` with care.** Quill starts a background thread, and `fork()` interacts poorly with
multithreaded processes. If you need logging in child processes, call `quill::Backend::start()`
after `fork()` in each process that should log, and write parent and child output to different
files.

Example:

```c++
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

int main()
{
  // DO NOT CALL THIS BEFORE FORK
  // quill::Backend::start();

  if (fork() == 0)
  {
    quill::Backend::start();

    // Write child output to its own file.
    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("child.log");
    
    quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));

    LOG_INFO(logger, "Hello from Child {}", 123);
  }
  else
  {
    quill::Backend::start();

    // Write parent output to its own file.
    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("parent.log");

    quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));

    LOG_INFO(logger, "Hello from Parent {}", 123);
  }
}
```

---

## 📝 License

Quill is licensed under the [MIT License](https://opensource.org/licenses/MIT).

Quill depends on third party libraries with separate copyright notices and license terms.
Your use of the source code for these subcomponents is subject to the terms and conditions of the following licenses.

- ([MIT License](https://opensource.org/licenses/MIT)) [{fmt}](https://github.com/fmtlib/fmt/blob/master/LICENSE)
- ([MIT License](https://opensource.org/licenses/MIT)) [doctest](https://github.com/onqtam/doctest/blob/master/LICENSE.txt)
