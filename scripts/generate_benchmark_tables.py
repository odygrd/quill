#!/usr/bin/env python3
"""
Writes the result tables of the benchmark pages, docs/benchmark_tables/*.inc, from the results of
logger_benchmarks.

Each platform's results are one JSON file, written in logger_benchmarks with
`python3 tools/generate_tables.py --stat best --format json`, and kept in docs/benchmark_results/:
linux.json and windows.json. Every table is written inside tabs, Linux and Windows, then thread
counts, after the chart that docs/_static/benchmark_charts.js draws from it. The benchmark pages
include these files, so updating the results is:

    python3 scripts/generate_benchmark_tables.py
"""

from __future__ import annotations

import json
from pathlib import Path

DOCS = Path(__file__).resolve().parents[1] / "docs"
RESULTS = DOCS / "benchmark_results"
OUTPUT = DOCS / "benchmark_tables"

# (tab label, results file, required). An optional platform gets its tabs only once its results file exists.
PLATFORMS = [
    ("Linux Intel (GCC)", "linux.json", True),
    ("Linux AMD (GCC)", "linux_amd.json", False),
    ("Windows Intel (MSVC)", "windows.json", True),
]

# Platforms whose timestamp counter advances in steps too coarse to resolve a single call: AMD Zen's TSC reads in
# steps of about 10 ns. Scenarios that time one call per sample get a note above their tables there; burst and
# disabled hot average 20 and 1,000 calls per sample and need none.
COARSE_TIMER_PLATFORMS = {"Linux AMD (GCC)"}
SINGLE_CALL_SCENARIOS = {"single", "cold", "disabled_cold"}


def coarse_timer_note(scenario: str) -> list[str]:
    text = ("   This platform's timestamp counter advances in steps of about 10 ns, so each call here is timed to the "
            "nearest 10 ns and the fastest libraries tie.")
    if scenario in ("cold", "disabled_cold"):
        # The harness's own check: a line read before the sweep reloaded in about 2 ns on the Ryzen 9 9950X, an L2
        # hit, against about 8 ns, an L3 hit, on the Intel machines
        text += (" The cache sweep also leaves more of the L2 cache in place on this CPU than on the Intel ones, so "
                 "these calls start less cold.")
    return [".. note::", "", text + " See :ref:`benchmarks_latency:timer resolution`.", ""]


def with_note(platform: str, scenario: str, lines: list[str]) -> list[str]:
    if platform in COARSE_TIMER_PLATFORMS and scenario in SINGLE_CALL_SCENARIOS:
        return coarse_timer_note(scenario) + lines
    return lines

SCENARIOS = {
    "single": "Single Call",
    "burst": "Burst of 20 Calls",
    "cold": "Cold Cache",
}

# (tab label, description, the log statement, a note after it)
PAYLOADS = {
    "int_int_double": (
        "Numbers",
        "Two integers and a double:",
        'LOG_INFO(logger, "Logging int: {}, int: {}, double: {}", i, j, d);',
        "",
    ),
    "int_int_largestr": (
        "Large Strings",
        "Two integers and a ``std::string`` of more than 35 characters, too long for the short string optimization:",
        'LOG_INFO(logger, "Logging int: {}, int: {}, string: {}", i, j, large_string);',
        "",
    ),
    "vector_largestr": (
        "Complex Types",
        "Two integers and a ``std::vector<std::string>`` of 16 strings of 50 to 60 characters:",
        'LOG_INFO(logger, "Logging int: {}, int: {}, vector: {}", i, j, v);',
        "PlatformLab NanoLog, Reckless, BqLog, g3log and Iyengar NanoLog cannot log a ``std::vector`` and are not "
        "shown. Boost.Log cannot either; its benchmark writes each element into the record on the calling thread.",
    ),
}

DISABLED = {"disabled_hot": "Hot", "disabled_cold": "Cold", "disabled_argument": "Expensive Argument"}

# Decimals shown per scenario. A sample of one call is timed with two reads of the CPU's timestamp counter, so it
# moves in steps of a few tenths of a nanosecond and varies by about a nanosecond between runs: whole nanoseconds.
# Burst and the disabled hot and expensive argument samples average 20 or 1,000 calls, which resolves tenths.
DECIMALS = {"single": 0, "cold": 0, "disabled_cold": 0, "burst": 1, "disabled_hot": 1, "disabled_argument": 1}


def indent(lines: list[str], level: int) -> list[str]:
    return [("   " * level + line) if line else "" for line in lines]


def link(row: dict) -> str:
    return f"`{row['name']} <{row['url']}>`__"


def chart(title: str) -> list[str]:
    return [".. raw:: html", "", f'   <div class="bench-chart" data-title="{title}"></div>', ""]


def latency_table(rows: list[dict], title: str, decimals: int) -> list[str]:
    # The rows of the disabled argument scenario also say whether the library computed the argument. The column
    # comes last: benchmark_charts.js reads the six percentiles from the columns after the library.
    argument = any("argument_evaluated" in row for row in rows)
    columns = ["50th", "75th", "90th", "95th", "99th", "99.9th"] + (["Argument"] if argument else [])

    lines = chart(title) + [".. list-table::", "   :header-rows: 1", "   :class: bench-table bench-center", ""]
    lines += ["   * - Library"] + [f"     - {column}" for column in columns]
    for row in rows:
        lines.append(f"   * - {link(row)}")
        lines += [f"     - {value:.{decimals}f}" for value in row["values"]]
        if argument:
            lines.append(f"     - {'evaluated' if row.get('argument_evaluated') else 'skipped'}")
    return lines + [""]


def throughput_table(rows: list[dict], title: str) -> list[str]:
    lines = chart(title) + [".. list-table::", "   :header-rows: 1", "   :class: bench-table bench-center", ""]
    columns = ("million msg/second", "elapsed time", "producers done")
    lines += ["   * - Library"] + [f"     - {column}" for column in columns]
    for row in rows:
        producers = f"{row['producers_ms']:.0f} ms" if row.get("producers_ms") is not None else "-"
        lines.append(f"   * - {link(row)}")
        lines += [f"     - {row['throughput']:.2f}", f"     - {row['elapsed_ms']:.0f} ms", f"     - {producers}"]
    return lines + [""]


def tab_set(items: list[tuple[str, list[str]]], choice: str | None = None) -> list[str]:
    """
    Tabs with the given labels and contents. Tabs with the same label on a page switch together.
    The first choice of a section, payload or platform, names its choice and is drawn as large cards.
    """
    lines = [".. md-tab-set::"]
    if choice:
        lines.append(f"   :class: bench-tabs bench-tabs-{choice}")
    lines.append("")
    for label, content in items:
        lines += [f"   .. md-tab-item:: {label}", ""] + indent(content, 2) + [""]
    return lines


def latency_tables(results: dict, scenario: str, payload: str) -> dict[int, list[dict]]:
    return {
        table["threads"]: table["rows"]
        for table in results["latency"]
        if table["scenario"] == scenario and table["payload"] == payload
    }


def not_run(platform: str, results: dict) -> list[str]:
    if results.get("missing"):
        return [f"*The {platform} results are being updated.*", ""]
    return [f"*Not run on {platform}.*", ""]


def latency_fragment(platform_results: list[tuple[str, dict]], scenario: str) -> list[str]:
    payload_items = []
    for payload, (payload_title, description, statement, note) in PAYLOADS.items():
        platform_items = []
        for platform, results in platform_results:
            tables = latency_tables(results, scenario, payload)
            if not tables:
                platform_items.append((platform, not_run(platform, results)))
                continue

            chart = f"{SCENARIOS[scenario]} · {payload_title}"
            if len(tables) == 1:
                threads, rows = next(iter(tables.items()))
                platform_items.append(
                    (platform, with_note(platform, scenario, latency_table(rows, f"{chart} · {platform}", DECIMALS[scenario]))))
                continue

            thread_items = []
            for threads, rows in sorted(tables.items()):
                label = "1 Thread" if threads == 1 else f"{threads} Threads"
                thread_items.append((label, latency_table(rows, f"{chart} · {label} · {platform}", DECIMALS[scenario])))
            platform_items.append((platform, with_note(platform, scenario, tab_set(thread_items))))

        intro = [description, "", ".. code-block:: cpp", "", f"   {statement}", ""] + ([note, ""] if note else [])
        payload_items.append((payload_title, intro + tab_set(platform_items)))
    return tab_set(payload_items, "payload")


def disabled_fragment(platform_results: list[tuple[str, dict]]) -> list[str]:
    platform_items = []
    for platform, results in platform_results:
        items = []
        for scenario, label in DISABLED.items():
            tables = latency_tables(results, scenario, "int_int_double")
            if 1 in tables:
                items.append((label, with_note(platform, scenario, latency_table(
                    tables[1], f"Disabled Statement · {label} · {platform}", DECIMALS[scenario]))))
        platform_items.append((platform, tab_set(items) if items else not_run(platform, results)))
    return tab_set(platform_items, "platform")


def throughput_fragment(platform_results: list[tuple[str, dict]]) -> list[str]:
    platform_items = []
    for platform, results in platform_results:
        items = []
        for table in sorted(results["throughput"], key=lambda table: table["threads"]):
            threads = table["threads"]
            label = "1 Producer" if threads == 1 else f"{threads} Producers"
            items.append((label, throughput_table(table["rows"], f"Throughput · {label} · {platform}")))
        platform_items.append((platform, tab_set(items) if items else not_run(platform, results)))
    return tab_set(platform_items, "platform")


def write(name: str, lines: list[str]) -> None:
    header = [f".. Generated by scripts/generate_benchmark_tables.py from docs/benchmark_results, do not edit", ""]
    text = "\n".join(header + lines).rstrip() + "\n"
    (OUTPUT / name).write_text(text, encoding="utf-8")


def main() -> None:
    platform_results = []
    for platform, file_name, required in PLATFORMS:
        path = RESULTS / file_name
        if not required and not path.exists():
            continue
        results = (json.loads(path.read_text(encoding="utf-8")) if path.exists()
                   else {"latency": [], "throughput": [], "missing": True})
        platform_results.append((platform, results))

    OUTPUT.mkdir(exist_ok=True)
    for scenario in SCENARIOS:
        write(f"latency_{scenario}.inc", latency_fragment(platform_results, scenario))
    write("latency_disabled.inc", disabled_fragment(platform_results))
    write("throughput.inc", throughput_fragment(platform_results))


if __name__ == "__main__":
    main()
