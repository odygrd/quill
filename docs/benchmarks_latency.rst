.. title:: C++ Logging Latency Benchmarks

.. meta::
   :description: Latency of a log call on the calling thread for C++ logging libraries: Quill, spdlog, fmtlog, BqLog, XTR, NanoLog, MS BinLog, Reckless, g3log and Boost.Log, measured per call, in bursts, with a cold cache and for disabled statements.

.. _benchmarks_latency:

Log Call Latency
================

How long a log call takes on the thread that makes it, from just before the call until it returns, in
nanoseconds (lower is better). For an asynchronous logger this is the cost your application pays on its own
thread; formatting and writing happen later on the backend thread and are measured by the
:doc:`throughput benchmark <benchmarks_throughput>`.

Pick a payload, a platform and a thread count in the tabs. Every table has a **Timer only** row, the dashed line
in its chart: the same harness with a call that logs nothing, so the cost of measuring (see
:ref:`Timer Resolution <benchmarks_latency:timer resolution>`). The chart controls and the method shared with the
other benchmarks are on the :ref:`benchmarks page <benchmarks:reading the results>`, each library's settings in the
:doc:`library notes <benchmarks_library_notes>`.

Single Call
-----------

One log call every 100 to 300 µs, timed on its own: the scenario closest to an application that logs between
other work, as every sample is the first call after a quiet gap.

- **Payloads and threads:** the three payloads, with one thread and with four threads logging at the same moments.
- **Samples:** 100,000 per thread, after 1,000 warm-up calls.
- **Timer only:** the fastest libraries sit close to it, so their own cost is the small difference above the line,
  and differences of a nanosecond or two between them are within run-to-run noise.
- **Between samples** the calling thread waits in counted loops, checking the timestamp counter between them, and
  polls it continuously only for the last 16 to 20 µs. Polling for the whole gap left the branch predictor in a
  state an application does not log from: a branch of the next log call was mispredicted on every sample, adding
  5 to 9 ns to some libraries and nothing to others.

.. include:: benchmark_tables/latency_single.inc

Burst of 20 Calls
-----------------

20 log calls back to back, every 2 to 2.2 ms. Each sample is the average of the 20 calls, which spreads the cost
of reading the timestamp counter over them and shows the steady cost of a log call in a hot loop.

- **Payloads and threads:** the three payloads, with one thread and with four threads logging at the same moments.
- **Samples:** 10,000 per thread, after 100 warm-up bursts.
- **Tails:** the average also divides a slow call by 20, so the 99th and 99.9th percentiles look better than any
  single call; the :ref:`single call <benchmarks_latency:single call>` scenario shows those.
- **Quill's bounded dropping queue** is given 512 KiB per thread in every latency scenario. At its default of
  128 KiB it dropped messages in the bursts of complex types on Windows whenever its backend fell behind, as a
  dropping queue is meant to, and runs that lose a message are not published.

.. include:: benchmark_tables/latency_burst.inc

Cold Cache
----------

One log call after other work has pushed the logger out of the cache, which is common in applications that log
rarely.

- **Before each call** the calling thread reads a buffer four times the largest L2 cache the harness finds, at
  least 4 MiB: 5 MiB on Linux Intel (1.25 MiB of L2), 8 MiB on Windows (the i5-12600K's E-cores have 2 MiB of L2,
  although the benchmarks run on its P-cores) and 4 MiB on AMD (1 MiB of L2). The logger's data then comes from
  the L3 cache or memory. The harness checks this before each run: a line read before the sweep reloads in about
  8 ns on the Intel CPUs, an L3 hit, but in about 2 ns on the AMD one, an L2 hit, so on AMD the sweep leaves more
  in L2 and the calls start less cold.
- **Kept warm:** the top of the calling thread's stack and the call's arguments, which a running thread always
  has in its cache, are read back before the call is timed. Everything that belongs to the logger stays cold,
  including the benchmark's own pointer to it.
- **Payloads and threads:** the three payloads, with one thread only.
- **Samples:** 20,000, after 500 warm-up calls.

.. include:: benchmark_tables/latency_cold.inc

Disabled Statement
------------------

A debug statement while the logger's level is info, so nothing is logged: the statement loads the level, compares
it and takes a branch around the log call. One thread, logging numbers. The level does not change during the
benchmark, as in an application, so the branch is normally predicted correctly: these tables show the steady cost
of the check, never that of a mispredicted branch. g3log and Reckless are not shown: their builds here have no
runtime level check.

- **Hot:** the average of 1,000 such calls in a loop, the cost of a filtered-out statement in a hot path, from
  2,000 samples. The loop alone, the **Timer only** row, takes one or two CPU cycles per call. The libraries a few
  tenths of a nanosecond above it check the level inline, or through a call to a small function (both NanoLogs):
  they differ by a cycle or two, as much from how the compiler arranged the benchmark loop for each library as
  from the library, so their order is not a ranking. Three libraries do more than an inline check:

  - BqLog's fast mode macros leave the inline path for two function calls.
  - spdlog's log call is a function: its arguments are passed and the function is called before the level is
    checked.
  - Boost.Log has no level to compare: it locks the logger, opens a record and evaluates the severity filter of
    the core, which rejects it.

- **Cold:** one call after the :ref:`cache sweep <benchmarks_latency:cold cache>`, from 20,000 samples: the level
  comes from the L3 cache and the branch is still predicted correctly. Most of each number is the cost of timing
  one call.
- **Expensive Argument:** the hot loop with an argument that takes tens of nanoseconds to compute; the
  **Argument** column says whether the library computed it. A library that checks the level before evaluating the
  arguments, in a macro or a stream expression, skips the computation. A library whose log call is a function
  evaluates its arguments first and pays for them on every call, logged or not: Quill's macro-free functions,
  spdlog and BqLog. In place of Timer only, this table has an **Argument only** row: the same loop computing the
  argument without a logger.

.. include:: benchmark_tables/latency_disabled.inc

Timer Resolution
----------------

Every sample is timed with two reads of the CPU's timestamp counter, with fences that keep the calls between
them. The calls of a sample, a fixed number for each scenario, run in a function of their own, so nothing of the
benchmark itself is read or decided between the two reads. The **Timer only** row is this cost.

How many calls a sample averages sets how finely its table resolves time:

- **One call per sample** (single call, cold cache and disabled cold). On the Intel CPUs the counter advances
  every few tenths of a nanosecond, so one call is resolved to well under a nanosecond, but it varies by about a
  nanosecond between runs. These tables show whole nanoseconds, as a tenth would suggest a precision the
  measurement does not have.
- **Many calls per sample** (the burst of 20 calls, and the disabled hot and expensive argument scenarios of
  1,000 calls). The average resolves tenths of a nanosecond, so these tables keep one decimal.

On AMD Zen CPUs, Zen 4 and Zen 5 alike, including the Ryzen 9 9950X here, the counter advances in steps of about 10 ns: two
reads a few nanoseconds apart return the same value or values 10 ns apart.

- **One call per sample:** a log call of 15 to 30 ns reads as 20, 30 or 40 ns, so the fastest libraries cannot be
  told apart. These tables move in 10 ns steps and carry a note: read them for the large differences, not the
  small ones.
- **Many calls per sample:** resolved finely on AMD too, as a 10 ns step changes the result by at most 0.5 ns.
