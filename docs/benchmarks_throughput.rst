.. title:: C++ Logging Throughput Benchmarks

.. meta::
   :description: How many log messages per second C++ logging libraries write to a file: Quill, spdlog, fmtlog, BqLog, XTR, NanoLog, MS BinLog, Reckless and Boost.Log, with one and four producer threads.

.. _benchmarks_throughput:

Throughput
==========

How many messages per second a library writes to a log file, in millions (higher is better). Each benchmark logs
4 million messages as fast as the producer threads can, and the clock runs from the first message until the last
one has been written to the file. The shared settings, log pattern and message check are under
:ref:`How the Benchmarks Are Run <benchmarks:how the benchmarks are run>`.

- **Workload:** 256 distinct log statements with one to five arguments each (integers, doubles, strings and
  characters), logged in a fixed pseudo-random order. An application has hundreds of log statements, so the
  backend formats many different format strings, argument types and call sites rather than the same few.
- **1 Producer** logs all 4 million messages from one thread. **4 Producers** split them between four threads,
  which shows how a library scales when several threads log at once.
- **Elapsed time** runs until the last message is written. **Producers done** is when the producer threads had
  logged their share: the gap is the time the backend needed to catch up. A producer can finish early when its
  queue grows while the backend writes.
- **The clock stops** once the data has been handed to the operating system: the benchmark does not wait for the
  disk, but :ref:`PlatformLab NanoLog <benchmarks_library_notes:platformlab nanolog>` opens its log for
  synchronous writes.
- **Queues:** each library's default type and size, as in the latency benchmarks, so fixed-size queues differ:
  spdlog has 8,192 messages shared by all threads, XTR 256 KiB and fmtlog 1 MiB per thread, BqLog and Reckless
  their default buffers. Quill's queue per thread, and the queues of MS BinLog and Boost.Log, grow instead. The
  :doc:`library notes <benchmarks_library_notes>` give each library's settings.
- **Libraries shown:** only those that can wait until every message is written, so the elapsed time covers all
  of them. g3log and Iyengar NanoLog have no such call.
- **Binary logs** are listed for reference but are not directly comparable with text output, so the chart leaves
  them out unless you include them.
- **MS BinLog's** result with four producers varies widely between runs on Linux Intel, from about 50 to 100
  million messages per second, as its queues grow during the run; its decoded log holds every message.

.. include:: benchmark_tables/throughput.inc
