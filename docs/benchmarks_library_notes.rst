.. title:: C++ Logging Libraries Compared: Notes and Verdict

.. meta::
   :description: How Quill, spdlog, fmtlog, BqLog, XTR, PlatformLab NanoLog, Iyengar NanoLog, MS BinLog, Reckless, g3log and Boost.Log work, their strengths and drawbacks, and how each was set up for the benchmarks.

.. _benchmarks_library_notes:

Library Notes and Verdict
=========================

These notes describe how each library works and how it was set up here, so a result can be read in the context
of the work your application does, such as copying containers, waking a worker or waiting for a file write. They
are checked against the library versions in `logger_benchmarks <https://github.com/odygrd/logger_benchmarks>`__
whenever the benchmarks are updated.

At a Glance
-----------

The state of each library in November 2026.

- **Status:** *active* means a real code change in the last 12 months, *low activity* in the last three years,
  and *unmaintained* none for three years or more, or an archived repository.
- **Platforms:** Reckless no longer builds with the current Windows SDK and Iyengar NanoLog does not compile with
  MSVC.
- **Output:** binary logs need the library's own decoder to read.
- **Full queue** and **Idle backend** show each library's default; the benchmarks change some of them, as each
  library's notes list.

.. list-table::
   :header-rows: 1
   :class: bench-glance

   * - Library
     - Status
     - Platforms
     - Output
     - Formats on
     - Containers
     - Full queue
     - Idle backend
     - Waiting flush
   * - `Quill <https://github.com/odygrd/quill>`__
     - Active
     - Linux · Windows · macOS
     - Text, JSON
     - Backend
     - Yes
     - Grows, then blocks; bounded or dropping options
     - Sleeps 500 µs
     - Yes
   * - `spdlog <https://github.com/gabime/spdlog>`__
     - Active
     - Linux · Windows · macOS
     - Text
     - Calling thread
     - Yes
     - Block, overwrite or drop
     - Waits for work
     - No (async)
   * - `fmtlog <https://github.com/MengRao/fmtlog>`__
     - Low activity
     - Linux · Windows
     - Text
     - Your poll thread
     - Yes
     - Drops, or blocks at compile time
     - Yours; built-in poller sleeps 1 s
     - Yes
   * - `BqLog <https://github.com/Tencent/BqLog>`__
     - Active
     - Linux · Windows · macOS
     - Text, compressed binary
     - Backend
     - No
     - Block, drop or expand
     - Sleeps up to 66 ms
     - Yes
   * - `XTR <https://github.com/choll/xtr>`__
     - Active
     - Linux · FreeBSD
     - Text
     - Backend
     - Yes
     - Blocks, or drops
     - Spins
     - Yes, with ``fsync``
   * - `PlatformLab NanoLog <https://github.com/PlatformLab/NanoLog>`__
     - Unmaintained (last code Dec 2020)
     - Linux
     - Binary
     - Offline decoder
     - No
     - Blocks
     - Polls every 1 µs
     - Yes
   * - `Iyengar NanoLog <https://github.com/Iyengar111/NanoLog>`__
     - Unmaintained (last code Mar 2017)
     - Linux
     - Text
     - Backend
     - No
     - Grows or overwrites
     - Sleeps 50 µs
     - No
   * - `MS BinLog <https://github.com/Morgan-Stanley/binlog>`__
     - Archived (last code Jul 2024)
     - Linux · Windows · macOS
     - Binary
     - Offline reader
     - Yes
     - Grows
     - Yours
     - Yes
   * - `Reckless <https://github.com/mattiasflodin/reckless>`__
     - Unmaintained (last code Apr 2021)
     - Linux
     - Text
     - Backend
     - No
     - Blocks
     - Sleeps up to 1 s
     - Yes
   * - `g3log <https://github.com/KjellKod/g3log>`__
     - Active
     - Linux · Windows · macOS
     - Text
     - Calling thread
     - No
     - Grows
     - Waits for work
     - No
   * - `Boost.Log <https://www.boost.org/doc/libs/release/libs/log/>`__
     - Active
     - Linux · Windows · macOS
     - Text
     - Calling thread
     - Per element
     - Grows; bounded options
     - Waits for work
     - Yes

Every library here is open source under a permissive license: MIT (Quill, spdlog, fmtlog, XTR, Iyengar NanoLog,
Reckless); Apache-2.0 (BqLog, MS BinLog); ISC (PlatformLab NanoLog); BSL-1.0 (Boost.Log); Unlicense (g3log).

Quill
-----

`Quill <https://github.com/odygrd/quill>`__ is an asynchronous logging library whose backend thread does more than
logging, as listed below. A log call encodes its arguments into the calling thread's own lock-free queue, and the
backend formats them and writes them in timestamp order across threads.

**Threading:** One backend thread per process, by design, with no option for more. A log call never wakes the
backend; an application can wake it at once by calling ``Backend::notify()`` after logging. Asynchronous only; the
backend can also run on a thread of your own (:doc:`ManualBackendWorker <manual_backend_worker>`).

**The good**

- Little work on the calling thread for every payload, including STL containers and user-defined types, which it
  encodes into the queue instead of formatting them.
- It logs almost anything. The ``quill/std/`` headers cover STL containers, including nested ones. For your own
  types, a ``fmtquill::formatter`` sets the text and a ``quill::Codec`` sets what goes into the queue: a copy of the
  object for deferred formatting, or just the fields the formatter needs. Formatting never runs on your thread,
  except for types you give a :ref:`DirectFormatCodec <recipes:directformatcodec>`.
- One backend for more than logs. :doc:`Metrics <metrics>` go to Prometheus through ``PrometheusSink`` or to a sink
  of your own, with each call passing only a pointer to the registered metric and a value. The same backend writes
  :doc:`CSV files <csv_writing>`, raw binary files and pcap captures that Wireshark and tcpdump open, and can
  :doc:`decode binary protocols <binary_protocols>` such as SBE into readable fields with the decoder generated for
  your schema.
- Macros or functions: with C++20, ``quill::info()`` and the other logging functions with a literal format string
  and no tags come close to the ``LOG_*`` macros in speed. A function evaluates its arguments even when its level
  is disabled, which the macros skip.
- Configurable throughout: :doc:`FrontendOptions <frontend_options>` chooses the queue (bounded or growing,
  blocking or dropping, its capacity, huge pages on Linux), :doc:`BackendOptions <backend_options>` the idle wait,
  CPU affinity, buffer limits, timestamp ordering, flushing, callbacks and character sanitization, and each logger
  its pattern and clock (the CPU timestamp counter, the system clock or your own).
- :doc:`Built-in sinks <sink_types>` for consoles, files, rotating files, JSON, syslog, the systemd journal and
  Android, plus your own, each with its own filters; tags, mapped diagnostic context, rate-limited macros, and
  backtrace logging that keeps recent messages in a ring buffer until an error.
- ``flush_log()`` waits until every message is written, and an optional signal handler writes pending messages
  on a crash.

**The bad**

- Many features, so configure them deliberately. The defaults suit most applications, but a latency-critical or
  memory-constrained one should choose its queue, backend and sink settings on purpose.
- Memory is limited at both ends. The default unbounded queue grows during a burst, up to
  ``unbounded_queue_max_capacity`` (2 GiB per thread by default, configurable), then blocks, or drops with a
  dropping queue. The backend caps its own buffering with ``transit_events_soft_limit`` and
  ``transit_events_hard_limit``. After a burst, a thread keeps the largest queue it grew to until it calls
  ``Frontend::shrink_thread_local_queue()``; a bounded queue keeps memory fixed from the start.
- One backend thread, so its formatting and I/O rate is the ceiling: it keeps up with high throughput from many
  producer threads, but cannot scale past it. An application that needs more splits its logging across processes.
- No separate synchronous logger: synchronous behaviour is simulated by flushing after each message with
  ``set_immediate_flush()``.

**As benchmarked**

- The backend never sleeps (``sleep_duration = 0``; the default is 500 µs). When every queue is empty it
  busy-waits with the CPU pause instruction for ``spin_wait_duration``, 10 µs by default, before reading them
  again, so it does not keep taking the queues' cache lines from the logging threads.
- Latency: the default unbounded queue, and the bounded dropping queue at
  :ref:`512 KiB per thread <benchmarks_latency:burst of 20 calls>`.
- Throughput: the default unbounded queue, which grows during the run.

spdlog
------

`spdlog <https://github.com/gabime/spdlog>`__ is the most widely used library here. Its asynchronous logger formats
the message on the calling thread, copies it into a queue shared by every thread, and a thread pool applies the
pattern and writes it.

**Threading:** Synchronous or asynchronous loggers. All producers of an asynchronous logger share one queue guarded
by one ``std::mutex``, and every log call notifies the worker's condition variable. The thread pool can run several
workers (``init_thread_pool(queue_size, n)``), which drain the same queue.

**The good**

- By far the most used, with many sinks and a simple API.
- Three documented full-queue policies: block, overwrite the oldest message, or discard the new one.
- The worker sleeps on a condition variable when idle, so it uses no CPU. The price is on the hot path: every
  log call notifies it, which can mean a system call, and the first message after a quiet spell waits for the
  worker to be woken and scheduled.

**The bad**

- It does not scale with more producer threads: they wait for each other on the one ``std::mutex`` that every
  producer and the worker share.
- Formatting happens on the calling thread, so every call pays for it, and containers most of all. A message over
  250 bytes allocates.
- More than one pool thread loses message order.
- The asynchronous ``flush()`` only queues a flush and returns; nothing waits until messages are written.

**As benchmarked**

- The default queue of 8,192 messages and one worker thread, blocking when the queue is full.
- Throughput: a file sink that works like ``basic_file_sink`` and records each completed flush, so the clock stops
  when the worker has written the last message, without timing the thread pool's shutdown.

fmtlog
------

`fmtlog <https://github.com/MengRao/fmtlog>`__ is a small {fmt}-based logger with a 1 MiB queue per thread. It has
no backend thread of its own: nothing is written until the application calls ``fmtlog::poll()``.

**Threading:** One poller for all queues: your own thread calling ``poll()``, or the thread
``startPollingThread()`` starts. Each thread logs into a queue of its own and never wakes the poller. Calling
``poll()`` yourself also lets it run synchronously in a single thread.

**The good**

- Little work on the calling thread for numbers and strings, formatted later by the poller; containers work
  through {fmt}.
- Timestamps from the CPU's timestamp counter.

**The bad**

- You write the backend yourself: run a thread that calls ``fmtlog::poll()`` in a loop and decide how it waits, or
  call ``startPollingThread()``, which polls once a second by default. Only one thread may poll.
- A full queue drops messages unless the program is compiled with ``FMTLOG_BLOCK=1``, and blocking then retries in
  a loop with no pause. The queue size is also fixed at compile time (``FMTLOG_QUEUE_SIZE``).
- One global logger and one output file.
- Its queue publishes each message with ``volatile`` and plain stores rather than atomics, so the compiler may
  reorder them: in the GCC 14 build here the message size, which tells the backend a message is ready, is stored
  before the last argument, so the backend can read that argument before it is written.
- Containers and other complex types are costly: the whole object, such as a vector and every string in it, is
  copied into the queue on the calling thread, with heap allocations, instead of being serialized into it.

**As benchmarked**

- A dedicated thread calls ``poll()`` in a loop that never sleeps.
- The default 1 MiB queue per thread, compiled with ``FMTLOG_BLOCK=1`` so a full queue blocks instead of dropping.

BqLog
-----

`BqLog <https://github.com/Tencent/BqLog>`__ is a logger from Tencent for C++ and several other languages. Threads
write into a shared lock-free buffer, or a buffer of their own once they log often enough, and a worker thread
formats them into text or compressed binary files.

**Threading:** Synchronous, asynchronous (one worker for every log in the process) or independent (one worker per
log). Threads that log rarely share one lock-free buffer and threads that log often get one of their own, as do the
fast mode macros used here; a writer only wakes the worker when a buffer passes half full.

**The good**

- Little work on the calling thread with its fast mode macros.
- Text and binary from the same log: a log object writes to any number of appenders, such as a readable
  ``text_file`` and a compact ``compressed_file`` (read with its decoder) at once, which suits devices with little
  storage or upload bandwidth; BqLog lists embedded Linux and mobile platforms among its targets.
- ``force_flush()`` waits until everything is written.

**The bad**

- Where it sleeps: in the default ``async`` mode the one worker waits up to ``log.worker_interval_ms``, 66 ms by
  default, when it finds nothing to write, and a writer only wakes it early when a buffer is half full or full.
  In a quiet process a line can reach the file 66 ms after it was logged. Setting the interval to 0 makes the
  worker spin on a core instead.
- That interval is global in ``async`` mode: the log object created last sets it for every other one.
- A thread only gets its fast per-thread buffer when it logs more than 1,000 messages per second.
- It formats with its own implementation, not {fmt}: ``{}`` placeholders follow C++20 ``std::format`` rules
  without positional indexes or time formatting, so {fmt} formatters written for your own types do not apply.
- No built-in support for STL containers or your own types: for each type you write two functions
  (``bq_log_format_str_size()`` and ``bq_log_format_str_chars()``) that turn the value into a string, which BqLog
  copies into the log when you make the call.
- The default text line has millisecond timestamps and no file and line.
- Settings are a configuration string, so a misspelt key is silently ignored.
- Its lowest calling-thread cost needs the C++ fast mode macros, and a fast mode call site is bound to the log
  object and format string of its first call, so it cannot be used inside a helper that logs through different
  objects.

**As benchmarked**

- Its C++ fast mode macros (``BQ_LOG_FAST_INFO``), its fastest way to log, which write the same entry as
  ``log_obj.info()`` with much less work on the calling thread. These results depend on them; the normal mode is
  slower.
- The worker never sleeps (``log.worker_interval_ms=0``; the default is 66 ms).
- The default buffer size.

XTR
---

`XTR <https://github.com/choll/xtr>`__ is a C++20 logger for Linux and FreeBSD. Each thread logs through its own
sink, a 256 KiB queue by default, and a background thread formats the messages with {fmt}.

**Threading:** One consumer thread per logger object, so more consumers means more logger objects, each with its
own output. Each sink, one per thread here, has a queue of its own, and a log call never wakes the consumer, which
spins. Asynchronous, with the option to drive the consumer from your own thread (``pump_io()``).

**The good**

- Little work on the calling thread for numbers and strings, and a fast consumer.
- Logs containers through {fmt}, and offers io_uring output.

**The bad**

- Its background thread never sleeps: ``consumer::run`` calls ``run_once`` in a loop with no idle wait, so it
  keeps a CPU core fully busy even when nothing is logged.
- Containers and other complex types are costly: the whole object, such as a vector and every string in it, is
  copied into the queue on the calling thread, with heap allocations, instead of being serialized into it.
- Linux and FreeBSD only, so it is absent from the Windows results.
- A full sink blocks the caller (``XTR_TRY_LOG`` drops instead).
- The sink queue size is fixed when the library is compiled (``XTR_SINK_CAPACITY``).
- ``sync()`` always calls ``fsync``, so there is no way to wait for the data to be written without waiting for the
  disk.

**As benchmarked**

- ``XTR_LOGL_TSC``, which reads the CPU timestamp counter on the calling thread, the default 256 KiB sink, and
  io_uring turned off, so it writes with plain ``write`` calls like every other library.
- Throughput: each producer thread has its own sink, and the log file's ``sync()`` does not call ``fsync``, so the
  clock stops once the data is handed to the operating system, as for every other library.

PlatformLab NanoLog
-------------------

`PlatformLab NanoLog <https://github.com/PlatformLab/NanoLog>`__ takes ``printf``-style calls and writes a compressed
binary log that its decompressor turns into text. It is built here from its C++17 runtime, without its
preprocessor.

**Threading:** One background thread per process compresses and writes everything. Each thread logs into a staging
buffer of its own and never wakes the backend. Asynchronous only.

**The good**

- Little work on the calling thread for numbers and strings, which it copies into the staging buffer.
- Very compact logs: it writes each format string once and packs the arguments and timestamp differences. The
  file carries its own dictionary, so its decompressor turns it into text offline without the program.

**The bad**

- A ``printf``-style API: every argument needs a conversion that matches its exact type (``%lu`` for a
  ``size_t``, ``%d`` for an ``int``, ``%s`` for a string), written out in every call, and there is no way to log
  your own types or containers.
- Binary output that needs the decompressor.
- Buffer sizes and the poll interval are constants in its source, not settings.
- Linux only.

**As benchmarked**

- It opens its log with ``O_DSYNC``, so every write waits for the disk.
- Its throughput is listed as binary output and left out of the throughput chart.

Iyengar NanoLog
---------------

`Iyengar NanoLog <https://github.com/Iyengar111/NanoLog>`__ is a small stream-style logger that copies arguments into
a buffer for a background thread.

**Threading:** One background thread per process. All threads share the logger's queue, synchronized with atomic
operations and a spinlock, and never wake the worker, which sleeps 50 µs when the queue is empty. Asynchronous
only.

**The good**

- A small, simple code base.

**The bad**

- Producers contend for the one shared queue and its spinlock as threads are added, which raises latency and
  lengthens the tails.
- The guaranteed logger allocates a new buffer from the heap when the current one fills.
- No flush call, so a program cannot wait until its messages are written.
- No ``std::vector`` arguments, and it does not compile with MSVC.

**As benchmarked**

- The guaranteed logger, which never drops a message.
- Not in the throughput tables: without a flush call its elapsed time cannot cover every message.

MS BinLog
---------

`MS BinLog <https://github.com/Morgan-Stanley/binlog>`__ writes a structured binary log. Each thread has a 1 MiB
queue, the application runs the loop that moves log data from the queues to the file, and the ``bread`` tool turns
the file into text.

**Threading:** No backend thread: the application calls ``consume()`` when and on whichever thread it likes, so it
can also write right after logging, and separate sessions can be consumed in parallel. Each thread writes into a
channel of its own and never wakes the consumer.

**The good**

- Little work on the calling thread for a structured log, containers included, which it serializes rather than
  formats.
- Compact logs that keep each record's timestamp, thread, severity and source location, for devices with little
  storage or upload bandwidth, such as an in-vehicle recorder. ``bread`` converts them to text later, sorted by
  time, without the program or a schema.

**The bad**

- The log cannot be read without ``bread``.
- The application must run and pace the consume loop itself.
- A full queue neither blocks nor drops: the writer allocates a larger queue, so memory grows without limit when
  the consumer falls behind.

**As benchmarked**

- The consume loop never sleeps, the setting every library's backend is given.
- Its throughput is listed as binary output and left out of the throughput chart.

Reckless
--------

`Reckless <https://github.com/mattiasflodin/reckless>`__ formats on its background thread from arguments copied on
the calling thread.

**Threading:** One worker thread per log object. All threads share the log's lock-free buffer, reserving space with
a compare-and-swap on the same position, and a producer only wakes the worker when the buffer is full. Asynchronous
only.

**The good**

- Its worker sleeps when idle, and a full buffer blocks the caller on an event instead of spinning.
- A crash handler that writes pending messages.

**The bad**

- All threads share one buffer.
- A ``printf``-like API with its own, smaller set of conversions: integers take ``%d``, characters and strings
  ``%s``, so existing format strings often need changing, and a wrong conversion is only found when the line is
  written.
- No runtime log level filter, and no ``std::vector`` arguments.
- Not built on Windows here, because it conflicts with the current Windows SDK headers.

**As benchmarked**

- The default buffer sizes.
- A failed write blocks until it succeeds; by default Reckless ignores temporary write errors. No write fails here.

g3log
-----

`g3log <https://github.com/KjellKod/g3log>`__ builds each message with a ``std::ostringstream`` on the calling thread
and passes it to a background worker.

**Threading:** One dispatcher thread, plus one thread per sink. All producers share one queue guarded by one
``std::mutex``, and every log call notifies the worker. Asynchronous only.

**The good**

- Catches fatal signals and ``LOG(FATAL)`` and writes every pending message before the process exits.

**The bad**

- Every call pays for formatting with a stream on the calling thread.
- It slows down as threads are added: all producers share one locked queue and notify the worker on every call.
- Its queue is unbounded and has no full-queue policy.
- No flush call.
- No ``std::vector`` arguments.

**As benchmarked**

- Its default worker and file sink.
- Not in the throughput tables: without a flush call its elapsed time cannot cover every message.

Boost.Log
---------

`Boost.Log <https://www.boost.org/doc/libs/release/libs/log/>`__ is a very flexible framework of attributes, filters,
formatters and sinks, used here with an asynchronous sink.

**Threading:** Synchronous, unlocked or asynchronous sinks. Each asynchronous sink has a feeding thread of its own,
so separate sinks process records in parallel, but all threads share each sink's queue, and with the default
unbounded queue every record signals the sink's event.

**The good**

- Part of Boost, and the most configurable library here.
- Its asynchronous sink can use an unbounded queue (the default) or a bounded one that blocks or drops, and
  ``flush()`` waits.

**The bad**

- Every call pays for building and streaming the record on the calling thread.
- No direct ``std::vector`` support; its benchmark writes each element into the record.
- All threads share each asynchronous sink's queue, and the default queue signals the sink's event on every record.

**As benchmarked**

- An asynchronous sink with the default unbounded queue, fed by a thread pinned to the backend CPU, and the
  thread-safe ``severity_logger_mt`` shared by every logging thread.

Verdict
-------

For a latency-sensitive application that wants readable logs, :ref:`Quill <benchmarks_library_notes:quill>` is the
strongest general-purpose choice here:

- **Latency:** with complex types, a vector of strings, it is first on every platform and in every scenario,
  by a wide margin. With numbers and strings it is first or within about a nanosecond of the fastest in the single
  call and burst scenarios on Linux, and first in most of them on Windows; on AMD the fastest libraries tie within
  its 10 ns timer steps. :ref:`fmtlog <benchmarks_library_notes:fmtlog>` and :ref:`XTR <benchmarks_library_notes:xtr>`
  match it for plain numbers but take many times longer for a vector of strings, which real applications log all
  the time. In the disabled cold scenario, libraries such as fmtlog and XTR can reach their state faster
  through a global or thread-local variable. On both Linux systems they also lead Quill's default queue in
  the cold scenario with two integers and a double. Quill's calls first load the logger object held by the
  application, which is cold in these scenarios. At p90, its C++20 logging functions are within two nanoseconds
  of its macros for numbers and strings in the single-call and burst scenarios with GCC. With MSVC they cost
  several nanoseconds more. The libraries that format on the calling thread pay for it on every call:
  :ref:`g3log <benchmarks_library_notes:g3log>` takes microseconds and
  :ref:`Boost.Log <benchmarks_library_notes:boost.log>` is among the slowest in every table.
- **Throughput:** its text throughput is near the top: third on Linux, a little behind
  :ref:`XTR <benchmarks_library_notes:xtr>` and :ref:`BqLog <benchmarks_library_notes:bqlog>`, and second on
  Windows, behind :ref:`BqLog <benchmarks_library_notes:bqlog>`. Of the binary loggers,
  :ref:`MS BinLog <benchmarks_library_notes:ms binlog>` has the highest throughput with several producer threads.
- **Scope:** it runs on Linux, Windows and macOS with nothing to set up first, and the same backend also publishes
  metrics and writes CSV files, raw binary data and packet captures.

Another library fits better when:

- **You need compact logs**, for a device with little storage or upload bandwidth:
  :ref:`MS BinLog <benchmarks_library_notes:ms binlog>`,
  :ref:`PlatformLab NanoLog <benchmarks_library_notes:platformlab nanolog>` or
  :ref:`BqLog <benchmarks_library_notes:bqlog>`'s compressed log write files about two to six and a half times
  smaller, decoded to text later. For the 4 million messages of the throughput benchmark, most text loggers wrote
  about 400 MiB, MS BinLog 188 MiB and PlatformLab NanoLog 62 MiB; BqLog's compressed file was 81 MiB against
  509 MiB of its own text.
- **One logging setup must span several languages and platforms**, mobile included:
  :ref:`BqLog <benchmarks_library_notes:bqlog>`.
