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
from typing import Callable

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
QUILL_VERSION_RE = re.compile(r"^- \*\*Quill Version:\*\*\s+(\S+)$", re.MULTILINE)

LATENCY_LABELS = ["50th", "75th", "90th", "95th", "99th", "99.9th"]
RANKING_PERCENTILE = "99th"
RANKING_PERCENTILE_INDEX = LATENCY_LABELS.index(RANKING_PERCENTILE)

# Categorical palette. Quill uses green/cyan; competitors remain distinct but secondary.
COLOR_MAP = {
    "Quill Bounded Dropping Queue": "#15803d",
    "Quill Unbounded Queue": "#16a34a",
    "Quill Unbounded Queue (Macro Free Mode)": "#0891b2",
    "Quill": "#16a34a",
    "Quill - Macro Free Mode": "#0891b2",
    "fmtlog": "#ea580c",
    "PlatformLab NanoLog": "#d97706",
    "MS BinLog": "#8b5cf6",
    "MS BinLog (binary log)": "#8b5cf6",
    "XTR": "#a16207",
    "Reckless": "#ef4444",
    "Iyengar NanoLog": "#6366f1",
    "spdlog": "#0284c7",
    "g3log": "#a855f7",
    "BqLog": "#d946ef",
    "BqLog (binary log)": "#a21caf",
    "Boost.Log": "#78716c",
}

# Lighter tints used by the throughput bars.
COLOR_MAP_LIGHT = {
    "Quill Bounded Dropping Queue": "#4ade80",
    "Quill Unbounded Queue": "#86efac",
    "Quill Unbounded Queue (Macro Free Mode)": "#67e8f9",
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

# Each latency series keeps a distinct marker shape so it remains traceable without color.
MARKER_SHAPE_MAP = {
    "Quill Bounded Dropping Queue": "square",
    "Quill Unbounded Queue": "circle",
    "Quill Unbounded Queue (Macro Free Mode)": "diamond",
    "Quill - Macro Free Mode": "diamond",
    "Quill": "circle",
    "fmtlog": "triangle_up",
    "PlatformLab NanoLog": "triangle_down",
    "MS BinLog": "pentagon",
    "MS BinLog (binary log)": "pentagon",
    "XTR": "hexagon",
    "Reckless": "cross",
    "Iyengar NanoLog": "star",
    "spdlog": "circle_dot",
    "g3log": "square_x",
    "BqLog": "diamond_dot",
    "BqLog (binary log)": "diamond_dot",
    "Boost.Log": "plus",
}

LATENCY_MARKER_RADIUS = 4.25
QUILL_LATENCY_MARKER_RADIUS = 6.25
MARKER_COLLISION_PADDING = 2.0
MARKER_DODGE_SPACING = 13.0

THROUGHPUT_EXCLUDED = {"MS BinLog (binary log)", "BqLog (binary log)"}


@dataclass
class ChartSpec:
    stem: str
    title: str
    kind: str
    columns: list[str]
    rows: list[tuple[str, list[float]]]
    quill_version: str


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
        thread_label = {
            "1 Thread Logging": "1 Thread",
            "4 Threads Logging Simultaneously": "4 Concurrent Threads",
        }.get(h5, h5)
        return f"{h4} — {thread_label}"

    if kind == "throughput":
        return "Throughput Comparison"

    return h5 or h4 or "Benchmark Chart"


def _chart_specs_from_readme(readme_text: str) -> list[ChartSpec]:
    tables = list(TABLE_RE.finditer(readme_text))
    images = list(IMAGE_RE.finditer(readme_text))
    version_match = QUILL_VERSION_RE.search(readme_text)
    quill_version = version_match.group(1) if version_match else ""
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
                quill_version=quill_version,
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


def _label_lines(label: str, width: int = 28, max_lines: int = 2) -> list[str]:
    label = label.replace(" (", "\n(")
    pieces: list[str] = []
    for part in label.splitlines():
        pieces.extend(textwrap.wrap(part, width=width) or [""])
    return pieces[:max_lines]


def _is_quill(library: str) -> bool:
    return library.startswith("Quill")


def _quill_tint(library: str) -> str:
    return "#ecfeff" if "Macro Free" in library else "#ecfdf5"


def _series_marker(
    library: str,
    x: float,
    y: float,
    radius: float,
    fill: str,
    stroke: str,
    stroke_width: float,
    title: str = "",
) -> str:
    title_element = f"<title>{escape(title)}</title>" if title else ""
    common = f'fill="{fill}" stroke="{stroke}" stroke-width="{stroke_width}"'
    shape = MARKER_SHAPE_MAP.get(library, "circle")
    detail_color = stroke if fill != stroke else "#ffffff"

    if shape == "square":
        return (
            f'<rect x="{x - radius:.1f}" y="{y - radius:.1f}" width="{radius * 2:.1f}" '
            f'height="{radius * 2:.1f}" rx="1.5" {common}>{title_element}</rect>'
        )

    if shape == "diamond":
        return (
            f'<path d="M {x:.1f} {y - radius:.1f} L {x + radius:.1f} {y:.1f} '
            f'L {x:.1f} {y + radius:.1f} L {x - radius:.1f} {y:.1f} Z" '
            f'{common}>{title_element}</path>'
        )

    if shape == "triangle_up":
        return (
            f'<path d="M {x:.1f} {y - radius:.1f} '
            f'L {x + radius:.1f} {y + radius * 0.5:.1f} '
            f'L {x - radius:.1f} {y + radius * 0.5:.1f} Z" '
            f'{common}>{title_element}</path>'
        )

    if shape == "triangle_down":
        return (
            f'<path d="M {x - radius:.1f} {y - radius * 0.5:.1f} '
            f'L {x + radius:.1f} {y - radius * 0.5:.1f} '
            f'L {x:.1f} {y + radius:.1f} Z" '
            f'{common}>{title_element}</path>'
        )

    if shape == "hexagon":
        half_radius = radius * 0.5
        vertical_radius = radius * 0.87
        return (
            f'<path d="M {x - radius:.1f} {y:.1f} '
            f'L {x - half_radius:.1f} {y - vertical_radius:.1f} '
            f'L {x + half_radius:.1f} {y - vertical_radius:.1f} '
            f'L {x + radius:.1f} {y:.1f} '
            f'L {x + half_radius:.1f} {y + vertical_radius:.1f} '
            f'L {x - half_radius:.1f} {y + vertical_radius:.1f} Z" '
            f'{common}>{title_element}</path>'
        )

    if shape == "pentagon":
        points = []
        for point_idx in range(5):
            angle = -math.pi / 2 + point_idx * 2 * math.pi / 5
            points.append(f"{x + radius * math.cos(angle):.1f},{y + radius * math.sin(angle):.1f}")
        return f'<polygon points="{" ".join(points)}" {common}>{title_element}</polygon>'

    if shape == "star":
        points = []
        for point_idx in range(10):
            point_radius = radius if point_idx % 2 == 0 else radius * 0.45
            angle = -math.pi / 2 + point_idx * math.pi / 5
            points.append(
                f"{x + point_radius * math.cos(angle):.1f},{y + point_radius * math.sin(angle):.1f}"
            )
        return f'<polygon points="{" ".join(points)}" {common}>{title_element}</polygon>'

    if shape == "circle_dot":
        return (
            f'<g>{title_element}<circle cx="{x:.1f}" cy="{y:.1f}" r="{radius:.1f}" {common}/>'
            f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{max(1.2, radius * 0.32):.1f}" '
            f'fill="{detail_color}"/></g>'
        )

    if shape == "square_x":
        detail_width = max(1.4, stroke_width * 0.8)
        inset = radius * 0.48
        return (
            f'<g>{title_element}<rect x="{x - radius:.1f}" y="{y - radius:.1f}" '
            f'width="{radius * 2:.1f}" height="{radius * 2:.1f}" rx="1.5" {common}/>'
            f'<path d="M {x - inset:.1f} {y - inset:.1f} L {x + inset:.1f} {y + inset:.1f} '
            f'M {x + inset:.1f} {y - inset:.1f} L {x - inset:.1f} {y + inset:.1f}" '
            f'fill="none" stroke="{detail_color}" stroke-width="{detail_width:.1f}" '
            f'stroke-linecap="round"/></g>'
        )

    if shape == "diamond_dot":
        return (
            f'<g>{title_element}<path d="M {x:.1f} {y - radius:.1f} '
            f'L {x + radius:.1f} {y:.1f} L {x:.1f} {y + radius:.1f} '
            f'L {x - radius:.1f} {y:.1f} Z" {common}/>'
            f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{max(1.2, radius * 0.30):.1f}" '
            f'fill="{detail_color}"/></g>'
        )

    if shape in {"cross", "plus"}:
        line_width = stroke_width if stroke_width > 0 else max(1.5, radius * 0.4)
        if shape == "cross":
            marker_path = (
                f'M {x - radius:.1f} {y - radius:.1f} L {x + radius:.1f} {y + radius:.1f} '
                f'M {x + radius:.1f} {y - radius:.1f} L {x - radius:.1f} {y + radius:.1f}'
            )
        else:
            marker_path = (
                f'M {x - radius:.1f} {y:.1f} L {x + radius:.1f} {y:.1f} '
                f'M {x:.1f} {y - radius:.1f} L {x:.1f} {y + radius:.1f}'
            )
        return (
            f'<path d="{marker_path}" fill="none" stroke="{stroke}" '
            f'stroke-width="{line_width}" stroke-linecap="round">{title_element}</path>'
        )

    return (
        f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{radius:.1f}" '
        f'{common}>{title_element}</circle>'
    )


def _latency_marker_style(library: str) -> tuple[float, str, str, float]:
    """Return the one canonical marker style used in both latency panels."""
    color = COLOR_MAP.get(library, "#78716c")
    if _is_quill(library):
        return QUILL_LATENCY_MARKER_RADIUS, color, "#ffffff", 2.5
    return LATENCY_MARKER_RADIUS, "#ffffff", color, 2.0


def _latency_marker_x_offsets(
    spec: ChartSpec,
    x_for: Callable[[int], float],
    y_for: Callable[[float], float],
    plot_left: float,
    plot_right: float,
) -> dict[tuple[str, int], float]:
    """Dodge visually overlapping markers without changing their value-bearing y position."""
    offsets: dict[tuple[str, int], float] = {}

    for category_idx in range(len(LATENCY_LABELS)):
        points = []
        for original_idx, (library, values) in enumerate(spec.rows):
            radius, _fill, _stroke, _stroke_width = _latency_marker_style(library)
            points.append((y_for(values[category_idx]), original_idx, library, radius))
        points.sort(key=lambda point: (point[0], point[1]))

        clusters: list[list[tuple[float, int, str, float]]] = []
        for point in points:
            if not clusters:
                clusters.append([point])
                continue

            previous = clusters[-1][-1]
            collision_distance = previous[3] + point[3] + MARKER_COLLISION_PADDING
            if point[0] - previous[0] < collision_distance:
                clusters[-1].append(point)
            else:
                clusters.append([point])

        for cluster in clusters:
            if len(cluster) == 1:
                continue
            center = (len(cluster) - 1) / 2
            cluster_offsets = [
                (cluster_idx - center) * MARKER_DODGE_SPACING
                for cluster_idx in range(len(cluster))
            ]
            category_x = x_for(category_idx)
            marker_left = min(
                category_x + offset - point[3] for point, offset in zip(cluster, cluster_offsets)
            )
            marker_right = max(
                category_x + offset + point[3] for point, offset in zip(cluster, cluster_offsets)
            )
            plot_padding = 2.0
            cluster_shift = max(0.0, plot_left + plot_padding - marker_left)
            if marker_right + cluster_shift > plot_right - plot_padding:
                cluster_shift -= marker_right + cluster_shift - (plot_right - plot_padding)

            for point, offset in zip(cluster, cluster_offsets):
                offsets[(point[2], category_idx)] = offset + cluster_shift

    return offsets


def _accessible_svg_start(spec: ChartSpec, width: int, height: int, description: str) -> list[str]:
    chart_id = re.sub(r"[^a-zA-Z0-9_-]", "_", spec.stem)
    return [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" role="img" '
        f'aria-labelledby="{chart_id}_title {chart_id}_desc">',
        f'<title id="{chart_id}_title">{escape(spec.title)}</title>',
        f'<desc id="{chart_id}_desc">{escape(description)}</desc>',
    ]


def _latency_ranking_panel(
    spec: ChartSpec, x: float, y: float, width: float, height: float
) -> str:
    ranked_rows = sorted(
        enumerate(spec.rows),
        key=lambda item: (item[1][1][RANKING_PERCENTILE_INDEX], item[0]),
    )
    header_height = 58
    available_height = height - header_height - 12
    row_height = available_height / len(ranked_rows)
    list_top = y + header_height
    label_size = 13 if row_height >= 48 else 12
    value_size = 14 if row_height >= 48 else 13

    fragments = [
        f'<rect x="{x:.1f}" y="{y:.1f}" width="{width:.1f}" height="{height:.1f}" '
        f'rx="14" fill="#ffffff" stroke="#dbe5ee" stroke-width="1"/>',
        _svg_text(
            x + 18,
            y + 22,
            "TAIL RANKING",
            size=11,
            weight="700",
            fill="#15803d",
            extra='letter-spacing="1.2"',
        ),
        _svg_text(
            x + 18,
            y + 44,
            f"{RANKING_PERCENTILE} percentile",
            size=17,
            weight="700",
            fill="#172033",
        ),
        _svg_text(x + width - 18, y + 44, "ns", size=13, weight="600", anchor="end", fill="#64748b"),
        _svg_text(
            x + width - 18,
            y + 22,
            "COLOR + SYMBOL KEY",
            size=9,
            weight="700",
            anchor="end",
            fill="#64748b",
            extra='letter-spacing="0.5"',
        ),
    ]

    # Competition ranking keeps equal percentile values equal (1, 1, 3 rather than 1, 2, 3).
    previous_value = None
    display_rank = 0
    for row_idx, (_original_idx, (library, values)) in enumerate(ranked_rows):
        ranking_value = values[RANKING_PERCENTILE_INDEX]
        if previous_value is None or ranking_value != previous_value:
            display_rank = row_idx + 1
            previous_value = ranking_value

        row_top = list_top + row_idx * row_height
        center_y = row_top + row_height / 2
        is_quill = _is_quill(library)
        color = COLOR_MAP.get(library, "#78716c")

        if is_quill:
            fragments.append(
                f'<rect x="{x + 8:.1f}" y="{row_top + 2:.1f}" width="{width - 16:.1f}" '
                f'height="{row_height - 4:.1f}" rx="8" fill="{_quill_tint(library)}"/>'
            )
        elif row_idx % 2 == 1:
            fragments.append(
                f'<rect x="{x + 8:.1f}" y="{row_top + 2:.1f}" width="{width - 16:.1f}" '
                f'height="{row_height - 4:.1f}" rx="8" fill="#f8fafc"/>'
            )

        fragments.append(
            _svg_text(
                x + 21,
                center_y + 4,
                f"#{display_rank}",
                size=11,
                weight="700",
                anchor="middle",
                fill="#64748b",
            )
        )
        fragments.append(
            f'<line x1="{x + 39:.1f}" y1="{center_y:.1f}" x2="{x + 57:.1f}" y2="{center_y:.1f}" '
            f'stroke="{color}" stroke-width="3.2" stroke-linecap="round"/>'
        )
        marker_radius, marker_fill, marker_stroke, marker_stroke_width = _latency_marker_style(library)
        fragments.append(
            _series_marker(
                library,
                x + 48,
                center_y,
                marker_radius,
                marker_fill,
                marker_stroke,
                marker_stroke_width,
            )
        )

        lines = _label_lines(library)
        first_line_y = center_y + 4 if len(lines) == 1 else center_y - 3.5
        for line_idx, line in enumerate(lines):
            fragments.append(
                _svg_text(
                    x + 68,
                    first_line_y + line_idx * 15,
                    line,
                    size=label_size,
                    weight="700" if is_quill else "500",
                    fill="#15803d" if is_quill else "#334155",
                )
            )

        fragments.append(
            _svg_text(
                x + width - 18,
                center_y + 4,
                _compact_number(ranking_value),
                size=value_size,
                weight="700",
                anchor="end",
                fill="#15803d" if is_quill else "#334155",
            )
        )

    return "\n".join(fragments)


def _latency_chart_svg(spec: ChartSpec) -> str:
    width = 1480
    height = 760
    plot_left = 92
    plot_top = 104
    plot_right = 1045
    plot_bottom = 694
    plot_width = plot_right - plot_left
    plot_height = plot_bottom - plot_top
    data_left = plot_left + 28
    data_right = plot_right - 28
    panel_left = 1074
    panel_width = width - panel_left - 48

    values = [value for _library, row in spec.rows for value in row if value > 0]
    min_power = math.floor(math.log10(min(values)))
    max_power = math.ceil(math.log10(max(values)))
    if min_power == max_power:
        max_power += 1

    axis_min = 10 ** min_power
    axis_max = 10 ** max_power

    def y_for(value: float) -> float:
        log_position = (math.log10(value) - min_power) / (max_power - min_power)
        return plot_bottom - plot_height * log_position

    def x_for(index: int) -> float:
        return data_left + index * (data_right - data_left) / (len(LATENCY_LABELS) - 1)

    marker_x_offsets = _latency_marker_x_offsets(spec, x_for, y_for, plot_left, plot_right)

    version_description = f" Quill results use {spec.quill_version}." if spec.quill_version else ""
    description = (
        f"Log-scale frontend latency profile for {len(spec.rows)} logging libraries. "
        "Series use color and marker shape. Lower values are better; Quill series are emphasized "
        f"and the panel ranks {RANKING_PERCENTILE}-percentile latency. "
        "Equal percentile values share a rank. Visually overlapping markers are offset "
        "horizontally while retaining their exact value-bearing vertical position."
        f"{version_description}"
    )
    version_suffix = f" • Quill {spec.quill_version}" if spec.quill_version else ""
    fragments = _accessible_svg_start(spec, width, height, description)
    fragments.extend(
        [
            f'<rect x="0.5" y="0.5" width="{width - 1}" height="{height - 1}" '
            'rx="18" fill="#ffffff" '
            'stroke="#dbe5ee"/>',
            '<rect x="48" y="25" width="6" height="43" rx="3" fill="#22c55e"/>',
            _svg_text(68, 44, spec.title, size=25, weight="700", fill="#111827"),
            _svg_text(
                68,
                67,
                f"Frontend latency across percentiles • nanoseconds{version_suffix}",
                size=14,
                weight="500",
                fill="#64748b",
            ),
            '<rect x="1212" y="28" width="98" height="28" rx="14" fill="#f1f5f9"/>',
            _svg_text(
                1261, 46, "LOG SCALE", size=11, weight="700", anchor="middle",
                fill="#475569", extra='letter-spacing="0.8"'
            ),
            '<rect x="1318" y="28" width="114" height="28" rx="14" fill="#ecfdf5"/>',
            _svg_text(
                1375, 46, "LOWER IS BETTER", size=11, weight="700", anchor="middle",
                fill="#15803d", extra='letter-spacing="0.5"'
            ),
            f'<rect x="{plot_left}" y="{plot_top}" width="{plot_width}" height="{plot_height}" '
            f'rx="14" fill="#fbfdff" stroke="#dbe5ee"/>',
        ]
    )

    # Minor log-scale guides at 2x and 5x within each decade.
    for power in range(min_power, max_power):
        for factor in (2, 5):
            tick = factor * (10 ** power)
            if axis_min < tick < axis_max:
                y = y_for(tick)
                fragments.append(
                    f'<line x1="{plot_left}" y1="{y:.1f}" x2="{plot_right}" y2="{y:.1f}" '
                    'stroke="#edf2f7" stroke-width="1"/>'
                )

    tick_values = [10 ** power for power in range(min_power, max_power + 1)]
    for tick in tick_values:
        y = y_for(tick)
        fragments.append(
            f'<line x1="{plot_left}" y1="{y:.1f}" x2="{plot_right}" y2="{y:.1f}" '
            'stroke="#dbe5ee" stroke-width="1"/>'
        )
        fragments.append(
            _svg_text(
                plot_left - 12, y + 4, _compact_number(tick), size=12,
                weight="600", anchor="end", fill="#64748b"
            )
        )

    # The ranked percentile gets a quiet highlight linking it to the side panel.
    x_step = (data_right - data_left) / (len(LATENCY_LABELS) - 1)
    ranking_x = x_for(RANKING_PERCENTILE_INDEX)
    fragments.append(
        f'<rect x="{ranking_x - x_step * 0.27:.1f}" y="{plot_top + 1}" '
        f'width="{x_step * 0.54:.1f}" '
        f'height="{plot_height - 2}" rx="10" fill="#f0fdf4" opacity="0.72"/>'
    )

    for category_idx, category in enumerate(LATENCY_LABELS):
        x = x_for(category_idx)
        fragments.append(
            f'<line x1="{x:.1f}" y1="{plot_top}" x2="{x:.1f}" y2="{plot_bottom}" '
            'stroke="#e5ebf2" stroke-width="1" stroke-dasharray="3 5"/>'
        )
        fragments.append(
            _svg_text(
                x,
                plot_bottom + 25,
                category,
                size=13,
                weight="700" if category_idx == RANKING_PERCENTILE_INDEX else "600",
                anchor="middle",
                fill="#15803d" if category_idx == RANKING_PERCENTILE_INDEX else "#475569",
            )
        )

    # Competitors are drawn first; Quill lines sit on top with a white separation stroke.
    ordered_rows = [row for row in spec.rows if not _is_quill(row[0])]
    ordered_rows.extend(row for row in spec.rows if _is_quill(row[0]))
    for library, row_values in ordered_rows:
        color = COLOR_MAP.get(library, "#78716c")
        is_quill = _is_quill(library)
        points = " ".join(
            f"{x_for(index):.1f},{y_for(value):.1f}" for index, value in enumerate(row_values)
        )

        if is_quill:
            fragments.append(
                f'<polyline points="{points}" fill="none" stroke="#ffffff" stroke-width="8.5" '
                'stroke-linecap="round" stroke-linejoin="round"/>'
            )
        fragments.append(
            f'<polyline points="{points}" fill="none" stroke="{color}" '
            f'stroke-width="{4.2 if is_quill else 2.5}" opacity="{1 if is_quill else 0.80}" '
            'stroke-linecap="round" stroke-linejoin="round"/>'
        )

        for category_idx, value in enumerate(row_values):
            cx = x_for(category_idx) + marker_x_offsets.get((library, category_idx), 0.0)
            cy = y_for(value)
            marker_radius, marker_fill, marker_stroke, marker_stroke_width = _latency_marker_style(library)
            fragments.append(
                _series_marker(
                    library,
                    cx,
                    cy,
                    marker_radius,
                    marker_fill,
                    marker_stroke,
                    marker_stroke_width,
                    f"{library}: {_compact_number(value)} ns at {LATENCY_LABELS[category_idx]}",
                )
            )

    fragments.append(
        _svg_text(
            (plot_left + plot_right) / 2,
            height - 17,
            "Percentile",
            size=12,
            weight="700",
            anchor="middle",
            fill="#64748b",
            extra='letter-spacing="0.5"',
        )
    )
    fragments.append(
        _svg_text(
            24,
            (plot_top + plot_bottom) / 2,
            "Latency (ns)",
            size=12,
            weight="700",
            anchor="middle",
            fill="#64748b",
            extra=f'transform="rotate(-90 24 {(plot_top + plot_bottom) / 2:.1f})" letter-spacing="0.5"',
        )
    )
    fragments.append(
        _latency_ranking_panel(spec, panel_left, plot_top, panel_width, plot_height)
    )
    fragments.append("</svg>")
    return "\n".join(fragments)


def _throughput_chart_svg(spec: ChartSpec) -> str:
    rows = sorted(
        (
            (library, values)
            for library, values in spec.rows
            if library not in THROUGHPUT_EXCLUDED
        ),
        key=lambda row: row[1][0],
        reverse=True,
    )
    width = 1200
    row_height = 50
    plot_top = 112
    plot_left = 338
    plot_right = 1130
    plot_width = plot_right - plot_left
    plot_height = len(rows) * row_height
    plot_bottom = plot_top + plot_height
    height = plot_bottom + 92

    max_value = max(values[0] for _library, values in rows)
    x_max = math.ceil(max_value * 1.10)
    tick_step = 2 if x_max > 10 else 1

    def x_for(value: float) -> float:
        return plot_left + plot_width * (value / x_max)

    version_description = f" Quill results use {spec.quill_version}." if spec.quill_version else ""
    description = (
        f"Ranked backend throughput for {len(rows)} text-output logging modes in millions "
        "of messages per second. Higher values are better; equal rates share a rank and "
        f"binary-output modes are omitted.{version_description}"
    )
    version_suffix = f" • Quill {spec.quill_version}" if spec.quill_version else ""
    fragments = _accessible_svg_start(spec, width, height, description)
    fragments.extend(
        [
            _gradient_defs(rows, "horizontal"),
            f'<rect x="0.5" y="0.5" width="{width - 1}" height="{height - 1}" rx="18" '
            'fill="#ffffff" stroke="#dbe5ee"/>',
            '<rect x="48" y="25" width="6" height="43" rx="3" fill="#22c55e"/>',
            _svg_text(68, 44, spec.title, size=25, weight="700", fill="#111827"),
            _svg_text(
                68,
                67,
                f"Backend output rate • human-readable text modes{version_suffix}",
                size=14,
                weight="500",
                fill="#64748b",
            ),
            '<rect x="1012" y="28" width="140" height="28" rx="14" fill="#ecfdf5"/>',
            _svg_text(
                1082, 46, "HIGHER IS BETTER", size=11, weight="700", anchor="middle",
                fill="#15803d", extra='letter-spacing="0.5"'
            ),
        ]
    )

    # Quill rows receive a full-width tint so they remain visible even at README scale.
    for row_idx, (library, _values) in enumerate(rows):
        if _is_quill(library):
            row_top = plot_top + row_idx * row_height
            fragments.append(
                f'<rect x="32" y="{row_top + 3:.1f}" width="1136" height="{row_height - 6:.1f}" '
                f'rx="10" fill="{_quill_tint(library)}"/>'
            )

    # Grid and scale labels sit behind the bar tracks.
    tick = 0
    while tick <= x_max:
        x = x_for(tick)
        fragments.append(
            f'<line x1="{x:.1f}" y1="{plot_top}" x2="{x:.1f}" y2="{plot_bottom}" '
            'stroke="#e5ebf2" stroke-width="1"/>'
        )
        fragments.append(
            _svg_text(
                x,
                plot_bottom + 22,
                str(tick),
                size=12,
                weight="600",
                anchor="middle",
                fill="#64748b",
            )
        )
        tick += tick_step

    # Keep rank labels correct if a future benchmark produces equal results.
    previous_value = None
    display_rank = 0
    for idx, (library, values) in enumerate(rows):
        value = values[0]
        if previous_value is None or value != previous_value:
            display_rank = idx + 1
            previous_value = value

        row_top = plot_top + idx * row_height
        center_y = row_top + row_height / 2
        bar_height = 20
        bar_y = center_y - bar_height / 2
        grad = f"url(#{_grad_id(library)})"
        color = COLOR_MAP.get(library, "#78716c")
        is_quill = _is_quill(library)
        bar_right = x_for(value)
        bar_width = bar_right - plot_left

        fragments.append(
            _svg_text(
                57,
                center_y + 4,
                f"#{display_rank}",
                size=12,
                weight="700",
                anchor="middle",
                fill="#64748b",
            )
        )
        fragments.append(
            _series_marker(
                library,
                84,
                center_y,
                5.5,
                color,
                color,
                0,
            )
        )
        fragments.append(
            _svg_text(
                100,
                center_y + 5,
                library,
                size=14,
                weight="700" if is_quill else "500",
                fill="#15803d" if is_quill else "#334155",
            )
        )
        fragments.append(
            f'<rect x="{plot_left}" y="{bar_y:.1f}" width="{plot_width}" height="{bar_height}" '
            'rx="10" fill="#edf2f7"/>'
        )
        stroke_attr = f' stroke="{color}" stroke-width="1"' if is_quill else ""
        fragments.append(
            f'<rect x="{plot_left}" y="{bar_y:.1f}" width="{bar_width:.1f}" height="{bar_height}" rx="10" '
            f'fill="{grad}"{stroke_attr}/>'
        )
        fragments.append(
            _svg_text(
                min(bar_right + 10, width - 54),
                center_y + 5,
                f"{value:.2f}",
                size=13,
                weight="700",
                fill="#15803d" if is_quill else "#334155",
            )
        )

    fragments.append(
        _svg_text(
            (plot_left + plot_right) / 2,
            plot_bottom + 48,
            "Million messages / second",
            size=12,
            weight="700",
            anchor="middle",
            fill="#64748b",
            extra='letter-spacing="0.5"',
        )
    )
    if THROUGHPUT_EXCLUDED:
        note = (
            f"Text-output comparison • {len(THROUGHPUT_EXCLUDED)} binary modes omitted "
            "(shown in the table above)"
        )
        fragments.append(
            _svg_text(48, height - 22, note, size=12, weight="600", fill="#64748b")
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
