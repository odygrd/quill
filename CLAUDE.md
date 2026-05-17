# CLAUDE.md

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

## Testing

- Each new feature or fix should have a corresponding test, either a unit test or a regression/integration test.
- Sometimes consider extending an existing test with one more assertion or log statement instead of adding a new one.
- Unit test files may contain multiple `TEST_CASE`s when the scenarios are closely related.
- Each integration test file should contain exactly one `TEST_CASE`.
- Integration/regression test files should be self-contained. Do not add shared helper headers for them; keep any
  test-only setup local to the `.cpp` file even if that means a small amount of duplication.
- Integration tests must use unique logger names, test names, and log/output filenames so they can run safely in
  parallel without colliding through shared files or global state.
- For platform-specific tests, keep the `TEST_CASE` itself unguarded and put the platform `#if` inside the test body.
  On unsupported platforms, return early from the test instead of wrapping the whole test declaration in preprocessor
  guards.
- If the behavior is user-visible or easy to regress, prefer an end-to-end test in addition to a narrow unit test.
- New features and tests should compile and run across supported platforms and toolchains, including Linux, macOS, and
  Windows, and major compilers such as GCC, Clang, and Intel LLVM.

## Code Style

- The repo is C++17 only.
- Follow existing codebase patterns.
- Prefer readable, maintainable, low-overhead code.
- Use descriptive names.
- Prefer const-correctness, `constexpr`, and RAII where appropriate.
- Use `auto` when it improves readability, not by default everywhere.
- Prefer brace initialization for variables and objects instead of `=` or `()` where it is valid and readable.
- Always use braces for control flow.
- Place member variables at the end of the class definition.
- Keep comments concise and useful; follow the existing Doxygen style in headers.

## Performance Mindset

- Never assume an optimization helps; use `perf` or assembly output to validate.
- Watch unnecessary allocations, cache locality, vectorization hints, and false sharing.
- Consider the performance impact of every change, especially on the frontend where even a single extra branch can
  matter. The backend is less sensitive, but throughput still matters.
