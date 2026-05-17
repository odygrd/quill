#!/usr/bin/env python3
"""Generate benchmark charts for README.md as static SVG assets.

The README benchmark tables are the single source of truth. This script parses
those markdown tables and regenerates the chart assets under ``docs/charts`` so
the visuals stay in sync with the published benchmark numbers.
"""

from __future__ import annotations

from dataclasses import dataclass
from html import escape
from pathlib import Path
import math
import re
import textwrap

ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
CHARTS_DIR = ROOT / "docs" / "charts"

LINK_RE = re.compile(r"\[([^\]]+)\]\([^)]+\)")
TABLE_RE = re.compile(
    r"(?P<table>(?:^\|.*\|\n){3,})",
    re.MULTILINE,
)
IMAGE_RE = re.compile(
    r"!\[(?P<alt>[^\]]*)\]\((?:docs/charts/|docs%2Fcharts%2F)(?P<stem>[^.)]+)\.(?:webp|svg)\)"
)
HEADING_RE = re.compile(r"^(#{2,6})\s+(.*)$", re.MULTILINE)

LATENCY_LABELS = ["50th", "75th", "90th", "95th", "99th", "99.9th"]

# Refined color palette — vibrant but professional
COLOR_MAP = {
    "Quill Bounded Dropping Queue": "#16a34a",
    "Quill Unbounded Queue": "#22c55e",
    "Quill Unbounded Queue (Log Functions)": "#06b6d4",
    "Quill": "#22c55e",
    "Quill - Macro Free Mode": "#06b6d4",
    "fmtlog": "#f97316",
    "PlatformLab NanoLog": "#fbbf24",
    "MS BinLog": "#8b5cf6",
    "MS BinLog (binary log)": "#8b5cf6",
    "XTR": "#eab308",
    "Reckless": "#ef4444",
    "Iyengar NanoLog": "#6366f1",
    "spdlog": "#0ea5e9",
    "g3log": "#a855f7",
    "BqLog": "#d946ef",
    "BqLog (binary log)": "#a21caf",
    "Boost.Log": "#78716c",
}

# Lighter tint for gradient top
COLOR_MAP_LIGHT = {
    "Quill Bounded Dropping Queue": "#4ade80",
    "Quill Unbounded Queue": "#86efac",
    "Quill Unbounded Queue (Log Functions)": "#67e8f9",
    "Quill": "#86efac",
    "Quill - Macro Free Mode": "#67e8f9",
    "fmtlog": "#fdba74",
    "PlatformLab NanoLog": "#fde68a",
    "MS BinLog": "#c4b5fd",
    "MS BinLog (binary log)": "#c4b5fd",
    "XTR": "#fde047",
    "Reckless": "#fca5a5",
    "Iyengar NanoLog": "#a5b4fc",
    "spdlog": "#7dd3fc",
    "g3log": "#d8b4fe",
    "BqLog": "#f0abfc",
    "BqLog (binary log)": "#e879f9",
    "Boost.Log": "#a8a29e",
}

THROUGHPUT_EXCLUDED = {"MS BinLog (binary log)", "BqLog (binary log)", "BqLog"}


@dataclass
class ChartSpec:
    stem: str
    title: str
    kind: str
    columns: list[str]
    rows: list[tuple[str, list[float]]]


def _strip_link_markup(value: str) -> str:
    return LINK_RE.sub(r"\1", value).strip()


def _split_table_row(line: str) -> list[str]:
    return [part.strip() for part in line.strip().strip("|").split("|")]


def _parse_table(table_text: str) -> tuple[list[str], list[list[str]]]:
    lines = [line for line in table_text.strip().splitlines() if line.strip()]
    columns = _split_table_row(lines[0])
    rows: list[list[str]] = []

    for line in lines[2:]:
        rows.append(_split_table_row(line))

    return columns, rows


def _human_title(readme_text: str, table_start: int, kind: str) -> str:
    headings = list(HEADING_RE.finditer(readme_text[:table_start]))
    h4 = next((m.group(2).strip() for m in reversed(headings) if len(m.group(1)) == 4), None)
    h5 = next((m.group(2).strip() for m in reversed(headings) if len(m.group(1)) == 5), None)

    if kind == "latency" and h4 and h5:
        return f"{h4} — {h5}"

    if kind == "throughput":
        return "Throughput Comparison"

    return h5 or h4 or "Benchmark Chart"


def _chart_specs_from_readme(readme_text: str) -> list[ChartSpec]:
    tables = list(TABLE_RE.finditer(readme_text))
    images = list(IMAGE_RE.finditer(readme_text))
    specs: list[ChartSpec] = []

    for image in images:
        image_start = image.start()
        table_match = next((t for t in reversed(tables) if t.end() < image_start), None)
        if table_match is None:
            raise RuntimeError(f"Could not find markdown table for chart '{image.group('stem')}'.")

        columns, raw_rows = _parse_table(table_match.group("table"))
        kind = "latency" if columns[1:] == LATENCY_LABELS else "throughput"
        rows: list[tuple[str, list[float]]] = []

        for raw_row in raw_rows:
            library = _strip_link_markup(raw_row[0])
            if kind == "latency":
                values = [float(value) for value in raw_row[1:7]]
            else:
                values = [float(raw_row[1])]
            rows.append((library, values))

        specs.append(
            ChartSpec(
                stem=image.group("stem"),
                title=_human_title(readme_text, table_match.start(), kind),
                kind=kind,
                columns=columns,
                rows=rows,
            )
        )

    return specs


def _compact_number(value: float) -> str:
    if value >= 1_000_000:
        return f"{value / 1_000_000:g}M"
    if value >= 1_000:
        return f"{value / 1_000:g}k"
    if float(value).is_integer():
        return str(int(value))
    return f"{value:g}"


def _svg_text(x: float, y: float, text: str, size: int = 12, weight: str = "400",
              anchor: str = "start", fill: str = "#1e293b", extra: str = "") -> str:
    return (
        f'<text x="{x:.1f}" y="{y:.1f}" font-family="-apple-system, BlinkMacSystemFont, '
        f'Segoe UI, Roboto, Helvetica Neue, Arial, sans-serif" '
        f'font-size="{size}" font-weight="{weight}" text-anchor="{anchor}" '
        f'fill="{fill}" {extra}>{escape(text)}</text>'
    )


def _gradient_defs(rows: list[tuple[str, list[float]]], direction: str = "vertical") -> str:
    """Generate SVG gradient definitions for each library color."""
    defs: list[str] = ['<defs>']
    seen: set[str] = set()
    for library, _ in rows:
        grad_id = _grad_id(library)
        if grad_id in seen:
            continue
        seen.add(grad_id)
        base = COLOR_MAP.get(library, "#78716c")
        light = COLOR_MAP_LIGHT.get(library, base)
        if direction == "vertical":
            defs.append(
                f'<linearGradient id="{grad_id}" x1="0" y1="0" x2="0" y2="1">'
                f'<stop offset="0%" stop-color="{light}"/>'
                f'<stop offset="100%" stop-color="{base}"/>'
                f'</linearGradient>'
            )
        else:
            defs.append(
                f'<linearGradient id="{grad_id}" x1="0" y1="0" x2="1" y2="0">'
                f'<stop offset="0%" stop-color="{base}"/>'
                f'<stop offset="100%" stop-color="{light}"/>'
                f'</linearGradient>'
            )
    defs.append('</defs>')
    return "\n".join(defs)


def _grad_id(library: str) -> str:
    return "grad_" + re.sub(r"[^a-zA-Z0-9]", "_", library)


def _legend_label_lines(label: str, width: int = 26) -> list[str]:
    label = label.replace(" (", "\n(")
    pieces: list[str] = []
    for part in label.splitlines():
        pieces.extend(textwrap.wrap(part, width=width) or [""])
    return pieces[:3]


def _legend_block(x: float, y: float, rows: list[tuple[str, list[float]]], height: float) -> str:
    legend_w = 290
    fragments = [
        f'<rect x="{x:.1f}" y="{y:.1f}" width="{legend_w}" height="{height:.0f}" '
        f'rx="12" fill="#f8fafc" stroke="#e2e8f0" stroke-width="1"/>',
        _svg_text(x + 18, y + 26, "Libraries", size=13, weight="700", fill="#334155"),
    ]

    cursor_y = y + 50
    for library, _values in rows:
        color = COLOR_MAP.get(library, "#78716c")
        is_quill = library.startswith("Quill")
        stroke_attr = ' stroke="#15803d" stroke-width="1.5"' if is_quill else ""
        fragments.append(
            f'<rect x="{x + 18:.1f}" y="{cursor_y - 9:.1f}" width="12" height="12" rx="3" '
            f'fill="{color}"{stroke_attr}/>'
        )
        # Use bold for Quill entries
        fw = "600" if is_quill else "400"
        fc = "#15803d" if is_quill else "#334155"
        lines = _legend_label_lines(library)
        for idx, line in enumerate(lines):
            fragments.append(_svg_text(x + 38, cursor_y + idx * 14, line, size=11, weight=fw, fill=fc))
        cursor_y += max(22, 14 * len(lines) + 6)

    return "\n".join(fragments)


def _latency_chart_svg(spec: ChartSpec) -> str:
    width = 1480
    height = 820
    left = 80
    top = 80
    bottom = 90
    right = 340
    plot_width = width - left - right
    plot_height = height - top - bottom
    plot_bottom = top + plot_height
    plot_right = left + plot_width

    values = [value for _library, row in spec.rows for value in row if value > 0]
    max_value = max(values)
    max_power = math.ceil(math.log10(max_value))
    tick_values = [10 ** power for power in range(0, max_power + 1)]

    def y_for(value: float) -> float:
        log_value = math.log10(max(value, 1.0))
        return top + plot_height * (1 - (log_value / max_power))

    category_width = plot_width / len(LATENCY_LABELS)
    inner_group_width = category_width * 0.84
    bar_slot = inner_group_width / len(spec.rows)
    bar_width = max(5.0, bar_slot * 0.80)

    fragments = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        _gradient_defs(spec.rows, "vertical"),
        # Background
        f'<rect width="100%" height="100%" fill="#ffffff" rx="16"/>',
        # Title
        _svg_text(left, 36, spec.title, size=22, weight="700", fill="#0f172a"),
        _svg_text(left, 56, "Latency percentiles in nanoseconds — lower is better", size=12, fill="#64748b"),
        # Plot area
        f'<rect x="{left}" y="{top}" width="{plot_width}" height="{plot_height}" rx="8" fill="#f8fafc" stroke="#e2e8f0" stroke-width="1"/>',
    ]

    # Grid lines
    for tick in tick_values:
        y = y_for(tick)
        fragments.append(
            f'<line x1="{left}" y1="{y:.1f}" x2="{plot_right}" y2="{y:.1f}" stroke="#e2e8f0" stroke-width="1"/>'
        )
        fragments.append(_svg_text(left - 10, y + 4, _compact_number(tick), size=11, anchor="end", fill="#94a3b8"))

    # Value labels collected per category — we label the best (lowest) bars
    value_labels: list[str] = []

    # Category groups
    for category_idx, category in enumerate(LATENCY_LABELS):
        group_left = left + category_idx * category_width + (category_width - inner_group_width) / 2
        cx = group_left + inner_group_width / 2

        # Subtle category separator
        if category_idx > 0:
            sep_x = left + category_idx * category_width
            fragments.append(
                f'<line x1="{sep_x:.1f}" y1="{top}" x2="{sep_x:.1f}" y2="{plot_bottom}" '
                f'stroke="#f1f5f9" stroke-width="1"/>'
            )

        fragments.append(
            _svg_text(cx, plot_bottom + 22, category, size=12, weight="500", anchor="middle", fill="#475569")
        )

        # Find the best (lowest) value in this category to label it
        sorted_vals = sorted(
            [(row_values[category_idx], row_idx, library)
             for row_idx, (library, row_values) in enumerate(spec.rows)],
            key=lambda t: t[0],
        )
        # Label the top 2 distinct Quill entries in each group
        quill_labeled = set()

        for row_idx, (library, row_values) in enumerate(spec.rows):
            value = row_values[category_idx]
            bar_x = group_left + row_idx * bar_slot + (bar_slot - bar_width) / 2
            bar_y = y_for(value)
            bar_h = plot_bottom - bar_y
            grad = f"url(#{_grad_id(library)})"
            is_quill = library.startswith("Quill")
            stroke_attr = ' stroke="#15803d" stroke-width="0.8"' if is_quill else ""
            fragments.append(
                f'<rect x="{bar_x:.1f}" y="{bar_y:.1f}" width="{bar_width:.1f}" height="{bar_h:.1f}" '
                f'rx="3" fill="{grad}"{stroke_attr}/>'
            )

            # Add value label above Quill bars (top 2 only to avoid clutter)
            if is_quill and len(quill_labeled) < 2:
                label_x = bar_x + bar_width / 2
                label_y = bar_y - 4
                if label_y > top + 10:
                    val_str = str(int(value)) if value == int(value) else f"{value:.0f}"
                    value_labels.append(
                        _svg_text(label_x, label_y, val_str, size=8, weight="700",
                                  anchor="middle", fill="#15803d")
                    )
                    quill_labeled.add(library)

    # Append value labels on top of bars (drawn last so they're above everything)
    fragments.extend(value_labels)

    # Axis labels
    fragments.append(
        _svg_text(left + plot_width / 2, height - 20, "Percentile", size=12, weight="600", anchor="middle",
                  fill="#475569")
    )
    fragments.append(
        _svg_text(22, top + plot_height / 2, "Latency (ns, log scale)", size=12, weight="600",
                  anchor="middle", fill="#475569",
                  extra=f'transform="rotate(-90 22 {top + plot_height / 2:.1f})"')
    )

    # Legend
    legend_h = 38 + len(spec.rows) * 24
    fragments.append(_legend_block(plot_right + 24, top, spec.rows, legend_h))
    fragments.append("</svg>")
    return "\n".join(fragments)


def _throughput_chart_svg(spec: ChartSpec) -> str:
    rows = [(library, values) for library, values in spec.rows if library not in THROUGHPUT_EXCLUDED]
    width = 1100
    row_h = 44
    height = 110 + len(rows) * row_h
    left = 280
    top = 80
    bottom = 60
    right = 80
    plot_width = width - left - right
    plot_height = height - top - bottom
    plot_bottom = top + plot_height

    max_value = max(values[0] for _library, values in rows)
    x_max = math.ceil(max_value * 1.15)
    tick_step = 2 if x_max > 10 else 1

    def x_for(value: float) -> float:
        return left + plot_width * (value / x_max)

    fragments = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        _gradient_defs(rows, "horizontal"),
        f'<rect width="100%" height="100%" fill="#ffffff" rx="16"/>',
        _svg_text(left, 36, spec.title, size=22, weight="700", fill="#0f172a"),
        _svg_text(left, 56, "Million messages per second — higher is better", size=12, fill="#64748b"),
        f'<rect x="{left}" y="{top}" width="{plot_width}" height="{plot_height}" rx="8" fill="#f8fafc" stroke="#e2e8f0" stroke-width="1"/>',
    ]

    # Vertical grid
    tick = 0
    while tick <= x_max:
        x = x_for(tick)
        fragments.append(
            f'<line x1="{x:.1f}" y1="{top}" x2="{x:.1f}" y2="{plot_bottom}" stroke="#e2e8f0" stroke-width="1"/>'
        )
        fragments.append(_svg_text(x, plot_bottom + 20, str(tick), size=11, anchor="middle", fill="#94a3b8"))
        tick += tick_step

    bar_h = row_h * 0.58
    actual_row_h = plot_height / len(rows)

    for idx, (library, values) in enumerate(rows):
        value = values[0]
        row_top = top + idx * actual_row_h
        y = row_top + (actual_row_h - bar_h) / 2
        grad = f"url(#{_grad_id(library)})"
        is_quill = library.startswith("Quill")
        bar_right = x_for(value)
        bar_w = bar_right - left

        # Library label
        fw = "600" if is_quill else "400"
        fc = "#15803d" if is_quill else "#334155"
        fragments.append(
            _svg_text(left - 14, row_top + actual_row_h / 2 + 4, library, size=12, weight=fw, anchor="end", fill=fc))

        # Horizontal separator
        if idx > 0:
            fragments.append(
                f'<line x1="{left}" y1="{row_top:.1f}" x2="{left + plot_width}" y2="{row_top:.1f}" '
                f'stroke="#f1f5f9" stroke-width="1"/>'
            )

        # Bar
        stroke_attr = ' stroke="#15803d" stroke-width="0.8"' if is_quill else ""
        fragments.append(
            f'<rect x="{left}" y="{y:.1f}" width="{bar_w:.1f}" height="{bar_h:.1f}" rx="4" '
            f'fill="{grad}"{stroke_attr}/>'
        )

        # Value label
        fragments.append(
            _svg_text(bar_right + 8, row_top + actual_row_h / 2 + 4, f"{value:.2f}", size=12, weight="700",
                      fill="#334155")
        )

    # X-axis label
    fragments.append(
        _svg_text(left + plot_width / 2, height - 12, "Million messages / second", size=12, weight="600",
                  anchor="middle", fill="#475569")
    )
    if THROUGHPUT_EXCLUDED:
        # Keep the note concise — just show count if many are excluded
        excluded_list = sorted(THROUGHPUT_EXCLUDED)
        if len(excluded_list) <= 2:
            note = ", ".join(excluded_list) + " omitted"
        else:
            note = f"{len(excluded_list)} binary/non-comparable entries omitted"
        fragments.append(
            _svg_text(width - 16, height - 12, note, size=10, anchor="end", fill="#94a3b8")
        )

    fragments.append("</svg>")
    return "\n".join(fragments)


def _render_chart(spec: ChartSpec) -> str:
    if spec.kind == "latency":
        return _latency_chart_svg(spec)
    return _throughput_chart_svg(spec)


def main() -> None:
    readme_text = README.read_text(encoding="utf-8")
    specs = _chart_specs_from_readme(readme_text)

    CHARTS_DIR.mkdir(parents=True, exist_ok=True)
    generated: list[Path] = []

    for spec in specs:
        output_path = CHARTS_DIR / f"{spec.stem}.svg"
        output_path.write_text(_render_chart(spec), encoding="utf-8")
        generated.append(output_path)

    for path in generated:
        print(path.relative_to(ROOT))


if __name__ == "__main__":
    main()
