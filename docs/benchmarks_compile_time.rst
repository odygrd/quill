.. title:: C++ Logging Compile Time Benchmark

.. meta::
   :description: How long C++ code full of Quill log statements takes to compile with GCC, Clang and MSVC, with interactive compile-time profiles in Speedscope.

.. _benchmarks_compile_time:

Compile Time
============

Logging should not slow down your build. These are clean ``Release`` builds of
`BENCHMARK_quill_compile_time <https://github.com/odygrd/quill/blob/master/benchmarks/compile_time/compile_time_bench.cpp>`__,
which compiles ``2000`` auto-generated log statements with varied argument types. Each build runs alone with
``-j4``, as C++20, and each time is the median of three clean builds.

.. md-tab-set::
   :class: bench-tabs bench-tabs-platform

   .. md-tab-item:: Linux (GCC)

      Measured on the Linux system described on the :doc:`benchmarks page <benchmarks>` with ``-march=x86-64-v3``,
      on four CPU cores kept free of other work. Clang builds additionally enable ``-ftime-trace``.

      With C++17 instead of the default C++20, the same builds take 64.80 s with GCC and 38.33 s with Clang.

      .. list-table::
         :header-rows: 1
         :class: bench-table bench-right

         * - Compiler
           - Clean Build Time
           - Benchmark Binary
           - Main TU Object
         * - ``clang 17.0.6``
           - ``40.61 s``
           - ``6.88 MB``
           - ``10.38 MB``
         * - ``gcc 14.2.1``
           - ``66.13 s``
           - ``6.16 MB``
           - ``9.76 MB``

   .. md-tab-item:: Windows (MSVC)

      Measured on the Windows system described on the :doc:`benchmarks page <benchmarks>` with ``/arch:AVX2`` and
      CMake's default MSVC flags. This is a different machine, so the times are not directly comparable with the
      Linux ones.

      .. list-table::
         :header-rows: 1
         :class: bench-table bench-right

         * - Compiler
           - Clean Build Time
           - Benchmark Binary
           - Main TU Object
         * - ``MSVC 19.44``
           - ``12.49 s``
           - ``4.62 MB``
           - ``21.88 MB``

**Why log statements compile quickly:** Quill's macros pass call-site metadata (file, line, format string and
tags) in a ``MacroMetadata`` argument instead of the template's identity, so log statements with the same argument
types share one ``log_statement`` instantiation however their metadata differs.

Compile-Time Profiles
---------------------

These profiles show where the compiler spends its time, from Clang's ``-ftime-trace`` on Linux and C++ Build
Insights on Windows. Show a profile here, then drag to zoom and click a frame for details, or open it full screen
in Speedscope.

**Header include profile:** the headers a file that logs pulls in, following the
`recommended_usage <https://github.com/odygrd/quill/blob/master/examples/recommended_usage/recommended_usage.cpp>`__
example.

.. md-tab-set::
   :class: bench-tabs bench-tabs-platform

   .. md-tab-item:: Linux (Clang)

      .. raw:: html

         <div class="speedscope-frame" data-src="https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/master/docs/traces/recommended_usage.cpp.json" data-title="Speedscope profile recommended_usage.cpp.json"><button type="button" class="speedscope-load">Show the interactive profile</button></div>
         <p class="speedscope-open"><a href="https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/master/docs/traces/recommended_usage.cpp.json" target="_blank" rel="noopener">Open in Speedscope ↗</a></p>


   .. md-tab-item:: Windows (MSVC)

      .. raw:: html

         <div class="speedscope-frame" data-src="https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/master/docs/traces/msvc_recommended_usage.cpp.json" data-title="Speedscope profile msvc_recommended_usage.cpp.json"><button type="button" class="speedscope-load">Show the interactive profile</button></div>
         <p class="speedscope-open"><a href="https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/master/docs/traces/msvc_recommended_usage.cpp.json" target="_blank" rel="noopener">Open in Speedscope ↗</a></p>


**Compile-time benchmark:** compiling the
`2000 auto-generated log statements <https://github.com/odygrd/quill/blob/master/benchmarks/compile_time/compile_time_bench.cpp>`__.

.. md-tab-set::
   :class: bench-tabs bench-tabs-platform

   .. md-tab-item:: Linux (Clang)

      .. raw:: html

         <div class="speedscope-frame" data-src="https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/master/docs/traces/compile_time_bench.cpp.json" data-title="Speedscope profile compile_time_bench.cpp.json"><button type="button" class="speedscope-load">Show the interactive profile</button></div>
         <p class="speedscope-open"><a href="https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/master/docs/traces/compile_time_bench.cpp.json" target="_blank" rel="noopener">Open in Speedscope ↗</a></p>


   .. md-tab-item:: Windows (MSVC)

      .. raw:: html

         <div class="speedscope-frame" data-src="https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/master/docs/traces/msvc_compile_time_bench.cpp.json" data-title="Speedscope profile msvc_compile_time_bench.cpp.json"><button type="button" class="speedscope-load">Show the interactive profile</button></div>
         <p class="speedscope-open"><a href="https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/master/docs/traces/msvc_compile_time_bench.cpp.json" target="_blank" rel="noopener">Open in Speedscope ↗</a></p>


To generate these profiles yourself with Clang:

.. code-block:: bash

   cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release \
     -DQUILL_BUILD_BENCHMARKS=ON -DQUILL_ENABLE_TIME_TRACE=ON \
     -DCMAKE_CXX_FLAGS='-march=x86-64-v3' ..
   cmake --build . --target BENCHMARK_quill_compile_time -j 4
   # Load the resulting .cpp.json files into https://www.speedscope.app

MSVC has no ``-ftime-trace``: capture its profiles with ``vcperf``, the C++ Build Insights tool included with
Visual Studio. Speedscope cannot open vcperf's trace directly, so convert it with
`scripts/convert_vcperf_trace.py <https://github.com/odygrd/quill/blob/master/scripts/convert_vcperf_trace.py>`__,
which also removes the environment variables vcperf records. From a Visual Studio x64 developer prompt:

.. code-block:: powershell

   cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DQUILL_BUILD_BENCHMARKS=ON `
     "-DCMAKE_CXX_FLAGS=/DWIN32 /D_WINDOWS /EHsc /GR /arch:AVX2" ..
   vcperf /start /noadmin /level3 quill
   cmake --build . --target BENCHMARK_quill_compile_time -j 4
   vcperf /stop /templates quill /timetrace compile_time_bench.raw.json
   python ..\scripts\convert_vcperf_trace.py compile_time_bench.raw.json compile_time_bench.cpp.json
   # Load compile_time_bench.cpp.json into https://www.speedscope.app
