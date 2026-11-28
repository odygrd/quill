.. title:: C++ Logging Library Benchmarks

.. meta::
   :description: Comprehensive C++ logging library benchmarks: call latency, throughput and compile time of Quill, spdlog, fmtlog, BqLog, XTR, NanoLog, MS BinLog, Reckless, g3log and Boost.Log on Linux and Windows.

.. _benchmarks:

Comprehensive C++ Logging Library Benchmarks
============================================

Open-source C++ logging libraries compared on Linux with Intel and AMD CPUs (GCC) and on Windows (MSVC).

.. rst-class:: quill-meta

*Benchmarks last updated: November 2026*

.. toctree::
   :hidden:

   benchmarks_latency
   benchmarks_throughput
   benchmarks_compile_time
   benchmarks_library_notes

.. rst-class:: quill-doc-cards quill-doc-cards-wide

- :si-icon:`octicons/zap-16` :doc:`Log Call Latency <benchmarks_latency>` *How long a log call takes on the calling
  thread for numbers, large strings and STL containers: one call at a time, in bursts, with a cold cache and for
  disabled statements.*
- :si-icon:`octicons/pulse-16` :doc:`Throughput <benchmarks_throughput>` *How many messages per second each library
  writes to a log file, with one and four producer threads.*
- :si-icon:`octicons/stopwatch-16` :doc:`Compile Time <benchmarks_compile_time>` *How long code full of log statements
  takes to build, with interactive compile-time profiles.*
- :si-icon:`octicons/checklist-16` :doc:`Library Notes and Verdict <benchmarks_library_notes>` *How each library works
  and its trade-offs, such as a backend thread that spins or a queue that grows without limit, so you can choose
  for your own workload rather than by one number.*

The libraries are :ref:`Quill <benchmarks_library_notes:quill>`, :ref:`spdlog <benchmarks_library_notes:spdlog>`,
:ref:`fmtlog <benchmarks_library_notes:fmtlog>`, :ref:`BqLog <benchmarks_library_notes:bqlog>`,
:ref:`XTR <benchmarks_library_notes:xtr>`, :ref:`PlatformLab NanoLog <benchmarks_library_notes:platformlab nanolog>`,
:ref:`Iyengar NanoLog <benchmarks_library_notes:iyengar nanolog>`, :ref:`MS BinLog <benchmarks_library_notes:ms binlog>`,
:ref:`Reckless <benchmarks_library_notes:reckless>`, :ref:`g3log <benchmarks_library_notes:g3log>` and
:ref:`Boost.Log <benchmarks_library_notes:boost.log>`. The benchmarks are maintained by the Quill project; their code
is in the `logger_benchmarks <https://github.com/odygrd/logger_benchmarks>`__ repository, so anyone can check the
setup of each library and reproduce the results.

Reading the Results
-------------------

- Latency is in nanoseconds and lower is better; throughput is in millions of messages per second and higher is
  better.
- A percentile is the time below which that share of calls finished: the 99th percentile is the slowest 1 in 100
  calls. The tails show how often a logger stalls your thread, which the median hides.
- Differences of a few nanoseconds at the top of the latency tables are within run-to-run noise.
- Compare libraries within one platform, not one platform with another: the Windows machine is not tuned for
  benchmarking.
- Each chart is drawn from the table below it. Latency tables are sorted by the 90th percentile and throughput
  tables by messages per second. Hover over a library for all its numbers, click it to highlight it in every
  chart, and use the buttons to change the percentile, the view or the scale of all charts at once. Tabs with
  the same name switch together across the page.

How the Benchmarks Are Run
--------------------------

- **Same settings**, where a library has them: a backend thread that never sleeps, so no library pays for waking
  up; its default queue size, blocking when the queue is full so no message is dropped; and timestamps read on the
  calling thread. The one exception is Quill's bounded dropping queue, a variant shown next to its default queue,
  which is given :ref:`512 KiB per thread <benchmarks_latency:burst of 20 calls>`. The
  :doc:`library notes <benchmarks_library_notes>` list what this changes from each library's defaults.
- **Same work**: every library logs the same messages, with the log pattern closest to time, thread, file and
  line, level and message that it offers. Some cannot print every field: BqLog and Reckless, for example, have no
  file and line. Quill's default pattern pads its fields to line them up, which the other libraries do not offer,
  so it is not used here.
- **Pinned threads**: the calling threads and the backend thread each run on their own CPU core.
- **Warm-up**: every thread logs before timing starts, and the first samples of each run are discarded, so
  creating queues and first-time costs are not measured.
- **Three runs**: the whole suite runs three times, each time in a random order, and every table shows each
  library's best run: the lowest 90th, then 95th, 99th and 99.9th percentile for latency, the highest throughput.
  The same rule applies to every library.
- **Every message checked**: after each run the lines of every text log file are counted, and a run that lost a
  message is not published.
- **Pinned versions**: every library is built from the version recorded in logger_benchmarks'
  `vendor_manifest.json <https://github.com/odygrd/logger_benchmarks/blob/master/vendor_manifest.json>`__, all with
  the same compiler and flags, listed for each platform below.

System Configuration
--------------------

.. md-tab-set::
   :class: bench-tabs bench-tabs-platform

   .. md-tab-item:: Linux Intel (GCC)

      - **Quill Version:** v14.0.0
      - **OS:** Linux RHEL 9.4
      - **CPU:** Intel Core i5-12600 (12th Gen) @ 4.8 GHz
      - **Compiler:** GCC 14.2
      - **Build:** C++20, Release with ``-m64 -march=x86-64-v3``
      - **Benchmark-Tuned System:** CPUs 1 to 5 isolated and interrupts on CPU 0, as the command line shows.

      - **Command Line Parameters:**

        .. code-block:: shell

           $ cat /proc/cmdline
           BOOT_IMAGE=(hd0,gpt2)/vmlinuz-5.14.0-611.47.1.el9_7.x86_64+rt root=/dev/mapper/rhel-root ro crashkernel=1G-4G:192M,4G-64G:256M,64G-:512M resume=/dev/mapper/rhel-swap rd.lvm.lv=rhel/root rd.lvm.lv=rhel/swap rhgb quiet nohz=on nohz_full=1-5 rcu_nocbs=1-5 rcu_nocb_poll isolcpus=domain,managed_irq,1-5 mitigations=off transparent_hugepage=never intel_pstate=disable nosoftlockup irqaffinity=0 skew_tick=1 audit=0 nmi_watchdog=0 nowatchdog spec_store_bypass_disable=off spectre_v2=off iommu=pt

   .. md-tab-item:: Linux AMD (GCC)

      - **Quill Version:** v14.0.0
      - **OS:** AlmaLinux 10.3, kernel 6.12
      - **CPU:** AMD Ryzen 9 9950X (Zen 5), 16 cores on two CCDs of 8, up to 5.7 GHz
      - **Memory:** 192 GB DDR5 (4 x 48 GB at 3600 MT/s)
      - **Compiler:** GCC 14.4
      - **Build:** C++20, Release with ``-m64 -march=x86-64-v3``
      - **Benchmark-Tuned System:** Tuned like the Intel system: SMT off, CPUs 1 to 7 of the first CCD isolated, the
        ``performance`` policy of ``amd-pstate-epp`` with boost on, and interrupts on CPU 0. The calling threads, the
        backend thread and the main thread share the first CCD and its L3 cache. BIOS settings such as C-states are
        at their defaults.
      - **Timer Resolution:** Its timestamp counter advances in steps of about 10 ns, too coarse to tell the fastest
        libraries apart on one call. Every latency scenario is shown for it; the tables that time one call per
        sample (single call, cold cache and disabled cold) carry a note. See
        :ref:`Timer Resolution <benchmarks_latency:timer resolution>`.

      - **Command Line Parameters:**

        .. code-block:: shell

           $ cat /proc/cmdline
           BOOT_IMAGE=(mduuid/81a688fce98e3e999fdb2306660fe8d0)/boot/vmlinuz-6.12.0-264.16.1.el10_3.x86_64 root=UUID=9df65bc8-856c-4edb-a01c-a5d5092d0714 ro rd.md.uuid=81a688fc:e98e3e99:9fdb2306:660fe8d0 nosmt isolcpus=domain,managed_irq,1-7 nohz_full=1-7 rcu_nocbs=1-7 rcu_nocb_poll irqaffinity=0 mitigations=off transparent_hugepage=never nosoftlockup skew_tick=1 audit=0 nmi_watchdog=0 nowatchdog

   .. md-tab-item:: Windows Intel (MSVC)

      - **Quill Version:** v14.0.0
      - **OS:** Windows 11 Home 26H2 (build 26300)
      - **CPU:** Intel Core i5-12600K (12th Gen), 6 P-cores + 4 E-cores, up to 4.9 GHz
      - **Compiler:** MSVC 19.44 (Visual Studio 2022)
      - **Build:** C++20, Release with ``/O2 /arch:AVX2``
      - **Not Tuned for Benchmarking:** No isolated CPUs and the default Balanced power plan, so the 99th and 99.9th
        percentiles are noisier than on the tuned Linux systems. The benchmarks and the method are the same as on
        Linux; they run at high priority, with every benchmark thread pinned to its own P-core (no SMT siblings, no
        E-cores).
      - **Not Built on Windows:** XTR (Linux and FreeBSD only), PlatformLab NanoLog (Linux only), Iyengar NanoLog
        (does not compile with MSVC) and Reckless (conflicts with the current Windows SDK headers).
