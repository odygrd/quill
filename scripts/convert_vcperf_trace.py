#!/usr/bin/env python3
"""Convert an MSVC C++ Build Insights trace (vcperf /timetrace) so that it opens in Speedscope.

vcperf writes the Chrome trace event format, but Speedscope cannot read it as is:

- vcperf closes every "B" (begin) event with an "E" (end) event whose name is empty. Speedscope
  matches end events to begin events by name, so no frame is ever closed and the profile comes
  out mangled. Each B/E pair is converted into a single complete ("X") event instead, which is
  also how Clang's -ftime-trace files are written.
- Zero-length events carry no timing and make end matching ambiguous, so they are dropped.
- The compiler invocation records the INCLUDE, LIBPATH and PATH environment variables, which
  expose the local machine setup, so every "Env Var: ..." argument is removed.

Usage:
    vcperf /start /noadmin /level3 quill
    cmake --build . --target BENCHMARK_quill_compile_time -j 4
    vcperf /stop /templates quill /timetrace compile_time_bench.raw.json
    python scripts/convert_vcperf_trace.py compile_time_bench.raw.json compile_time_bench.cpp.json
"""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path


def convert(trace: dict | list) -> dict:
    events = trace["traceEvents"] if isinstance(trace, dict) else trace
    display_time_unit = trace.get("displayTimeUnit", "ms") if isinstance(trace, dict) else "ms"

    complete_events = []
    open_events = collections.defaultdict(list)

    for event in events:
        phase = event.get("ph")
        thread = (event.get("pid"), event.get("tid"))

        if phase == "B":
            open_events[thread].append(event)
        elif phase == "E":
            if not open_events[thread]:
                raise ValueError(f"end event without a begin event on thread {thread} at ts {event.get('ts')}")
            begin = open_events[thread].pop()
            complete_event = {
                "name": begin["name"],
                "ph": "X",
                "pid": begin["pid"],
                "tid": begin["tid"],
                "ts": begin["ts"],
                "dur": event["ts"] - begin["ts"],
            }
            if begin.get("args"):
                complete_event["args"] = begin["args"]
            complete_events.append(complete_event)
        elif phase == "X":
            complete_events.append(event)
        else:
            raise ValueError(f"unsupported trace event phase {phase!r}")

    unclosed = sum(len(stack) for stack in open_events.values())
    if unclosed:
        raise ValueError(f"{unclosed} begin events have no end event")

    for event in complete_events:
        args = event.get("args")
        if args:
            for key in [key for key in args if key.startswith("Env Var:")]:
                del args[key]
            if not args:
                del event["args"]

    complete_events = [event for event in complete_events if event["dur"] > 0]

    # Parents first: sort by thread, start time, then longest duration
    complete_events.sort(key=lambda e: (e["pid"], e["tid"], e["ts"], -e["dur"]))

    return {"traceEvents": complete_events, "displayTimeUnit": display_time_unit}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", type=Path, help="trace written by vcperf /timetrace")
    parser.add_argument("output", type=Path, help="converted trace to load into Speedscope")
    args = parser.parse_args()

    trace = json.loads(args.input.read_text(encoding="utf-8-sig"))
    converted = convert(trace)
    args.output.write_text(json.dumps(converted, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")

    event_count = len(trace["traceEvents"] if isinstance(trace, dict) else trace)
    print(f"{args.output}: {event_count} events -> {len(converted['traceEvents'])} complete events")


if __name__ == "__main__":
    main()
