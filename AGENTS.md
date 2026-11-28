# Agent Instructions

## Project Summary

Quill is a C++17 asynchronous low-latency logging library.

Main design goals:

- keep frontend logging overhead as low as possible,
- move formatting and I/O to a backend worker thread,
- preserve timestamp ordering across threads,
- provide a practical API without compromising hot-path performance.

The project is performance-sensitive. Many design choices prioritize low frontend latency over extra convenience or
extra validation at every call site.

## Repository Shape

Important directories:

- `include/quill/`: public headers and most implementation.
- `docs/`: Sphinx docs and API guidance.
- `test/unit_tests/`: focused component tests.
- `test/integration_tests/`: end-to-end behavioral tests.
- `examples/`: usage examples.
- `benchmarks/`: compile-time and runtime benchmarks.

## Core Architecture

The library has two main parts:

1. Frontend

- User threads log via macros or macro-free APIs.
- Each thread writes into its own thread-local SPSC queue.
- The hot path serializes arguments and stores metadata references.

2. Backend

- A single backend worker thread polls frontend queues.
- It orders events by timestamp.
- It performs formatting and sink I/O.

When reviewing or changing code, always ask:

- Is this on the frontend hot path or backend path?
- Does this add cost to every log statement?
- Does this move work from backend to frontend?
- Does this change a documented user contract?
- Before fixing a "bug", trace all call sites — the invariant may be maintained by callers rather than the function
  itself.

## Reviewing

- Report defects in realistic use, including rare cases and behavior the documentation leaves unspecified.
  For an edge case, explain what could trigger it in a real application and what the impact would be.
- Weigh the likelihood and impact of an edge case against the complexity of fixing it. A highly unlikely,
  low-impact scenario may be worth noting without requiring a fix.
- Prefer the simplest fix.
- Also say what can be removed or simplified, not only what is missing.
- A fix must not change what user code sees, for example the file name that a custom sink's constructor receives.

## Important Contracts

- Single backend worker thread.
- `Logger` objects are thread-safe for logging.
- Logger configuration is effectively immutable after creation; recreate a logger instead of mutating it in place.
- `Frontend::remove_logger_blocking()` must only be used while the backend is running.
- Once a logger is removed, that `Logger*` must not be used again.
- Removing the same logger from multiple threads is unsupported.
- The project supports both exception and no-exception builds.
- Ideally used as a static library, but some users integrate it through shared-library / `.so` boundaries. Keep
  shared-library pitfalls in mind as well, especially singleton lifetime, duplicated state across DSOs, symbol
  visibility, and shutdown behavior.
- If you change public behavior, keep headers, docs, tests, and changelog aligned.

## Layering

Respect the library layering:

- Avoid introducing dependencies from lower-level code to higher-level code.
- Flag dependency inversions in reviews.
- `quill/core` is part of the frontend-facing surface and should remain self-contained: it may depend on `quill/core`
  and `quill/bundled`, but not on other layers.
- For normal user-side logging, the intended lightweight include surface is `quill/Logger.h` and `quill/LogMacros.h`. Be
  very careful with every include reachable from those headers, including standard library headers. Any extra `#include`
  there propagates into user translation units and increases compile-time cost and header pollution.

## Updating Bundled libfmt

The bundled fmt copy under `include/quill/bundled/fmt/` is vendored source with Quill-local patches. Treat each upgrade
as two separate jobs: first identify the upstream delta, then port the Quill-specific changes.

- Do not modify the bundled fmt sources outside an upgrade: every local edit has to be found and ported again on the
  next one. To silence a warning that fmt triggers under Quill's build flags, add a diagnostic suppression next to the
  existing ones in `format.h` (or `base.h`) instead of changing fmt code. Make any other edit only when there is no
  alternative, keep it minimal, and list it below so the next upgrade ports it.

- Identify the current bundled fmt version from `FMTQUILL_VERSION` in `include/quill/bundled/fmt/base.h`; the value uses
  fmt's `major * 10000 + minor * 100 + patch` encoding. Pick the target version from the official fmt releases at
  https://github.com/fmtlib/fmt/releases.
- Download and unpack the official current and target fmt release archives outside `include/quill/bundled/fmt/`. Compare
  the official current release with Quill's bundled tree to find local patches, then compare the target release with the
  official current release to understand upstream changes.
- Replace the vendored fmt files from the official target release, then run:
  `python3 scripts/rename_libfmt.py include/quill/bundled/fmt FMTQUILL fmtquill`
- Preserve upstream formatting in bundled fmt files. Do not run clang-format or broad formatting tools over vendored fmt;
  port Quill patches as the smallest necessary edits.
- Reapply and re-check Quill-local changes that still apply to the target fmt version:
  - In `base.h`, force header-only mode with `FMTQUILL_HEADER_ONLY`.
  - In `base.h`, keep the end-of-file `format.h` include removed or disabled when it is only included because
    `FMTQUILL_HEADER_ONLY` is set.
  - Keep the buffer `append` fast path that uses `memcpy(ptr_ + size_, begin, count * sizeof(T))` when
    `std::is_same<T, U>::value`, with the element-wise copy fallback when the types differ.
  - In `chrono.h`, keep the `std::micro` suffix as `"us"` unless upstream already matches that behavior.
  - In `format.h`, retain only the warning suppressions still needed by supported compilers, such as GCC/Clang
    `-Wfloat-equal`, `-Wsign-conversion` (the BSD `FILE` buffer code in `format-inl.h`) and GCC
    `-Wstringop-overflow` false positives.
  - Keep `is_fast_float<T>::value` in places where supported compilers require the type-trait form; if upstream changes
    this code, confirm with a targeted compile instead of applying a blind search-and-replace.
  - Search the previous local diff for any other Quill-specific compiler workarounds, diagnostics, ABI/header-only
    changes, namespace changes, or performance fixes and port the ones that remain applicable.
- After porting, inspect the diff to confirm it contains only the upstream fmt update plus intentional Quill-local
  patches, then run the narrowest relevant build or test target for the touched code.

## Updating Benchmark Results

Before a release, refresh the benchmark pages (`docs/benchmarks*.rst`) with results from
[logger_benchmarks](https://github.com/odygrd/logger_benchmarks). Every table, chart and trace must come from the
release being published. Do the steps in order, and never run two benchmarks or builds at the same time: compiler
activity distorts latency and throughput results.

- Update logger_benchmarks first, on a branch that is merged before results are published:
  - Update the other libraries with `tools/update_vendor.py` (pins in `vendor_manifest.json`): the latest release, or
    the latest `master` for libraries without releases. Build everything to confirm it compiles.
  - Vendor Quill by copying `include/` and the CMake files from the Quill commit being released into `quill/`,
    following the previous "Update Quill" commit, and update its pin.
  - Port changes from Quill's `benchmarks/` to the matching logger_benchmarks benchmarks. Keep every library on the
    same workload: when the Quill throughput loop changes, change it for every library.
  - Quill's `benchmarks/hot_path_latency` uses the same harness as logger_benchmarks'
    `benchmarks/call_site_latency` (`hot_path_bench.h` follows `call_site_latency_bench.h` and
    `call_site_latency_bench_common.h`). A change to either harness goes to both, so the in-repo numbers match the
    published ones.
  - BqLog benchmarks use its C++ fast mode macros (`BQ_LOG_FAST_INFO`).
  - Keep the equal-settings rule for every library, Quill included: a backend that never sleeps where it can be set,
    each library's default queue type and size, blocking rather than dropping where the library offers it, and
    timestamps read on the calling thread where it offers that. Every library logs the same messages with the same
    pattern (the throughput statements come from `tools/gen_many_statements.py`). The one exception is Quill's
    bounded dropping variant, which is sized at 512 KiB per thread (`benchmark_quill_frontend_options.h`): at the
    default 128 KiB it drops messages in the complex types burst on Windows. The docs state this exception.
  - After any change to the harness (`call_site_latency_bench*.h`, `backend_total_time/utils.h`) or to how a
    library is set up, have it reviewed independently before running: logging calls the compiler can remove or move
    out of the timed region (read the disassembly of the disabled and single-call loops), timing and percentile
    maths, the cache sweep, and whether every library's clock stops only after all its messages are written.
  - Keep these properties of the latency harness, which earlier versions broke and which made some libraries look
    faster or slower than they are:
    - The timed calls run in their own never-inlined function, `bench_timed_calls<CallsPerSample>`, with the number
      of calls a compile-time constant, so no harness value is read from the stack between the two timestamps.
    - The wait between samples is counted loops plus a timestamp spin of only the last 16 to 20 us. Polling the
      timestamp counter for the whole gap leaves the branch predictor mispredicting the next branches, and whether
      one lands in the timed call depends only on code layout.
    - Cold samples read back only the thread's own state: its stack, its thread pointer and the call's payload.
      Never the logger or the benchmark's pointer to it, and never write to a global there, which can share a cache
      line with a logger's state.
    - Each producer thread writes only to its own stack or state; `timer_only` stores to a local, not a shared
      global.
    - The compiler barrier after every call stays, so a disabled statement reloads its level on every call.
- Linux runtime benchmarks, on the tuned system described in `docs/benchmarks.rst`:
  - Clean Release build with the GCC version listed there, Ninja and `-m64 -march=x86-64-v3`, in
    `cmake-build-release`.
  - Check that nothing else runs on the machine (`top`): another process on the benchmark CPUs makes every result
    worthless.
  - `tools/generate_tables.py` reads every `bench_results_*.txt` in the root of the logger_benchmarks checkout,
    whatever the current directory. Before a new run, move the previous run's files out of the root into an
    archive folder, so old and new results never mix.
  - Run `python3 tools/run_all.py --passes 3` once. It makes three passes over every binary in a random order
    (record the seed it prints), syncs the disk between binaries, writes `bench_results_run<pass>_<n>.txt` with the
    system configuration, and checks that every text log has all its lines.
  - Read every `delivery:` line: a `FAIL` means a library dropped messages or stopped its clock early, and its numbers
    must not be published. `generate_tables.py` skips such runs and names them on stderr. Binary logs (MS BinLog,
    BqLog binary, PlatformLab NanoLog) cannot be line-counted, so check their end-of-run logic by reading the code.
  - Check after the run, before publishing:
    - Every binary has exactly three result files and every `exit code:` is 0.
    - Every disabled expensive argument result says `Disabled argument: evaluated` or `skipped`, with a count of 0
      or the full call count.
    - The `Cold cache check` line of the cold runs: about 7 to 9 ns, an L3 hit, on the Intel machines. A value near
      an L2 hit means the sweep did not evict L2 (the AMD 9950X reads about 2 ns), which the docs must say.
    - Compare the new tables with the previous run's: a library that moves by more than run-to-run noise in only one
      table, or a value that flips between two levels across runs, is a measurement problem to explain before
      publishing, not a result.
  - If a run is interrupted, or a tool limits how long one command may run, run the passes one at a time with the
    seed of the first: `--seed <seed> --first-pass <n> --passes 1`, after removing that pass's partial results. The
    order and file names are the same as one `--passes 3` run.
  - Do not edit files in the logger_benchmarks checkout while `run_all.py` runs.
  - To rerun some binaries after a fix, move their result files out of the root (find them by the `bench:` path on
    each file's first line), run `python3 tools/run_all.py --passes 3 --only '<regex>' --tag rerun`, then build the
    tables again: they combine the main results of every other binary with the rerun files.
  - After a change that affects only the latency harness, rerun only the latency scenarios with
    `--scenarios burst single cold disabled` and keep the `throughput` entry of the previous JSON: replace only its
    `latency` entry with the new one.
  - Every latency table has a `Timer only (no logging)` row from the `timer_only` benchmark: the cost of timing a
    sample, drawn as the dashed floor line in the charts. Keep it in every run. The disabled expensive argument
    table has an `Argument only (no logging)` row instead, from the same binary: the cost of the argument alone.
  - Write the results with `python3 tools/generate_tables.py --stat best --format json > linux.json`: each
    library's best of the three runs (lowest 90th, then 95th, 99th and 99.9th percentile; highest throughput). It
    applies the same rule to every library; never select runs for Quill alone.
- Windows runtime benchmarks: follow the Windows section of the logger_benchmarks README (MSVC, Ninja, Release with
  `/arch:AVX2`), with the same `run_all.py --passes 3` and `generate_tables.py --stat best --format json >
  windows.json`. Configure and run from PowerShell in a
  Visual Studio x64 developer environment; under Git Bash, g3log's CMake configure can pick up Git's `find.exe` and
  hang. Write the JSON with Python's own redirection or a file argument, not PowerShell's `>`, which writes UTF-16.
  To bring the results to the docs machine, commit `windows.json`, the raw `bench_results_run*.txt` files and the
  MSVC traces to a branch of logger_benchmarks, such as `windows-results-<commit>`, and fetch it; the branch is not
  merged.
- Linux AMD runtime benchmarks, on a bare-metal AMD Zen 5 desktop CPU such as the Ryzen 9 9950X:
  - Choose a CPU with at least 8 cores sharing one L3 (one CCD): the main thread, the four calling threads and the
    backend thread must share it (`lscpu -e`: CPUs 0 to 7 on the same L3). Server EPYCs with 4 or fewer cores per
    CCD put threads on different CCDs and measure the fabric, not the loggers.
  - Tune it like the Intel machine: `nosmt isolcpus=domain,managed_irq,1-7 nohz_full=1-7 rcu_nocbs=1-7
    rcu_nocb_poll irqaffinity=0 mitigations=off transparent_hugepage=never nosoftlockup skew_tick=1 audit=0
    nmi_watchdog=0 nowatchdog` on the kernel command line, irqbalance off, and the `performance` policy of
    `amd-pstate-epp` with boost on. A freshly installed RAID 1 resyncs its mirror in the background: wait until
    `/proc/mdstat` shows no resync before benchmarking.
  - Run the full suite as on Linux Intel and write the results as `linux_amd.json`. The generator adds the AMD tabs
    when that file exists.
  - AMD Zen's timestamp counter advances in steps of about 10 ns, so the scenarios that time one call per sample
    (single, cold, disabled cold) move in 10 ns steps. The generator puts a note above those AMD tables
    (`COARSE_TIMER_PLATFORMS`), and the Timer Resolution section of `docs/benchmarks_latency.rst` explains it.
  - Record its system configuration in its own tab of `docs/benchmarks.rst`: CPU, memory and its configured speed,
    OS, kernel, compiler and `/proc/cmdline`.
- Tables and charts:
  - Copy `linux.json`, `linux_amd.json` and `windows.json` to `docs/benchmark_results/` and run
    `python3 scripts/generate_benchmark_tables.py`. It rewrites `docs/benchmark_tables/*.inc`, which the latency and
    throughput pages include: every table inside payload, platform and thread tabs, after its chart. Never edit the
    `.inc` files by hand.
  - The charts are drawn in the browser by `docs/_static/benchmark_charts.js`, each from the table right after its
    `<div class="bench-chart">`, so the tables are the only source of numbers. Check every page in a local docs
    build: the tabs switch together by label, and every chart draws.
  - Update each system configuration in `docs/benchmarks.rst` (Quill version, OS, compiler, `/proc/cmdline`) and
    the `*Benchmarks last updated: <Month Year>*` line.
  - Keep the page text accurate: sample counts and waits of each scenario in `docs/benchmarks_latency.rst`
    (from `call_site_latency_bench_common.h`), the throughput setup in `docs/benchmarks_throughput.rst`, the
    libraries not built on Windows, and the libraries without `std::vector` support in the generator's payload
    descriptions.
  - The README only links to the benchmark pages; do not copy results back into it.
- Compile time, after the runtime benchmarks:
  - Run the builds on the isolated benchmark CPUs (`taskset -c 1-4 cmake --build ...`), once no benchmark runs: on
    CPU 0 they share the core with everything else and vary by tens of seconds.
  - Linux: three clean builds each with GCC and Clang (delete the build directory each time), configured with
    `-G Ninja -DCMAKE_BUILD_TYPE=Release -DQUILL_BUILD_BENCHMARKS=ON -DCMAKE_CXX_FLAGS=-march=x86-64-v3`; Clang also
    sets `-DQUILL_ENABLE_TIME_TRACE=ON -DQUILL_BUILD_EXAMPLES=ON`. Time only
    `cmake --build <dir> --target BENCHMARK_quill_compile_time -j 4` and report the median, with the sizes of the
    benchmark binary and `compile_time_bench.cpp.o` in MB (10^6 bytes).
  - Clang traces: also build `quill_example_recommended_usage`, then copy `compile_time_bench.cpp.json` and
    `recommended_usage.cpp.json` from the Clang build directory to `docs/traces/`.
  - Windows: the same three clean builds with MSVC and
    `-DCMAKE_CXX_FLAGS="/DWIN32 /D_WINDOWS /EHsc /GR /arch:AVX2"`. Setting only `/arch:AVX2` replaces CMake's default
    MSVC flags, including `/EHsc`.
  - MSVC traces: capture both files with `vcperf` as `docs/benchmarks_compile_time.rst` shows, then convert them with
    `scripts/convert_vcperf_trace.py` into `docs/traces/msvc_compile_time_bench.cpp.json` and
    `docs/traces/msvc_recommended_usage.cpp.json`. Never commit raw vcperf output: Speedscope cannot load it, and it
    records the machine's environment variables, including `PATH`.
  - The Speedscope profiles in `docs/benchmarks_compile_time.rst` load from `master`, so they only load after merging. Test a branch with its raw URL:
    `https://www.speedscope.app/#profileURL=https://raw.githubusercontent.com/odygrd/quill/<branch>/docs/traces/<file>`.
- Library notes (`docs/benchmarks_library_notes.rst`, and `Reading the Results` in `docs/benchmarks.rst`), after the
  tables:
  - Re-check every claim against the vendored sources and benchmark files of the new logger_benchmarks pin, and
    update or delete any that changed. Remove a claim you cannot point to in the code, the library's docs or the
    tables. For each library check:
    - What the backend does when idle (sleeps, waits or spins) and what a full queue does (blocks, drops or grows).
    - Where formatting happens, the calling thread or the backend, and whether the output is text or binary.
    - Every setting the benchmark changes from the library's default (queue sizes, sleep durations, macros,
      compile-time options), and anything inside a timed region that other libraries do not do (for example
      XTR's `fsync` in `sink.sync()`).
    - `std::vector` support, platforms built, and the date of the last real code change (GitHub API), which sets
      the status the `At a Glance` table shows: Active means a real code change in the last 12 months, low
      activity in the last three years, unmaintained none for three years or more, or an archived repository.
      CI-only and bot commits do not count. The table shows no release versions; unmaintained libraries show the
      month of their last code change.
  - Quill's notes follow the same rule: list every non-default setting its benchmarks use.
  - Every number quoted in the notes must match the new tables.
- Commands, in order, for a release done by hand (Linux; Windows uses the same `tools/` scripts from PowerShell in a
  Visual Studio x64 developer environment, with the build commands of the logger_benchmarks README):

  ```bash
  # logger_benchmarks, at the merged master
  git fetch origin && git checkout master && git reset --hard origin/master
  mkdir -p ../results-archive/<date> && mv bench_results_*.txt ../results-archive/<date>/
  cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=<GCC from docs/benchmarks.rst> "-DCMAKE_CXX_FLAGS=-m64 -march=x86-64-v3"
  cmake --build cmake-build-release                 # never while a benchmark runs
  python3 tools/run_all.py --passes 3               # latency and throughput, about 3 hours on Linux
  grep -l FAIL bench_results_*.txt                  # must print nothing
  python3 tools/generate_tables.py --stat best --format json > linux.json

  # Quill
  cp linux.json linux_amd.json windows.json <quill>/docs/benchmark_results/
  python3 scripts/generate_benchmark_tables.py

  # Compile time, three clean builds per compiler, on the benchmark CPUs once nothing benchmarks
  rm -rf build-ct && cmake -S . -B build-ct -G Ninja -DCMAKE_BUILD_TYPE=Release -DQUILL_BUILD_BENCHMARKS=ON \
    -DCMAKE_CXX_COMPILER=<g++ or clang++> -DCMAKE_CXX_FLAGS=-march=x86-64-v3
  time taskset -c 1-4 cmake --build build-ct --target BENCHMARK_quill_compile_time -j 4

  # Docs: the same Doxygen and Sphinx steps as Read the Docs (.readthedocs.yaml)
  python3 -m venv docvenv && docvenv/bin/pip install -r docs/requirements.txt
  docvenv/bin/python docs/build_doxygen.py          # downloads Doxygen 1.16, which needs glibc 2.38 or later;
                                                    # on an older system run `doxygen docs/Doxyfile.rtd` after it
  docvenv/bin/sphinx-build -E -b html docs <output directory>
  ```

  Open the latency, throughput and compile time pages of the build and check that the tabs switch together and every
  chart draws.
- Review before merging:
  - Re-check every claim in the Verdict against the new numbers, including the log file sizes.
  - Build the docs and read the diff of the benchmark pages to confirm only the intended tables, text and traces
    changed, and that every chart draws.

## Documentation

- The Sphinx docs live under `docs/`.
- Keep the text brief and the results first: a short lead on each page, the details of a scenario right before its
  charts, each fact said once where it belongs and linked from elsewhere, and nothing dropped when shortening. Do not
  hardcode counts that change with each release, such as the number of libraries benchmarked.
- Keep each changelog entry to one or two brief sentences describing a real usage scenario and the observable fix
  or benefit for library users. Name the relevant public class, function, or option and the conditions that trigger
  the issue; scope descriptions of hangs, errors, or missing output to that case. Keep internal implementation details
  in code comments or commit descriptions.
- Do not add changelog entries for documentation-only or benchmark-only changes.
- Match the surrounding changelog formatting: no blank lines between bullet entries, and indent wrapped
  continuation lines by two spaces.
- Any standalone runnable sample in a `.rst` file (anything with `main()`, a class definition, or that the reader could
  copy-paste and compile as-is) MUST live in its own compilable `.cpp` file under `docs/snippets/`, added to
  `docs/snippets/CMakeLists.txt`, and referenced from the `.rst` via `.. literalinclude:: snippets/<file>.cpp`. Never
  write full examples as inline `.. code-block:: cpp` blocks — CI must be able to compile them so they cannot silently
  drift from the real API.
- When adding a new docs page with a runnable sample, create the snippet file first, confirm it builds under
  `QUILL_BUILD_EXAMPLES=ON`, and only then wire it into the `.rst` with a `literalinclude` directive.
- Short illustrative fragments (a macro call, a function signature, a few lines showing an API shape) can stay as inline
  `.. code-block:: cpp` since they are not standalone programs.

## Testing

- Each new feature or fix should have a corresponding test, either a unit test or a regression/integration test.
- Sometimes consider extending an existing test with one more assertion or log statement instead of adding a new one.
- Unit test files may contain multiple `TEST_CASE`s when the scenarios are closely related.
- Do not use `SUBCASE` in unit tests. Use separate, independently named `TEST_CASE`s with their own setup and
  cleanup so each scenario is readable and runnable on its own. Prefer a little setup duplication to shared
  scenario-selection flags.
- Each integration test file should contain exactly one `TEST_CASE`.
- Integration/regression test files should be self-contained. Do not add shared helper headers for them; keep any
  test-only setup local to the `.cpp` file even if that means a small amount of duplication.
- Integration tests must use unique logger names, test names, and log/output filenames so they can run safely in
  parallel without colliding through shared files or global state.
- For conditionally-skipped tests (platform-specific, `QUILL_NO_EXCEPTIONS`, etc.), keep the `TEST_CASE` itself
  unguarded and put the `#if` inside the test body. On unsupported configurations, return early from the test instead
  of wrapping the whole test declaration in preprocessor guards.
- If the behavior is user-visible or easy to regress, prefer an end-to-end test in addition to a narrow unit test.
- New features and tests should compile and run across supported platforms and toolchains, including Linux, macOS, and
  Windows, and major compilers such as GCC, Clang, and Intel LLVM.
- Never attempt a full build of all targets unless the user explicitly asks for one. Full builds are slow on this
  project, so only build the specific target(s) touched by the change (e.g. a single snippet, unit test, or integration
  test) and let the user drive any broader build.

## Code Style

- The repo is C++17 only.
- Format Quill's C++ sources under `include/quill/`, `test/`, `examples/`, `docs/snippets/`, `benchmarks/` and `fuzz/`
  with clang-format and the repository's `.clang-format`. Exclude vendored sources, including
  `include/quill/bundled/` and `test/bundled/`, and the generated codecs in `examples/sbe_binary_data/sample/`.
- Follow existing codebase patterns.
- Prefer readable, maintainable, low-overhead code.
- After a change, reread the surrounding code as a whole. Remove redundant logic and simplify anything the change
  makes unnecessary in the same commit.
- Use blank lines to separate logical steps or changes of context in code and tests, such as setup, actions,
  assertions, and cleanup. Keep related statements together; avoid dense walls of text or a blank line after every statement.
- Keep assignments out of `if` and `while` conditions: compute the value first, then test it.
  Keep state-changing steps separate from compound conditions so their order is easy to follow.
- Use descriptive names.
- Prefer const-correctness, `constexpr`, and RAII where appropriate.
- Use `auto` when it improves readability, not by default everywhere.
- Prefer brace initialization for variables and objects instead of `=` or `()` where it is valid and readable.
- Always use braces for control flow.
- Place member variables at the end of the class definition.
- Group members by access pattern first: keep frequently accessed members used together adjacent so they can share
  cache lines, while preserving intentional separation between threads. Within each group, prefer decreasing size,
  account for alignment, and place small flags together to reduce padding. Preserve initialization and destruction
  dependencies; verify the compiler's layout before claiming size or cache-line improvements.
- Keep comments concise and useful; follow the existing Doxygen style in headers.

## Performance Mindset

- Never assume an optimization helps; use `perf` or assembly output to validate.
- Watch unnecessary allocations, cache locality, vectorization hints, and false sharing.
- Consider the performance impact of every change, especially on the frontend where even a single extra branch can
  matter. The backend is less sensitive, but throughput still matters.
- Optimize for how applications log, not for how a benchmark logs. An application interleaves many different log
  statements, loggers, threads and argument sizes. Do not add a cache, fast path or special case whose gain depends on
  the same log statement repeating, such as remembering the previous statement's format string, metadata or sizes: it
  flatters a benchmark loop and does nothing, or costs, in real use.
- The throughput benchmarks cycle between several log statements for that reason; keep them that way. Before keeping
  an optimization, also measure it with hundreds of distinct statements in mixed order, and report both results. If
  the gain is not there with many statements, drop the change, and never quote a single-statement number as the gain.
