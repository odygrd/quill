<div align="center">
  <!--
  <meta name="description" content="C++20 or higher logging library. High-performance, asynchronous logging for low-latency real-time applications.">
  <meta name="keywords" content="C++20, modern C++, logging library, asynchronous logging, high performance, low latency">
  -->

  <br>
  <!-- img src="docs/logo.png" alt="Quill-T (quilt or quill-trimmed) C++ 20+ Logging Library" width="200" height="auto" / -->
  <h1>Quill-T >= C++20</h1>
  <p><b>quill-t or quilt or quill-trimmed - C++20 (and higher) Asynchronous Low Latency C++ Logging Library</b></p>
  <p><b>Modern C++ (>= c++20) Linux only minimal fork of <a href="https://github.com/odygrd/quill">quill (C++17 only library)</a> with Python integration</b></p>

  <!-- div>
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
  </div -->

  <!-- div>
    <a href="https://codecov.io/gh/odygrd/quill">
      <img src="https://img.shields.io/codecov/c/gh/odygrd/quill/master.svg?logo=codecov&style=flat-square" alt="Codecov" />
    </a>
    <a href="https://app.codacy.com/gh/odygrd/quill/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade">
      <img src="https://img.shields.io/codacy/grade/cd387bc34658475d98bff84db3ad5287?logo=codacy&style=flat-square" alt="Codacy" />
    </a>
    <a href="https://www.codefactor.io/repository/github/odygrd/quill">
      <img src="https://img.shields.io/codefactor/grade/github/odygrd/quill?logo=codefactor&style=flat-square" alt="CodeFactor" />
    </a>
  </div -->

  <div>
    <a href="https://opensource.org/licenses/MIT">
      <img src="https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square" alt="license" />
    </a>
    <a href="https://en.wikipedia.org/wiki/C%2B%2B17">
      <img src="https://img.shields.io/badge/language-C%2B%2B20-red.svg?style=flat-square" alt="language" />
    </a>
  </div>

  <!-- h4>
    <a href="https://quillcpp.readthedocs.io" title="Explore the full documentation">📚 Documentation</a>
    <span> · </span>
    <a href="https://quillcpp.readthedocs.io/en/latest/cheat_sheet.html" title="Quick reference for common tasks">⚡ Cheat Sheet</a>
    <span> · </span>
    <a href="https://github.com/odygrd/quill/issues/new?assignees=&labels=&projects=&template=bug-report.md&title=" title="Report a bug or issue">🐛 Report Bug</a>
    <span> · </span>
    <a href="https://github.com/odygrd/quill/issues/new?assignees=&labels=&projects=&template=feature_request.md&title=">💡 Request Feature</a>
  </h4 -->

  <div align="center"><img src="docs/quill_demo.gif" alt="Logging Demo" width="75%" /></div>
</div>

---

[//]: # ()
[//]: # (## 🧭 Table of Contents)

[//]: # ()
[//]: # (- [Introduction]&#40;#-introduction&#41;)

[//]: # (- [Quick Start]&#40;#-quick-start&#41;)

[//]: # (- [Features]&#40;#-features&#41;)

[//]: # (- [Performance]&#40;#-performance&#41;)

[//]: # (- [Usage]&#40;#-usage&#41;)

[//]: # (- [Design]&#40;#-design&#41;)

[//]: # (- [Caveats]&#40;#-caveats&#41;)

[//]: # (- [License]&#40;#-license&#41;)

[//]: # ()
[//]: # (---)

[//]: # ()
[//]: # (## ✨ Introduction)

[//]: # ()
[//]: # (**Quill** is a **high-performance asynchronous logging library** written in **C++**. It is designed for low-latency, performance-critical applications where every microsecond counts.)

[//]: # ()
[//]: # (- **Performance-Focused**: Quill consistently outperforms many popular logging libraries.)

[//]: # (- **Feature-Rich**: Packed with advanced features to meet diverse logging needs.)

[//]: # (- **Battle-Tested**: Proven in demanding production environments.)

[//]: # (- **Extensive Documentation**: Comprehensive guides and examples available.)

[//]: # (- **Community-Driven**: Open to contributions, feedback, and feature requests.)

[//]: # ()
[//]: # (Try it on [Compiler Explorer]&#40;https://godbolt.org/z/szncr8c8d&#41;)

[//]: # ()
[//]: # (---)

[//]: # ()
[//]: # (## ⏩ Quick Start)

[//]: # ()
[//]: # (Getting started is easy and straightforward. Follow these steps to integrate the library into your project:)

[//]: # ()
[//]: # (### Installation)

[//]: # ()
[//]: # (You can install Quill using the package manager of your choice:)

[//]: # ()
[//]: # (| Package Manager |              Installation Command              |)

[//]: # (|:---------------:|:----------------------------------------------:|)

[//]: # (|      vcpkg      |             `vcpkg install quill`              |)

[//]: # (|      Conan      |             `conan install quill`              |)

[//]: # (|    Homebrew     |              `brew install quill`              |)

[//]: # (|  Meson WrapDB   |           `meson wrap install quill`           |)

[//]: # (|      Conda      |      `conda install -c conda-forge quill`      |)

[//]: # (|     Bzlmod      | `bazel_dep&#40;name = "quill", version = "x.y.z"&#41;` |)

[//]: # (|      xmake      |             `xrepo install quill`              |)

[//]: # (|       nix       |            `nix-shell -p quill-log`            |)

[//]: # ()
[//]: # (### Setup)

[//]: # ()
[//]: # (Once installed, you can start using Quill with the following code:)

[//]: # ()
[//]: # (```c++)

[//]: # (#include "quill/Backend.h")

[//]: # (#include "quill/Frontend.h")

[//]: # (#include "quill/LogMacros.h")

[//]: # (#include "quill/Logger.h")

[//]: # (#include "quill/sinks/ConsoleSink.h")

[//]: # (#include <string_view>)

[//]: # ()
[//]: # (int main&#40;&#41;)

[//]: # ({)

[//]: # (  quill::Backend::start&#40;&#41;;)

[//]: # ()
[//]: # (  quill::Logger* logger = quill::Frontend::create_or_get_logger&#40;)

[//]: # (    "root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>&#40;"sink_id_1"&#41;&#41;;)

[//]: # ()
[//]: # (  LOG_INFO&#40;logger, "Hello from {}!", std::string_view{"Quill"}&#41;;)

[//]: # (})

[//]: # (```)

[//]: # ()
[//]: # (---)

[//]: # ()
[//]: # (## 🎯 Features)

[//]: # ()
[//]: # (- **High-Performance**: Ultra-low latency performance. View [Benchmarks]&#40;http://github.com/odygrd/quill#performance&#41;)

[//]: # (- **Asynchronous Processing**: Background thread handles formatting and I/O, keeping your main thread responsive.)

[//]: # (- **Minimal Header Includes**:)

[//]: # (    - **Frontend**: Only `Logger.h` and `LogMacros.h` needed for logging. Lightweight with minimal dependencies.)

[//]: # (    - **Backend**: Single `.cpp` file inclusion. No backend code injection into other translation units.)

[//]: # (- **Compile-Time Optimization**: Eliminate specific log levels at compile time.)

[//]: # (- **Custom Formatters**: Define your own log output patterns.)

[//]: # (  See [Formatters]&#40;https://quillcpp.readthedocs.io/en/latest/formatters.html&#41;.)

[//]: # (- **Timestamp-Ordered Logs**: Simplify debugging of multithreaded applications with chronologically ordered logs.)

[//]: # (- **Flexible Timestamps**: Support for `rdtsc`, `chrono`, or `custom clocks` - ideal for simulations and more.)

[//]: # (- **Backtrace Logging**: Store messages in a ring buffer for on-demand display.)

[//]: # (  See [Backtrace Logging]&#40;https://quillcpp.readthedocs.io/en/latest/backtrace_logging.html&#41;)

[//]: # (- **Multiple Output Sinks**: Console &#40;with color&#41;, files &#40;with rotation&#41;, JSON, ability to create custom sinks and more.)

[//]: # (- **Log Filtering**: Process only relevant messages.)

[//]: # (  See [Filters]&#40;https://quillcpp.readthedocs.io/en/latest/filters.html&#41;.)

[//]: # (- **JSON Logging**: Structured log output.)

[//]: # (  See [JSON Logging]&#40;https://quillcpp.readthedocs.io/en/latest/json_logging.html&#41;)

[//]: # (- **Configurable Queue Modes**: `bounded/unbounded` and `blocking/dropping` options with monitoring on dropped messages,)

[//]: # (  queue reallocations, and blocked hot threads.)

[//]: # (- **Crash Handling**: Built-in signal handler for log preservation during crashes.)

[//]: # (- **Huge Pages Support &#40;Linux&#41;**: Leverage huge pages on the hot path for optimized performance.)

[//]: # (- **Wide Character Support &#40;Windows&#41;**: Compatible with ASCII-encoded wide strings and STL containers consisting of wide)

[//]: # (  strings.)

[//]: # (- **Exception-Free Option**: Configurable builds with or without exception handling.)

[//]: # (- **Clean Codebase**: Maintained to high standards, warning-free even at strict levels.)

[//]: # (- **Type-Safe API**: Built on [{fmt}]&#40;http://github.com/fmtlib/fmt&#41; library.)

[//]: # ()
[//]: # (---)

[//]: # ()
[//]: # (## 🚀 Performance)

[//]: # ()
[//]: # (### System Configuration)

[//]: # ()
[//]: # (- **OS:** Linux RHEL 9.4)

[//]: # (- **CPU:** Intel Core i5-12600 &#40;12th Gen&#41; @ 4.8 GHz)

[//]: # (- **Compiler:** GCC 13.1)

[//]: # (- **Benchmark-Tuned System:** The system is specifically tuned for benchmarking.)

[//]: # ()
[//]: # (- **Command Line Parameters:**)

[//]: # (  ```shell)

[//]: # (  $ cat /proc/cmdline)

[//]: # (  BOOT_IMAGE=&#40;hd0,gpt2&#41;/vmlinuz-5.14.0-427.13.1.el9_4.x86_64 root=/dev/mapper/rhel-root ro crashkernel=1G-4G:192M,4G-64G:256M,64G-:512M resume=/dev/mapper/rhel-swap rd.lvm.lv=rhel/root rd.lvm.lv=rhel/swap rhgb quiet nohz=on nohz_full=1-5 rcu_nocbs=1-5 isolcpus=1-5 mitigations=off transparent_hugepage=never intel_pstate=disable nosoftlockup irqaffinity=0 processor.max_cstate=1 nosoftirqd sched_tick_offload=0 spec_store_bypass_disable=off spectre_v2=off iommu=pt)

[//]: # (  ```)

[//]: # ()
[//]: # (You can find the benchmark code on the [logger_benchmarks]&#40;http://github.com/odygrd/logger_benchmarks&#41; repository.)

[//]: # ()
[//]: # (### Latency)

[//]: # ()
[//]: # (The results presented in the tables below are measured in `nanoseconds &#40;ns&#41;`.)

[//]: # ()
[//]: # (The tables are sorted by the 95th percentile)

[//]: # ()
[//]: # (#### Logging Numbers)

[//]: # ()
[//]: # (`LOG_INFO&#40;logger, "Logging int: {}, int: {}, double: {}", i, j, d&#41;`.)

[//]: # ()
[//]: # (##### 1 Thread Logging)

[//]: # ()
[//]: # (| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |)

[//]: # (|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|)

[//]: # (| [Quill Bounded Dropping Queue]&#40;http://github.com/odygrd/quill&#41; |  7   |  8   |  8   |  9   |  9   |   11   |)

[//]: # (| [fmtlog]&#40;http://github.com/MengRao/fmtlog&#41;                     |  9   |  9   |  10  |  10  |  12  |   13   |)

[//]: # (| [Quill Unbounded Queue]&#40;http://github.com/odygrd/quill&#41;        |  10  |  10  |  10  |  10  |  12  |   14   |)

[//]: # (| [PlatformLab NanoLog]&#40;http://github.com/PlatformLab/NanoLog&#41;   |  13  |  14  |  16  |  17  |  19  |   25   |)

[//]: # (| [MS BinLog]&#40;http://github.com/Morgan-Stanley/binlog&#41;           |  21  |  21  |  22  |  22  |  56  |   93   |)

[//]: # (| [XTR]&#40;https://github.com/choll/xtr&#41;                            |  7   |  7   |  29  |  30  |  33  |   53   |)

[//]: # (| [Reckless]&#40;http://github.com/mattiasflodin/reckless&#41;           |  26  |  28  |  31  |  32  |  35  |   49   |)

[//]: # (| [BqLog]&#40;https://github.com/Tencent/BqLog&#41;                      |  29  |  29  |  30  |  49  |  56  |   71   |)

[//]: # (| [Iyengar NanoLog]&#40;http://github.com/Iyengar111/NanoLog&#41;        |  83  |  96  | 117  | 125  | 152  |  197   |)

[//]: # (| [spdlog]&#40;http://github.com/gabime/spdlog&#41;                      | 143  | 147  | 152  | 158  | 165  |  177   |)

[//]: # (| [g3log]&#40;http://github.com/KjellKod/g3log&#41;                      | 1161 | 1259 | 1329 | 1419 | 1602 |  1827  |)

[//]: # ()
[//]: # (![numbers_1_thread_logging.webp]&#40;docs%2Fcharts%2Fnumbers_1_thread_logging.webp&#41;)

[//]: # ()
[//]: # (##### 4 Threads Logging Simultaneously)

[//]: # ()
[//]: # (| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |)

[//]: # (|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|)

[//]: # (| [fmtlog]&#40;http://github.com/MengRao/fmtlog&#41;                     |  8   |  9   |  9   |  10  |  11  |   13   |)

[//]: # (| [Quill Bounded Dropping Queue]&#40;http://github.com/odygrd/quill&#41; |  8   |  9   |  10  |  10  |  12  |   14   |)

[//]: # (| [XTR]&#40;https://github.com/choll/xtr&#41;                            |  7   |  8   |  9   |  11  |  31  |   38   | )

[//]: # (| [Quill Unbounded Queue]&#40;http://github.com/odygrd/quill&#41;        |  10  |  11  |  11  |  12  |  13  |   15   |)

[//]: # (| [PlatformLab NanoLog]&#40;http://github.com/PlatformLab/NanoLog&#41;   |  15  |  17  |  20  |  23  |  27  |   32   |)

[//]: # (| [MS BinLog]&#40;http://github.com/Morgan-Stanley/binlog&#41;           |  21  |  22  |  22  |  23  |  62  |  100   |)

[//]: # (| [Reckless]&#40;http://github.com/mattiasflodin/reckless&#41;           |  19  |  23  |  26  |  28  |  34  |   55   |)

[//]: # (| [BqLog]&#40;https://github.com/Tencent/BqLog&#41;                      |  31  |  33  |  34  |  55  |  61  |   73   |)

[//]: # (| [Iyengar NanoLog]&#40;http://github.com/Iyengar111/NanoLog&#41;        |  58  |  90  | 123  | 131  | 168  |  242   |)

[//]: # (| [spdlog]&#40;http://github.com/gabime/spdlog&#41;                      | 210  | 243  | 288  | 313  | 382  |  694   |)

[//]: # (| [g3log]&#40;http://github.com/KjellKod/g3log&#41;                      | 1271 | 1337 | 1396 | 1437 | 1614 |  1899  |)

[//]: # ()
[//]: # (![numbers_4_thread_logging.webp]&#40;docs%2Fcharts%2Fnumbers_4_thread_logging.webp&#41;)

[//]: # ()
[//]: # (#### Logging Large Strings)

[//]: # ()
[//]: # (Logging `std::string` over 35 characters to prevent the short string optimization.)

[//]: # ()
[//]: # (`LOG_INFO&#40;logger, "Logging int: {}, int: {}, string: {}", i, j, large_string&#41;`.)

[//]: # ()
[//]: # (##### 1 Thread Logging)

[//]: # ()
[//]: # (| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |)

[//]: # (|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|)

[//]: # (| [Quill Bounded Dropping Queue]&#40;http://github.com/odygrd/quill&#41; |  11  |  13  |  13  |  14  |  15  |   16   |)

[//]: # (| [fmtlog]&#40;http://github.com/MengRao/fmtlog&#41;                     |  11  |  12  |  13  |  14  |  15  |   17   |)

[//]: # (| [Quill Unbounded Queue]&#40;http://github.com/odygrd/quill&#41;        |  14  |  15  |  16  |  17  |  18  |   19   |)

[//]: # (| [MS BinLog]&#40;http://github.com/Morgan-Stanley/binlog&#41;           |  22  |  23  |  24  |  25  |  61  |  100   |)

[//]: # (| [PlatformLab NanoLog]&#40;http://github.com/PlatformLab/NanoLog&#41;   |  15  |  17  |  21  |  27  |  33  |   39   |)

[//]: # (| [XTR]&#40;https://github.com/choll/xtr&#41;                            |  8   |  9   |  29  |  31  |  35  |   54   |)

[//]: # (| [BqLog]&#40;https://github.com/Tencent/BqLog&#41;                      |  29  |  30  |  31  |  51  |  60  |   71   |)

[//]: # (| [Reckless]&#40;http://github.com/mattiasflodin/reckless&#41;           |  91  | 107  | 115  | 118  | 124  |  135   |)

[//]: # (| [Iyengar NanoLog]&#40;http://github.com/Iyengar111/NanoLog&#41;        |  86  |  97  | 119  | 128  | 159  |  268   |)

[//]: # (| [spdlog]&#40;http://github.com/gabime/spdlog&#41;                      | 120  | 124  | 128  | 132  | 141  |  151   |)

[//]: # (| [g3log]&#40;http://github.com/KjellKod/g3log&#41;                      | 881  | 956  | 1018 | 1089 | 1264 |  1494  |)

[//]: # ()
[//]: # (![large_strings_1_thread_logging.webp]&#40;docs%2Fcharts%2Flarge_strings_1_thread_logging.webp&#41;)

[//]: # ()
[//]: # (##### 4 Threads Logging Simultaneously)

[//]: # ()
[//]: # (| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |)

[//]: # (|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|)

[//]: # (| [XTR]&#40;https://github.com/choll/xtr&#41;                            |  9   |  11  |  13  |  14  |  32  |   40   |)

[//]: # (| [fmtlog]&#40;http://github.com/MengRao/fmtlog&#41;                     |  11  |  12  |  13  |  14  |  16  |   19   |)

[//]: # (| [Quill Bounded Dropping Queue]&#40;http://github.com/odygrd/quill&#41; |  13  |  14  |  15  |  16  |  17  |   19   |)

[//]: # (| [Quill Unbounded Queue]&#40;http://github.com/odygrd/quill&#41;        |  15  |  16  |  17  |  18  |  19  |   21   |)

[//]: # (| [MS BinLog]&#40;http://github.com/Morgan-Stanley/binlog&#41;           |  23  |  25  |  27  |  28  |  65  |  105   |)

[//]: # (| [PlatformLab NanoLog]&#40;http://github.com/PlatformLab/NanoLog&#41;   |  16  |  20  |  32  |  38  |  44  |   51   |)

[//]: # (| [BqLog]&#40;https://github.com/Tencent/BqLog&#41;                      |  32  |  33  |  35  |  56  |  64  |   76   |)

[//]: # (| [Reckless]&#40;http://github.com/mattiasflodin/reckless&#41;           |  79  |  94  | 104  | 107  | 114  |  132   |)

[//]: # (| [Iyengar NanoLog]&#40;http://github.com/Iyengar111/NanoLog&#41;        |  85  |  93  | 125  | 133  | 168  |  237   |)

[//]: # (| [spdlog]&#40;http://github.com/gabime/spdlog&#41;                      | 178  | 218  | 261  | 281  | 381  |  651   |)

[//]: # (| [g3log]&#40;http://github.com/KjellKod/g3log&#41;                      | 992  | 1055 | 1121 | 1178 | 1360 |  1600  |)

[//]: # ()
[//]: # (![large_strings_4_thread_logging.webp]&#40;docs%2Fcharts%2Flarge_strings_4_thread_logging.webp&#41;)

[//]: # ()
[//]: # (#### Logging Complex Types)

[//]: # ()
[//]: # (Logging `std::vector<std::string>` containing 16 large strings, each ranging from 50 to 60 characters.)

[//]: # ()
[//]: # (Note: some of the previous loggers do not support passing a `std::vector` as an argument.)

[//]: # ()
[//]: # (`LOG_INFO&#40;logger, "Logging int: {}, int: {}, vector: {}", i, j, v&#41;`.)

[//]: # ()
[//]: # (##### 1 Thread Logging)

[//]: # ()
[//]: # (| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |)

[//]: # (|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|)

[//]: # (| [Quill Bounded Dropping Queue]&#40;http://github.com/odygrd/quill&#41; |  48  |  50  |  53  |  55  |  58  |   62   |)

[//]: # (| [Quill Unbounded Queue]&#40;http://github.com/odygrd/quill&#41;        |  54  |  56  |  57  |  58  |  61  |   66   |)

[//]: # (| [MS BinLog]&#40;http://github.com/Morgan-Stanley/binlog&#41;           |  68  |  69  |  72  |  74  |  79  |  281   |)

[//]: # (| [XTR]&#40;https://github.com/choll/xtr&#41;                            | 284  | 294  | 340  | 346  | 356  |  575   |)

[//]: # (| [fmtlog]&#40;http://github.com/MengRao/fmtlog&#41;                     | 711  | 730  | 754  | 770  | 804  |  834   |)

[//]: # (| [spdlog]&#40;http://github.com/gabime/spdlog&#41;                      | 6191 | 6261 | 6330 | 6386 | 6633 |  7320  |)

[//]: # ()
[//]: # (![vector_1_thread_logging.webp]&#40;docs%2Fcharts%2Fvector_1_thread_logging.webp&#41;)

[//]: # ()
[//]: # (##### 4 Threads Logging Simultaneously)

[//]: # ()
[//]: # (| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |)

[//]: # (|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|)

[//]: # (| [Quill Bounded Dropping Queue]&#40;http://github.com/odygrd/quill&#41; |  50  |  52  |  54  |  56  |  60  |   82   |)

[//]: # (| [MS BinLog]&#40;http://github.com/Morgan-Stanley/binlog&#41;           |  70  |  72  |  75  |  79  |  88  |  286   |)

[//]: # (| [Quill Unbounded Queue]&#40;http://github.com/odygrd/quill&#41;        |  97  | 107  | 116  | 122  | 135  |  148   |)

[//]: # (| [XTR]&#40;https://github.com/choll/xtr&#41;                            | 512  | 711  | 761  | 791  | 865  |  945   |)

[//]: # (| [fmtlog]&#40;http://github.com/MengRao/fmtlog&#41;                     | 780  | 804  | 823  | 835  | 860  |  896   |)

[//]: # (| [spdlog]&#40;http://github.com/gabime/spdlog&#41;                      | 6469 | 6549 | 6641 | 6735 | 7631 |  9430  |)

[//]: # ()
[//]: # (![vector_4_thread_logging.webp]&#40;docs%2Fcharts%2Fvector_4_thread_logging.webp&#41;)

[//]: # ()
[//]: # (The benchmark methodology involves logging 20 messages in a loop, calculating and storing the average latency for those)

[//]: # (20 messages, then waiting around ~2 milliseconds, and repeating this process for a specified number of iterations.)

[//]: # ()
[//]: # (_In the `Quill Bounded Dropping` benchmarks, the dropping queue size is set to `262,144` bytes, which is double the)

[//]: # (default size of `131,072` bytes._)

[//]: # ()
[//]: # (### Throughput)

[//]: # ()
[//]: # (Throughput is measured by calculating the maximum number of log messages the backend logging thread can write to a log)

[//]: # (file per second.)

[//]: # ()
[//]: # (The tests were run on the same system used for the latency benchmarks.)

[//]: # ()
[//]: # (Although Quill’s primary focus is not on maximizing throughput, it efficiently manages log messages across multiple)

[//]: # (threads. Benchmarking throughput of asynchronous logging libraries presents certain challenges. Some libraries may drop)

[//]: # (log messages, leading to smaller-than-expected log files, while others only provide asynchronous flushing, making it)

[//]: # (difficult to verify when the backend thread has fully processed all messages.)

[//]: # ()
[//]: # (For comparison, we benchmark against other asynchronous logging libraries that offer guaranteed logging with a)

[//]: # (flush-and-wait mechanism.)

[//]: # ()
[//]: # (Note that `MS BinLog` writes log data to a binary file, which requires offline formatting with an additional)

[//]: # (program—this makes it an unfair comparison, but it is included for reference.)

[//]: # ()
[//]: # (Similarly, `BqLog &#40;binary log&#41;` uses the compressed binary log appender, and its log files are not human-readable unless)

[//]: # (processed offline. However, it is included for reference. The other version of `BqLog` is using a text appender and)

[//]: # (produces human-readable log files.)

[//]: # ()
[//]: # (In the same way, `Platformlab Nanolog` also outputs binary logs and is expected to deliver high throughput. However, for)

[//]: # (reasons unexplained, the benchmark runs significantly slower &#40;10x longer&#41; than the other libraries, so it is excluded)

[//]: # (from the table.)

[//]: # ()
[//]: # (Logging 4 million times the message `"Iteration: {} int: {} double: {}"`)

[//]: # ()
[//]: # (| Library                                                           | million msg/second | elapsed time |)

[//]: # (|-------------------------------------------------------------------|:------------------:|:------------:|)

[//]: # (| [MS BinLog &#40;binary log&#41;]&#40;http://github.com/Morgan-Stanley/binlog&#41; |       63.80        |    62 ms     |)

[//]: # (| [BqLog &#40;binary log&#41;]&#40;https://github.com/Tencent/BqLog&#41;            |       15.92        |    251 ms    |)

[//]: # (| [Quill]&#40;http://github.com/odygrd/quill&#41;                           |        5.70        |    701 ms    |)

[//]: # (| [BqLog]&#40;https://github.com/Tencent/BqLog&#41;                         |        4.93        |    811 ms    |)

[//]: # (| [spdlog]&#40;http://github.com/gabime/spdlog&#41;                         |        3.54        |   1128 ms    |)

[//]: # (| [fmtlog]&#40;http://github.com/MengRao/fmtlog&#41;                        |        2.90        |   1378 ms    |)

[//]: # (| [Reckless]&#40;http://github.com/mattiasflodin/reckless&#41;              |        2.72        |   1471 ms    |)

[//]: # (| [XTR]&#40;https://github.com/choll/xtr&#41;                               |        2.61        |   1534 ms    |)

[//]: # ()
[//]: # (![throughput_chart.webp]&#40;docs%2Fcharts%2Fthroughput_chart.webp&#41;)

[//]: # ()
[//]: # (### Compilation Time)

[//]: # ()
[//]: # (Compile times are measured using `clang 15` and for `Release` build.)

[//]: # ()
[//]: # (Below, you can find the additional headers that the library will include when you need to log, following)

[//]: # (the [recommended_usage]&#40;https://github.com/odygrd/quill/blob/master/examples/recommended_usage/recommended_usage.cpp&#41;)

[//]: # (example)

[//]: # ()
[//]: # (![quill_v5_1_compiler_profile.speedscope.png]&#40;docs%2Fquill_v5_1_compiler_profile.speedscope.png&#41;)

[//]: # ()
[//]: # (There is also a compile-time benchmark measuring the compilation time of 2000 auto-generated log statements with)

[//]: # (various arguments. You can find)

[//]: # (it [here]&#40;https://github.com/odygrd/quill/blob/master/benchmarks/compile_time/compile_time_bench.cpp&#41;. It takes)

[//]: # (approximately 30 seconds to compile.)

[//]: # ()
[//]: # (![quill_v5_1_compiler_bench.speedscope.png]&#40;docs%2Fquill_v5_1_compiler_bench.speedscope.png&#41;)

[//]: # ()
[//]: # (### Verdict)

[//]: # ()
[//]: # (Quill excels in hot path latency benchmarks and supports high throughput, offering a rich set of features that outshines)

[//]: # (other logging libraries.)

[//]: # ()
[//]: # (The human-readable log files facilitate easier debugging and analysis. While initially larger, they compress)

[//]: # (efficiently, with the size difference between human-readable and binary logs becoming minimal once zipped.)

[//]: # ()
[//]: # (For example, for the same amount of messages:)

[//]: # ()
[//]: # (```)

[//]: # (ms_binlog_backend_total_time.blog &#40;binary log&#41;: 177 MB)

[//]: # (ms_binlog_backend_total_time.zip &#40;zipped binary log&#41;: 35 MB)

[//]: # (```)

[//]: # ()
[//]: # (```)

[//]: # (quill_backend_total_time.log &#40;human-readable log&#41;: 448 MB)

[//]: # (quill_backend_total_time.zip &#40;zipped human-readable log&#41;: 47 MB)

[//]: # (```)

[//]: # ()
[//]: # (If Quill were not available, MS BinLog would be a strong alternative. It delivers great latency on the hot path and)

[//]: # (generates smaller binary log files. However, the binary logs necessitate offline processing with additional tools, which)

[//]: # (can be less convenient.)

[//]: # ()
[//]: # (---)

[//]: # ()
[//]: # (## 🧩 Usage)

[//]: # ()
[//]: # (Also, see the [Quick Start Guide]&#40;https://quillcpp.readthedocs.io/en/latest/quick_start.html&#41; for a brief introduction.)

[//]: # ()
[//]: # (```c++)

[//]: # (#include "quill/Backend.h")

[//]: # (#include "quill/Frontend.h")

[//]: # (#include "quill/LogMacros.h")

[//]: # (#include "quill/Logger.h")

[//]: # (#include "quill/sinks/ConsoleSink.h")

[//]: # (#include "quill/std/Array.h")

[//]: # ()
[//]: # (#include <string>)

[//]: # (#include <utility>)

[//]: # ()
[//]: # (int main&#40;&#41;)

[//]: # ({)

[//]: # (  // Backend  )

[//]: # (  quill::BackendOptions backend_options;)

[//]: # (  quill::Backend::start&#40;backend_options&#41;;)

[//]: # ()
[//]: # (  // Frontend)

[//]: # (  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>&#40;"sink_id_1"&#41;;)

[//]: # (  quill::Logger* logger = quill::Frontend::create_or_get_logger&#40;"root", std::move&#40;console_sink&#41;&#41;;)

[//]: # ()
[//]: # (  // Change the LogLevel to print everything)

[//]: # (  logger->set_log_level&#40;quill::LogLevel::TraceL3&#41;;)

[//]: # ()
[//]: # (  // A log message with number 123)

[//]: # (  int a = 123;)

[//]: # (  std::string l = "log";)

[//]: # (  LOG_INFO&#40;logger, "A {} message with number {}", l, a&#41;;)

[//]: # ()
[//]: # (  // libfmt formatting language is supported 3.14e+00)

[//]: # (  double pi = 3.141592653589793;)

[//]: # (  LOG_INFO&#40;logger, "libfmt formatting language is supported {:.2e}", pi&#41;;)

[//]: # ()
[//]: # (  // Logging STD types is supported [1, 2, 3])

[//]: # (  std::array<int, 3> arr = {1, 2, 3};)

[//]: # (  LOG_INFO&#40;logger, "Logging STD types is supported {}", arr&#41;;)

[//]: # ()
[//]: # (  // Logging STD types is supported [arr: [1, 2, 3]])

[//]: # (  LOGV_INFO&#40;logger, "Logging STD types is supported", arr&#41;;)

[//]: # ()
[//]: # (  // A message with two variables [a: 123, b: 3.17])

[//]: # (  double b = 3.17;)

[//]: # (  LOGV_INFO&#40;logger, "A message with two variables", a, b&#41;;)

[//]: # ()
[//]: # (  for &#40;uint32_t i = 0; i < 10; ++i&#41;)

[//]: # (  {)

[//]: # (    // Will only log the message once per second)

[//]: # (    LOG_INFO_LIMIT&#40;std::chrono::seconds{1}, logger, "A {} message with number {}", l, a&#41;;)

[//]: # (    LOGV_INFO_LIMIT&#40;std::chrono::seconds{1}, logger, "A message with two variables", a, b&#41;;)

[//]: # (  })

[//]: # ()
[//]: # (  LOG_TRACE_L3&#40;logger, "Support for floats {:03.2f}", 1.23456&#41;;)

[//]: # (  LOG_TRACE_L2&#40;logger, "Positional arguments are {1} {0} ", "too", "supported"&#41;;)

[//]: # (  LOG_TRACE_L1&#40;logger, "{:>30}", std::string_view {"right aligned"}&#41;;)

[//]: # (  LOG_DEBUG&#40;logger, "Debugging foo {}", 1234&#41;;)

[//]: # (  LOG_INFO&#40;logger, "Welcome to Quill!"&#41;;)

[//]: # (  LOG_WARNING&#40;logger, "A warning message."&#41;;)

[//]: # (  LOG_ERROR&#40;logger, "An error message. error code {}", 123&#41;;)

[//]: # (  LOG_CRITICAL&#40;logger, "A critical error."&#41;;)

[//]: # (})

[//]: # (```)

[//]: # ()
[//]: # (### Output)

[//]: # ()
[//]: # (![example_output.png]&#40;docs%2Fexample_output.png&#41;)

[//]: # ()
[//]: # (### External CMake)

[//]: # ()
[//]: # (#### Building and Installing Quill)

[//]: # ()
[//]: # (To get started with Quill, clone the repository and install it using CMake:)

[//]: # ()
[//]: # (```bash)

[//]: # (git clone http://github.com/odygrd/quill.git)

[//]: # (mkdir cmake_build)

[//]: # (cd cmake_build)

[//]: # (cmake ..)

[//]: # (make install)

[//]: # (```)

[//]: # ()
[//]: # (- **Custom Installation**: Specify a custom directory with `-DCMAKE_INSTALL_PREFIX=/path/to/install/dir`.)

[//]: # (- **Build Examples**: Include examples with `-DQUILL_BUILD_EXAMPLES=ON`.)

[//]: # ()
[//]: # (Next, add Quill to your project using `find_package&#40;&#41;`:)

[//]: # ()
[//]: # (```cmake)

[//]: # (find_package&#40;quill REQUIRED&#41;)

[//]: # (target_link_libraries&#40;your_target PUBLIC quill::quill&#41;)

[//]: # (```)

[//]: # ()
[//]: # (#### Sample Directory Structure)

[//]: # ()
[//]: # (Organize your project directory like this:)

[//]: # ()
[//]: # (```)

[//]: # (my_project/)

[//]: # (├── CMakeLists.txt)

[//]: # (├── main.cpp)

[//]: # (```)

[//]: # ()
[//]: # (#### Sample CMakeLists.txt)

[//]: # ()
[//]: # (Here’s a sample `CMakeLists.txt` to get you started:)

[//]: # ()
[//]: # (```cmake)

[//]: # (# If Quill is in a non-standard directory, specify its path.)

[//]: # (set&#40;CMAKE_PREFIX_PATH /path/to/quill&#41;)

[//]: # ()
[//]: # (# Find and link the Quill library.)

[//]: # (find_package&#40;quill REQUIRED&#41;)

[//]: # (add_executable&#40;example main.cpp&#41;)

[//]: # (target_link_libraries&#40;example PUBLIC quill::quill&#41;)

[//]: # (```)

[//]: # ()
[//]: # (### Embedded CMake)

[//]: # ()
[//]: # (For a more integrated approach, embed Quill directly into your project:)

[//]: # ()
[//]: # (#### Sample Directory Structure)

[//]: # ()
[//]: # (```)

[//]: # (my_project/)

[//]: # (├── quill/            # Quill repo folder)

[//]: # (├── CMakeLists.txt)

[//]: # (├── main.cpp)

[//]: # (```)

[//]: # ()
[//]: # (#### Sample CMakeLists.txt)

[//]: # ()
[//]: # (Use this `CMakeLists.txt` to include Quill directly:)

[//]: # ()
[//]: # (```cmake)

[//]: # (cmake_minimum_required&#40;VERSION 3.1.0&#41;)

[//]: # (project&#40;my_project&#41;)

[//]: # ()
[//]: # (set&#40;CMAKE_CXX_STANDARD 17&#41;)

[//]: # (set&#40;CMAKE_CXX_STANDARD_REQUIRED ON&#41;)

[//]: # ()
[//]: # (add_subdirectory&#40;quill&#41;)

[//]: # (add_executable&#40;my_project main.cpp&#41;)

[//]: # (target_link_libraries&#40;my_project PUBLIC quill::quill&#41;)

[//]: # (```)

[//]: # ()
[//]: # (### Android NDK)

[//]: # ()
[//]: # (When building Quill for Android, you might need to add this flag during configuration, but in most cases, it works without it:)

[//]: # ()
[//]: # (```bash)

[//]: # (-DQUILL_NO_THREAD_NAME_SUPPORT:BOOL=ON)

[//]: # (```)

[//]: # ()
[//]: # (For timestamps, use `quill::ClockSourceType::System`. Quill also includes an `AndroidSink`, which integrates with Android's logging system.)

[//]: # ()
[//]: # (#### Minimal Example to Start Logging on Android)

[//]: # ()
[//]: # (```c++)

[//]: # (quill::Backend::start&#40;&#41;;)

[//]: # ()
[//]: # (auto sink = quill::Frontend::create_or_get_sink<quill::AndroidSink>&#40;"app", []&#40;&#41;{)

[//]: # (    quill::AndroidSinkConfig asc;)

[//]: # (    asc.set_tag&#40;"app"&#41;;)

[//]: # (    asc.set_format_message&#40;true&#41;;)

[//]: # (    return asc;)

[//]: # (}&#40;&#41;&#41;;)

[//]: # ()
[//]: # (auto logger = quill::Frontend::create_or_get_logger&#40;"root", std::move&#40;sink&#41;,)

[//]: # (                                                    quill::PatternFormatterOptions {}, )

[//]: # (                                                    quill::ClockSourceType::System&#41;;)

[//]: # ()
[//]: # (LOG_INFO&#40;logger, "Test {}", 123&#41;;)

[//]: # (```)

[//]: # ()
[//]: # (### Meson)

[//]: # ()
[//]: # (#### Using WrapDB)

[//]: # ()
[//]: # (Easily integrate Quill with Meson’s `wrapdb`:)

[//]: # ()
[//]: # (```bash)

[//]: # (meson wrap install quill)

[//]: # (```)

[//]: # ()
[//]: # (#### Manual Integration)

[//]: # ()
[//]: # (Copy the repository contents to your `subprojects` directory and add the following to your `meson.build`:)

[//]: # ()
[//]: # (```meson)

[//]: # (quill = subproject&#40;'quill'&#41;)

[//]: # (quill_dep = quill.get_variable&#40;'quill_dep'&#41;)

[//]: # (my_build_target = executable&#40;'name', 'main.cpp', dependencies : [quill_dep], install : true&#41;)

[//]: # (```)

[//]: # ()
[//]: # (### Bazel)

[//]: # ()
[//]: # (#### Using Blzmod)

[//]: # ()
[//]: # (Quill is available on `BLZMOD` for easy integration.)

[//]: # ()
[//]: # (#### Manual Integration)

[//]: # ()
[//]: # (For manual setup, add Quill to your `BUILD.bazel` file like this:)

[//]: # ()
[//]: # (```bazel)

[//]: # (cc_binary&#40;name = "app", srcs = ["main.cpp"], deps = ["//quill_path:quill"]&#41;)

[//]: # (```)

[//]: # ()
[//]: # (---)

[//]: # ()
[//]: # (## 📐 Design)

[//]: # ()
[//]: # (### Frontend &#40;caller-thread&#41;)

[//]: # ()
[//]: # (When invoking a `LOG_` macro:)

[//]: # ()
[//]: # (1. Creates a static constexpr metadata object to store `Metadata` such as the format string and source location.)

[//]: # ()
[//]: # (2. Pushes the data SPSC lock-free queue. For each log message, the following variables are pushed)

[//]: # ()
[//]: # (| Variable   |                                                  Description                                                   |)

[//]: # (|------------|:--------------------------------------------------------------------------------------------------------------:|)

[//]: # (| timestamp  |                                               Current timestamp                                                |)

[//]: # (| Metadata*  |                                        Pointer to metadata information                                         |)

[//]: # (| Logger*    |                                         Pointer to the logger instance                                         |)

[//]: # (| DecodeFunc | A pointer to a templated function containing all the log message argument types, used for decoding the message |)

[//]: # (| Args...    |           A serialized binary copy of each log message argument that was passed to the `LOG_` macro            |)

[//]: # ()
[//]: # (### Backend)

[//]: # ()
[//]: # (Consumes each message from the SPSC queue, retrieves all the necessary information and then formats the message.)

[//]: # (Subsequently, forwards the log message to all Sinks associated with the Logger.)

[//]: # ()
[//]: # (![design.jpg]&#40;docs%2Fdesign.jpg&#41;)

[//]: # ()
[//]: # (---)

[//]: # ()
[//]: # (## 🚨 Caveats)

[//]: # ()
[//]: # (Quill may not work well with `fork&#40;&#41;` since it spawns a background thread and `fork&#40;&#41;` doesn't work well with)

[//]: # (multithreading.)

[//]: # ()
[//]: # (If your application uses `fork&#40;&#41;` and you want to log in the child processes as well, you should call `quill::start&#40;&#41;`)

[//]: # (after the `fork&#40;&#41;` call. Additionally, you should ensure that you write to different files in the parent and child)

[//]: # (processes to avoid conflicts.)

[//]: # ()
[//]: # (For example :)

[//]: # ()
[//]: # (```c++)

[//]: # (#include "quill/Backend.h")

[//]: # (#include "quill/Frontend.h")

[//]: # (#include "quill/LogMacros.h")

[//]: # (#include "quill/Logger.h")

[//]: # (#include "quill/sinks/FileSink.h")

[//]: # ()
[//]: # (int main&#40;&#41;)

[//]: # ({)

[//]: # (  // DO NOT CALL THIS BEFORE FORK)

[//]: # (  // quill::Backend::start&#40;&#41;;)

[//]: # ()
[//]: # (  if &#40;fork&#40;&#41; == 0&#41;)

[//]: # (  {)

[//]: # (    quill::Backend::start&#40;&#41;;)

[//]: # (        )
[//]: # (    // Get or create a handler to the file - Write to a different file)

[//]: # (    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>&#40;)

[//]: # (      "child.log"&#41;;)

[//]: # (    )
[//]: # (    quill::Logger* logger = quill::Frontend::create_or_get_logger&#40;"root", std::move&#40;file_sink&#41;&#41;;)

[//]: # ()
[//]: # (    QUILL_LOG_INFO&#40;logger, "Hello from Child {}", 123&#41;;)

[//]: # (  })

[//]: # (  else)

[//]: # (  {)

[//]: # (    quill::Backend::start&#40;&#41;;)

[//]: # (          )
[//]: # (    // Get or create a handler to the file - Write to a different file)

[//]: # (    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>&#40;)

[//]: # (      "parent.log"&#41;;)

[//]: # (    )
[//]: # (    quill::Logger* logger = quill::Frontend::create_or_get_logger&#40;"root", std::move&#40;file_sink&#41;&#41;;)

[//]: # (    )
[//]: # (    QUILL_LOG_INFO&#40;logger, "Hello from Parent {}", 123&#41;;)

[//]: # (  })

[//]: # (})

[//]: # (```)

[//]: # ()
[//]: # (---)

[//]: # ()
[//]: # (## 📝 License)

[//]: # ()
[//]: # (Quill is licensed under the [MIT License]&#40;http://opensource.org/licenses/MIT&#41;)

[//]: # ()
[//]: # (Quill depends on third party libraries with separate copyright notices and license terms.)

[//]: # (Your use of the source code for these subcomponents is subject to the terms and conditions of the following licenses.)

[//]: # ()
[//]: # (- &#40;[MIT License]&#40;http://opensource.org/licenses/MIT&#41;&#41; [{fmt}]&#40;http://github.com/fmtlib/fmt/blob/master/LICENSE.rst&#41;)

[//]: # (- &#40;[MIT License]&#40;http://opensource.org/licenses/MIT&#41;&#41; [doctest]&#40;http://github.com/onqtam/doctest/blob/master/LICENSE.txt&#41;)
