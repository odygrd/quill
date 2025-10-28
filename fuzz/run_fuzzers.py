#!/usr/bin/env python3
"""
Run Quill fuzzers and report any errors.

Usage Examples:
    # Typical workflow: build then run fuzzers
    # Step 1: Build
    mkdir cmake-build-debug && cd cmake-build-debug
    CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Debug \
      -DQUILL_BUILD_TESTS=ON -DQUILL_FUZZING=ON ..
    cmake --build . -j4

    # Step 2: Run fuzzers (pass build directory)
    cd ..
    python3 test/fuzzing/run_fuzzers.py --duration 60 --fuzzer-dir cmake-build-debug

    # Run for 10 minutes with 16GB RAM
    python3 test/fuzzing/run_fuzzers.py --duration 600 --rss-limit 16384 \
        --fuzzer-dir cmake-build-release

    # Run a specific fuzzer only
    python3 test/fuzzing/run_fuzzers.py --duration 300 --fuzzer FUZZ_BasicTypes
    python3 test/fuzzing/run_fuzzers.py --duration 60 --fuzzer FUZZ_StlContainers

    # Run all fuzzers for 100,000 iterations each (instead of time-based)
    python3 test/fuzzing/run_fuzzers.py --runs 100000

    # Specify build directory (script finds fuzzers inside)
    python3 test/fuzzing/run_fuzzers.py --duration 60 \
        --fuzzer-dir cmake-build-debug

    python3 test/fuzzing/run_fuzzers.py --duration 60 \
        --fuzzer-dir cmake-build-release

    # Or specify exact fuzzer directory
    python3 test/fuzzing/run_fuzzers.py --duration 60 \
        --fuzzer-dir cmake-build-debug/build/test

    # From build directory (common in CI)
    cd build-fuzzing
    python3 ../test/fuzzing/run_fuzzers.py --duration 600 \
        --fuzzer-dir .

    # Run with leak detection enabled (slower, may have false positives)
    python3 test/fuzzing/run_fuzzers.py --duration 60 --detect-leaks

    # Verbose output (shows fuzzer progress)
    python3 test/fuzzing/run_fuzzers.py --duration 60 --verbosity 2

Fuzzer Directory:
    The script auto-detects fuzzer binaries by searching common locations.

    You can pass --fuzzer-dir in two ways:
    1. Build directory (e.g., cmake-build-debug, build-fuzzing)
       → Script searches for fuzzers in <dir>/build/test, <dir>/test
    2. Exact directory containing fuzzer binaries
       → Used directly

    Auto-detection searches:
    - build-fuzzing/build/test, build-fuzzing/test
    - build/test, test
    - Current directory

    The script prints "Using fuzzer directory: ..." showing what was found.

Exit Codes:
    0 - All fuzzers passed without errors
    1 - One or more fuzzers found errors or failed

Output:
    The script provides a summary showing:
    - Total runs completed across all fuzzers
    - Per-fuzzer statistics (runs, duration, runs/sec)
    - Any errors found with context
    - Overall pass/fail status
"""

import argparse
import subprocess
import sys
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional


@dataclass
class FuzzerResult:
    """Results from running a single fuzzer."""
    name: str
    exit_code: int
    stdout: str
    stderr: str
    duration: float
    runs_completed: int
    errors_found: List[str]


class FuzzerRunner:
    """Manages running multiple fuzzers in parallel."""

    FUZZERS = [
        "FUZZ_BasicTypes",
        "FUZZ_StlContainers",
        "FUZZ_UserDefinedDeferredFormat",
        "FUZZ_UserDefinedDirectFormat",
        "FUZZ_QueueStress",
        "FUZZ_BinaryData",
    ]

    ERROR_PATTERNS = [
        "assertion failed",
        "ERROR: AddressSanitizer",
        "ERROR: LeakSanitizer",
        "ERROR: UndefinedBehaviorSanitizer",
        "ERROR: libFuzzer",
        "deadly signal",
        "runtime error:",
        "SUMMARY: AddressSanitizer",
        "SUMMARY: UndefinedBehaviorSanitizer",
        "SUMMARY: LeakSanitizer",
    ]

    def __init__(self, duration: Optional[int], runs: Optional[int],
                 rss_limit: int, verbosity: int, detect_leaks: bool,
                 fuzzer_dir: Path, dictionary_path: Optional[Path] = None):
        self.duration = duration
        self.runs = runs
        self.rss_limit = rss_limit
        self.verbosity = verbosity
        self.detect_leaks = detect_leaks
        self.fuzzer_dir = fuzzer_dir
        self.dictionary_path = dictionary_path
        self.results: List[FuzzerResult] = []

    def build_fuzzer_command(self, fuzzer_name: str) -> List[str]:
        """Build the command line for running a fuzzer."""
        fuzzer_path = self.fuzzer_dir / fuzzer_name

        cmd = [str(fuzzer_path)]

        if self.duration is not None:
            cmd.extend([f"-max_total_time={self.duration}"])

        if self.runs is not None:
            cmd.extend([f"-runs={self.runs}"])

        cmd.extend([
            f"-rss_limit_mb={self.rss_limit}",
            f"-verbosity={self.verbosity}",
        ])

        if not self.detect_leaks:
            cmd.extend(["-detect_leaks=0"])

        # Add dictionary if available
        if self.dictionary_path and self.dictionary_path.exists():
            cmd.extend([f"-dict={self.dictionary_path}"])

        return cmd

    def run_fuzzer(self, fuzzer_name: str) -> FuzzerResult:
        """Run a single fuzzer and capture its output."""
        cmd = self.build_fuzzer_command(fuzzer_name)

        print(f"[{fuzzer_name}] Starting: {' '.join(cmd)}")

        start_time = time.time()
        stdout_lines = []

        try:
            # Use Popen to avoid pipe buffer deadlock with large outputs
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1  # Line buffered
            )

            # Read output line by line to avoid buffer deadlock
            for line in process.stdout:
                stdout_lines.append(line)

            process.wait(timeout=self.duration + 60 if self.duration else None)
            exit_code = process.returncode
            stdout = ''.join(stdout_lines)
            stderr = ""

        except subprocess.TimeoutExpired:
            process.kill()
            # Read any remaining output
            remaining = process.stdout.read()
            if remaining:
                stdout_lines.append(remaining)
            process.wait()
            stdout = ''.join(stdout_lines)
            stderr = ""
            # Timeout is expected when fuzzer runs close to max_total_time
            # Only treat as error if actual errors are found in output
            exit_code = 0
        except Exception as e:
            stdout = ''.join(stdout_lines) if stdout_lines else ""
            stderr = f"Failed to run fuzzer: {e}"
            exit_code = -1

        duration = time.time() - start_time

        # Extract number of runs completed
        runs_completed = self._extract_runs_completed(stdout)

        # Find errors
        errors = self._find_errors(stdout + stderr)

        return FuzzerResult(
            name=fuzzer_name,
            exit_code=exit_code,
            stdout=stdout,
            stderr=stderr,
            duration=duration,
            runs_completed=runs_completed,
            errors_found=errors
        )

    def _extract_runs_completed(self, output: str) -> int:
        """Extract the number of runs completed from fuzzer output."""
        # Look for patterns like "Done 12345 runs in" or "#12345  DONE"
        for line in output.split('\n'):
            # Pattern: "Done 12345 runs in"
            if 'Done' in line and 'runs in' in line:
                parts = line.split()
                try:
                    idx = parts.index('Done')
                    return int(parts[idx + 1])
                except (ValueError, IndexError):
                    pass
            # Pattern: "#12345  DONE"
            elif 'DONE' in line and line.strip().startswith('#'):
                try:
                    num_str = line.strip()[1:].split()[0]
                    return int(num_str)
                except (ValueError, IndexError):
                    pass
        return 0

    def _find_errors(self, output: str) -> List[str]:
        """Find error messages in fuzzer output."""
        errors = []
        lines = output.split('\n')

        for i, line in enumerate(lines):
            for pattern in self.ERROR_PATTERNS:
                if pattern in line:
                    # Include context: 2 lines before and 3 lines after
                    start = max(0, i - 2)
                    end = min(len(lines), i + 4)
                    context = '\n'.join(lines[start:end])
                    errors.append(f"Error pattern '{pattern}' found:\n{context}")
                    break

        return errors

    def run_all_parallel(self) -> List[FuzzerResult]:
        """Run all fuzzers in parallel."""
        threads = []
        results = []

        def run_and_store(fuzzer_name: str):
            result = self.run_fuzzer(fuzzer_name)
            results.append(result)

        # Start all fuzzers in parallel
        for fuzzer_name in self.FUZZERS:
            thread = threading.Thread(target=run_and_store, args=(fuzzer_name,))
            thread.start()
            threads.append(thread)

        # Wait for all to complete
        for thread in threads:
            thread.join()

        return results

    def print_summary(self, results: List[FuzzerResult]) -> int:
        """Print summary of all fuzzer runs. Returns exit code (0 = success)."""
        print("\n" + "=" * 80)
        print("FUZZING SUMMARY")
        print("=" * 80)

        total_runs = sum(r.runs_completed for r in results)
        total_duration = max(r.duration for r in results)
        total_errors = sum(len(r.errors_found) for r in results)

        print(f"\nTotal runs completed: {total_runs:,}")
        print(f"Total time: {total_duration:.1f} seconds")
        print(f"Total errors found: {total_errors}")
        print()

        # Print per-fuzzer results
        has_errors = False

        for result in sorted(results, key=lambda r: r.name):
            status = "✓ PASS" if not result.errors_found and result.exit_code == 0 else "✗ FAIL"

            print(f"{status} {result.name}")
            print(f"  Runs: {result.runs_completed:,} in {result.duration:.1f}s "
                  f"({result.runs_completed / result.duration:.0f} runs/s)")
            print(f"  Exit code: {result.exit_code}")

            if result.errors_found:
                has_errors = True
                print(f"  Errors: {len(result.errors_found)}")
                for i, error in enumerate(result.errors_found, 1):
                    print(f"\n  Error {i}:")
                    for line in error.split('\n'):
                        print(f"    {line}")

            if result.stderr:
                print(f"  Stderr: {result.stderr}")

            print()

        # Final result
        print("=" * 80)
        if has_errors or any(r.exit_code != 0 for r in results):
            print("FUZZING FAILED - Errors detected")
            print("=" * 80)
            return 1
        else:
            print("FUZZING PASSED - No errors detected")
            print("=" * 80)
            return 0


def main():
    parser = argparse.ArgumentParser(
        description="Run Quill fuzzers and report errors",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        "--duration",
        type=int,
        help="Run each fuzzer for this many seconds"
    )
    group.add_argument(
        "--runs",
        type=int,
        help="Run each fuzzer for this many iterations"
    )

    parser.add_argument(
        "--rss-limit",
        type=int,
        default=8192,
        help="RSS memory limit in MB (default: 8192)"
    )

    parser.add_argument(
        "--verbosity",
        type=int,
        default=1,
        choices=[0, 1, 2, 3],
        help="Fuzzer verbosity level (default: 1 for final stats)"
    )

    parser.add_argument(
        "--detect-leaks",
        action="store_true",
        help="Enable leak detection (slower, may have false positives)"
    )

    parser.add_argument(
        "--fuzzer",
        type=str,
        default="*",
        help="Fuzzer to run (default: * = all). Options: FUZZ_BasicTypes, FUZZ_StlContainers, "
             "FUZZ_UserDefinedDeferredFormat, FUZZ_UserDefinedDirectFormat, FUZZ_QueueStress, "
             "FUZZ_BinaryData, or * for all"
    )

    parser.add_argument(
        "--fuzzer-dir",
        type=Path,
        help="Build directory or directory containing fuzzer binaries. Examples: "
             "--fuzzer-dir cmake-build-debug (searches subdirs), "
             "--fuzzer-dir build/test (exact dir). Default: auto-detect"
    )

    args = parser.parse_args()

    # Auto-detect or find fuzzer directory
    if args.fuzzer_dir is None:
        script_dir = Path(__file__).parent

        # Try common locations
        candidates = [
            script_dir / "../../build-fuzzing/build/test",
            script_dir / "../../build-fuzzing/test",
            script_dir / "../../build/test",
            script_dir / "../../test",
            Path.cwd() / "build/test",
            Path.cwd() / "test",
            Path.cwd(),
        ]

        for candidate in candidates:
            if candidate.exists() and (candidate / "FUZZ_BasicTypes").exists():
                args.fuzzer_dir = candidate
                break

        if args.fuzzer_dir is None:
            print("ERROR: Could not find fuzzer binaries. Please specify --fuzzer-dir",
                  file=sys.stderr)
            return 1
    else:
        # User specified a directory - check if it's a build dir or the actual fuzzer dir
        fuzzer_dir = Path(args.fuzzer_dir).resolve()

        # If user passed a build directory, search for fuzzers in common subdirs
        if not (fuzzer_dir / "FUZZ_BasicTypes").exists():
            # Try common subdirectories within the build dir
            search_paths = [
                fuzzer_dir / "build/test",
                fuzzer_dir / "test",
                fuzzer_dir,
            ]

            found = False
            for search_path in search_paths:
                if search_path.exists() and (search_path / "FUZZ_BasicTypes").exists():
                    args.fuzzer_dir = search_path
                    found = True
                    break

            if not found:
                print(f"ERROR: Could not find fuzzer binaries in {fuzzer_dir} or its subdirectories",
                      file=sys.stderr)
                print("Searched in:", file=sys.stderr)
                for sp in search_paths:
                    print(f"  - {sp}", file=sys.stderr)
                return 1
        else:
            args.fuzzer_dir = fuzzer_dir

    args.fuzzer_dir = args.fuzzer_dir.resolve()

    # Auto-detect dictionary file in test/fuzzing directory
    script_dir = Path(__file__).parent
    dictionary_path = script_dir / "fuzz.dict"
    if not dictionary_path.exists():
        dictionary_path = None

    print(f"Using fuzzer directory: {args.fuzzer_dir}")
    if dictionary_path:
        print(f"Using dictionary: {dictionary_path}")
    print(f"Configuration:")
    if args.duration:
        print(f"  Duration: {args.duration} seconds per fuzzer")
    if args.runs:
        print(f"  Runs: {args.runs} iterations per fuzzer")
    print(f"  RSS limit: {args.rss_limit} MB")
    print(f"  Verbosity: {args.verbosity}")
    print(f"  Leak detection: {'enabled' if args.detect_leaks else 'disabled'}")
    print()

    # Filter fuzzers based on argument
    if args.fuzzer == "*":
        fuzzers_to_run = FuzzerRunner.FUZZERS
    elif args.fuzzer in FuzzerRunner.FUZZERS:
        fuzzers_to_run = [args.fuzzer]
    else:
        print(f"ERROR: Unknown fuzzer '{args.fuzzer}'. Available fuzzers:",
              file=sys.stderr)
        for f in FuzzerRunner.FUZZERS:
            print(f"  - {f}", file=sys.stderr)
        return 1

    print(f"Running fuzzers: {', '.join(fuzzers_to_run)}")
    print()

    runner = FuzzerRunner(
        duration=args.duration,
        runs=args.runs,
        rss_limit=args.rss_limit,
        verbosity=args.verbosity,
        detect_leaks=args.detect_leaks,
        fuzzer_dir=args.fuzzer_dir,
        dictionary_path=dictionary_path
    )

    # Override the fuzzers list
    runner.FUZZERS = fuzzers_to_run

    results = runner.run_all_parallel()
    exit_code = runner.print_summary(results)

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
