/*
 * Interactive benchmark charts for the benchmark pages of the documentation.
 *
 * The tables on the page are the only source of numbers: every <div class="bench-chart"> is drawn
 * from the table right after it. Chart settings (view, percentile, scale, binary logs, the
 * highlighted library) are shared by every chart on the page and remembered between visits.
 *
 * How a chart reads:
 *  - Latency is a dot. A latency axis is usually logarithmic, where the length of a bar would mean
 *    nothing, so the value is carried by the position of the dot alone. Its stem starts at the
 *    timer floor: what it spans is the part of the number that is the library, not the measurement.
 *  - Throughput is a bar from zero on a linear axis, where length is the value.
 *  - The distribution view keeps the dot at the chosen percentile and adds a band for every step
 *    from the 50th to the 99th percentile and a thin line out to the 99.9th.
 *  - "Timer only (no logging)" is the measurement floor: a dashed line, never a library.
 *  - The rows are in the order of the table. Nothing numbers them or names a winner: many results
 *    are tied within noise, and the order already says what there is to say.
 *  - A change of setting or tab is shown as a short move from the old picture to the new one.
 *    A page that is loading, a reader who asked for less motion and a hidden tab get no motion.
 */
(function () {
  "use strict";

  // The benchmark pages always open on their first tabs: Linux Intel (GCC), the first payload and one thread. The
  // theme remembers the last tab chosen on any page and selects it again when a page loads; this script runs
  // before that, so the memory is cleared on these pages. Tabs with the same label still switch together.
  if (/\/benchmarks[^/]*\.html$/.test(window.location.pathname)) {
    try {
      for (let i = window.localStorage.length - 1; i >= 0; --i) {
        const key = window.localStorage.key(i);
        if (key && key.endsWith("__tabs")) window.localStorage.removeItem(key);
      }
    } catch (e) {
      // Storage can be unavailable, for example in a private window with storage disabled
    }
    // A reload also restores which tab was checked, as the browser does for any form control
    document.addEventListener("DOMContentLoaded", () => {
      for (const set of document.querySelectorAll(".md-typeset .tabbed-set")) {
        const first = set.querySelector(":scope > input");
        if (first && !first.checked) {
          first.checked = true;
          first.dispatchEvent(new Event("change", { bubbles: true }));
        }
      }
    });
  }

  const PERCENTILES = ["50th", "75th", "90th", "95th", "99th", "99.9th"];
  const STORAGE_KEY = "quill-benchmark-charts";
  const SVG_NS = "http://www.w3.org/2000/svg";

  const defaults = { view: "ranking", percentile: "90th", scale: "auto", binary: false, pinned: null };
  // Auto switches to a log scale when the largest value is this many times the smallest
  const AUTO_LOG_RATIO = 30;
  // Below this chart width a library's name sits above its mark instead of beside it
  const NARROW_WIDTH = 560;
  // Every change of view, percentile, scale or tab is one short move from the old picture to the new
  const DURATION = 440;

  const state = Object.assign({}, defaults, loadState());
  const charts = [];
  let unit = 1;
  let stickyHeader;

  function loadState() {
    try {
      const saved = JSON.parse(window.localStorage.getItem(STORAGE_KEY) || "{}") || {};
      const clean = {};
      if (saved.view === "ranking" || saved.view === "distribution") clean.view = saved.view;
      if (PERCENTILES.includes(saved.percentile)) clean.percentile = saved.percentile;
      if (["auto", "linear", "log"].includes(saved.scale)) clean.scale = saved.scale;
      if (typeof saved.binary === "boolean") clean.binary = saved.binary;
      if (typeof saved.pinned === "string" && saved.pinned) clean.pinned = saved.pinned;
      return clean;
    } catch (e) {
      return {};
    }
  }

  function saveState() {
    try {
      window.localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
    } catch (e) {
      /* storage may be unavailable; the charts still work for this visit */
    }
  }

  function setState(changes, keep) {
    const changed = Object.keys(changes).filter((key) => state[key] !== changes[key]);
    if (!changed.length) return;
    Object.assign(state, changes);
    if (!keep) saveState();
    const pinOnly = changed.length === 1 && changed[0] === "pinned";
    charts.forEach((chart) => chart.update(pinOnly));
  }

  /* ---------- reading the tables ---------- */

  function cellText(cell) {
    return cell.textContent.replace(/\s+/g, " ").trim();
  }

  function decimalsOf(text) {
    const match = text.match(/\.(\d+)/);
    return match ? match[1].length : 0;
  }

  function parseTable(table) {
    const header = Array.from(table.querySelectorAll("thead th")).map(cellText);
    const bodyRows = Array.from(table.querySelectorAll("tbody tr"));
    const isLatency = PERCENTILES.every((label, i) => header[i + 1] === label);
    const rows = bodyRows.map((tr) => {
      const cells = Array.from(tr.children).map(cellText);
      const name = cells[0];
      if (isLatency) {
        const values = {};
        PERCENTILES.forEach((label, i) => (values[label] = parseFloat((cells[i + 1] || "").replace(/,/g, ""))));
        // "Argument only" is a reference row: the cost of the argument alone, drawn but marked as no library
        return { name, tr, values, floor: /^timer only/i.test(name), reference: /^argument only/i.test(name), quill: isQuill(name) };
      }
      return {
        name,
        tr,
        throughput: parseFloat((cells[1] || "").replace(/,/g, "")),
        elapsed: cells[2] || "",
        producers: cells[3] || "",
        binary: /binary log/i.test(name),
        quill: isQuill(name),
      };
    }).filter((row) => row.name && (isLatency
      ? PERCENTILES.every((label) => Number.isFinite(row.values[label]))
      : Number.isFinite(row.throughput)));
    // The table's own precision, so the chart shows values the way the table does
    const decimals = Math.max(0, ...bodyRows.flatMap((tr) => Array.from(tr.children).slice(1, isLatency ? 7 : 2)
      .map((td) => decimalsOf(cellText(td)))));
    return { kind: isLatency ? "latency" : "throughput", rows, decimals };
  }

  /* ---------- small helpers ---------- */

  function el(tag, attrs, parent) {
    const node = tag.startsWith("svg:") ? document.createElementNS(SVG_NS, tag.slice(4)) : document.createElement(tag);
    Object.entries(attrs || {}).forEach(([key, value]) => {
      if (value === null || value === undefined || value === false) return;
      if (key === "text") node.textContent = value;
      else node.setAttribute(key, value);
    });
    if (parent) parent.appendChild(node);
    return node;
  }

  function icon(paths, parent) {
    const svg = el("svg:svg", { class: "bench-chart__icon", viewBox: "0 0 16 16", "aria-hidden": "true", focusable: "false" }, parent);
    paths.forEach((d) => el("svg:path", { d }, svg));
    return svg;
  }

  const ICONS = {
    ranking: ["M2.5 4h11", "M2.5 8h7.5", "M2.5 12h4.5"],
    distribution: ["M2.5 4h6", "M5.5 8h8", "M3.5 12h5", "M9.6 4a1.1 1.1 0 1 0 .01 0", "M4.4 8a1.1 1.1 0 1 0 .01 0", "M9.6 12a1.1 1.1 0 1 0 .01 0"],
    lower: ["M8 2.8v10", "M4.2 9l3.8 3.8L11.8 9"],
    higher: ["M8 13.2v-10", "M4.2 7L8 3.2 11.8 7"],
    close: ["M4.5 4.5l7 7", "M11.5 4.5l-7 7"],
  };

  function isQuill(name) {
    return /^Quill\b/.test(name);
  }

  // A library keeps its highlight between its text and its binary mode
  function pinKey(name) {
    return name ? name.replace(/\s*\(binary log\)\s*$/i, "") : name;
  }

  function isPinned(name) {
    return Boolean(state.pinned) && pinKey(name) === pinKey(state.pinned);
  }

  function clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
  }

  function snap(value) {
    return Math.round(value * 2) / 2;
  }

  // Values are shown with the decimals of their table: whole nanoseconds, or tenths
  function formatNumber(value, decimals) {
    return value.toLocaleString("en-US", { minimumFractionDigits: decimals, maximumFractionDigits: decimals });
  }

  function formatTick(value) {
    return value.toLocaleString("en-US", { maximumFractionDigits: 6 });
  }

  // The nearest 1, 2 or 5 times a power of ten, at or below (or at or above) a value
  function nice125(value, up) {
    const base = Math.pow(10, Math.floor(Math.log10(value)));
    const fraction = value / base;
    const steps = [1, 2, 5, 10];
    const pick = up ? steps.find((s) => s >= fraction - 1e-9) : steps.slice().reverse().find((s) => s <= fraction + 1e-9);
    return parseFloat((pick * base).toPrecision(12));
  }

  function niceStep(raw) {
    const power = Math.pow(10, Math.floor(Math.log10(raw)));
    const fraction = raw / power;
    const nice = fraction <= 1 ? 1 : fraction <= 2 ? 2 : fraction <= 2.5 ? 2.5 : fraction <= 5 ? 5 : 10;
    return nice * power;
  }

  /*
   * A scale maps a value to a distance from the left edge of the plot. Ticks carry a level: 0 for
   * the lines that are always drawn and labelled, 1 and 2 for the finer ones of a log scale.
   */
  function logScale(min, max, width, fine) {
    let lo = nice125(min / 1.12, false);
    let hi = nice125(max, true);
    if (hi <= lo) hi = lo * 10;
    const span = Math.log10(hi / lo);
    const map = (v) => (Math.log10(Math.max(v, lo) / lo) / span) * width;
    const perDecade = width / span;
    const ticks = [];
    for (let exp = Math.floor(Math.log10(lo)); exp <= Math.ceil(Math.log10(hi)); exp++) {
      for (let m = 1; m < 10; m++) {
        const value = parseFloat((m * Math.pow(10, exp)).toPrecision(12));
        if (value < lo * (1 - 1e-9) || value > hi * (1 + 1e-9)) continue;
        const level = m === 1 ? 0 : m === 2 || m === 5 ? 1 : 2;
        const edge = Math.abs(value - lo) < lo * 1e-9 || Math.abs(value - hi) < hi * 1e-9;
        if (level === 2 && (!fine || perDecade < 110)) continue;
        if (level === 1 && perDecade < (fine ? 56 : 90) && !edge) continue;
        ticks.push({ value, x: map(value), level, label: level === 0 || edge || (level === 1 && perDecade >= 150), priority: level === 0 ? 2 : edge ? 1 : 0 });
      }
    }
    return { kind: "log", lo, hi, map, ticks };
  }

  function linearScale(max, width) {
    // Of the round steps, the one that leaves the least empty axis with a comfortable number of lines
    const most = Math.max(2, Math.floor(width / (44 * unit)));
    const target = clamp(Math.round(width / (100 * unit)), 3, 7);
    let best = null;
    for (let exp = Math.floor(Math.log10(max)) - 2; exp <= Math.ceil(Math.log10(max)); exp++) {
      [1, 2, 2.5, 5].forEach((m) => {
        const step = m * Math.pow(10, exp);
        const count = Math.max(1, Math.ceil(max / step - 1e-9));
        if (count > most) return;
        const score = (count * step - max) / (count * step) + Math.abs(count - target) / 20;
        if (!best || score < best.score - 1e-9 || (Math.abs(score - best.score) <= 1e-9 && step > best.step)) best = { step, count, score };
      });
    }
    if (!best) best = { step: niceStep(max), count: 1 };
    const hi = parseFloat((best.count * best.step).toPrecision(12));
    const map = (v) => (v / hi) * width;
    const ticks = [];
    for (let i = 0; i <= best.count; i++) {
      const value = parseFloat((i * best.step).toPrecision(12));
      ticks.push({ value, x: map(value), level: 1, label: true, priority: i === 0 ? 3 : i === best.count ? 2 : i % 2 ? 0 : 1 });
    }
    return { kind: "linear", lo: 0, hi, map, ticks };
  }

  function easeInOut(t) {
    return t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
  }

  function easeOut(t) {
    return 1 - Math.pow(1 - t, 3);
  }

  const reducedMotion = window.matchMedia ? window.matchMedia("(prefers-reduced-motion: reduce)") : { matches: false };
  const coarsePointer = window.matchMedia ? window.matchMedia("(pointer: coarse)") : { matches: false };

  // Nothing moves while the page is still arriving: the theme may be restoring the reader's tabs,
  // and a chart should simply be there. After that, changes are shown as a move.
  let settled = false;
  function settle() {
    window.setTimeout(() => (settled = true), 400);
  }
  if (document.readyState === "complete") settle();
  else window.addEventListener("load", settle);

  function motionAllowed() {
    return settled && !reducedMotion.matches && document.visibilityState !== "hidden" &&
      typeof window.requestAnimationFrame === "function";
  }

  function lerp(a, b, k) {
    return a + (b - a) * k;
  }

  function mixNumbers(from, to, k) {
    const out = {};
    Object.keys(to).forEach((key) => (out[key] = key in from ? lerp(from[key], to[key], k) : to[key]));
    return out;
  }

  /* ---------- controls ---------- */

  // A single-choice group: one tab stop, arrow keys move the choice
  function segmented(parent, key, options, onPick) {
    const group = el("div", { class: `bench-chart__group bench-chart__group--${key}` }, parent);
    const label = el("span", { class: "bench-chart__group-label" }, group);
    const track = el("div", { class: "bench-chart__segmented", role: "radiogroup" }, group);
    const buttons = options.map(([value, text, title, glyph]) => {
      const button = el("button", { type: "button", class: "bench-chart__toggle", role: "radio", title }, track);
      if (glyph) icon(ICONS[glyph], button);
      const span = el("span", { class: "bench-chart__toggle-text", "data-wide": text }, button);
      const inner = el("span", {}, span);
      // "th" is dropped on narrow charts, where six percentiles share one row
      const suffix = /^([\d.]+)(th)$/.exec(text);
      if (suffix) {
        inner.appendChild(document.createTextNode(suffix[1]));
        el("span", { class: "bench-chart__th", text: suffix[2] }, inner);
        button.setAttribute("aria-label", `${text} percentile`);
      } else {
        inner.textContent = text;
      }
      button.addEventListener("click", () => onPick(value));
      return { value, button };
    });
    track.addEventListener("keydown", (event) => {
      const index = buttons.findIndex((b) => b.button === document.activeElement);
      if (index < 0) return;
      let next = -1;
      if (event.key === "ArrowRight" || event.key === "ArrowDown") next = (index + 1) % buttons.length;
      else if (event.key === "ArrowLeft" || event.key === "ArrowUp") next = (index - 1 + buttons.length) % buttons.length;
      else if (event.key === "Home") next = 0;
      else if (event.key === "End") next = buttons.length - 1;
      if (next < 0) return;
      event.preventDefault();
      buttons[next].button.focus();
      onPick(buttons[next].value);
    });
    return {
      sync(current, text) {
        label.textContent = text;
        track.setAttribute("aria-label", text);
        buttons.forEach(({ value, button }) => {
          const on = value === current;
          button.setAttribute("aria-checked", String(on));
          button.tabIndex = on ? 0 : -1;
        });
      },
    };
  }

  /* ---------- chart ---------- */

  function Chart(container, table) {
    this.container = container;
    this.data = parseTable(table);
    this.kind = this.data.kind;
    this.title = container.dataset.title || "";
    this.views = new Map();
    this.order = [];
    this.width = 0;
    this.visible = false;
    this.stale = true;
    this.current = null;
    this.raf = 0;
    this.textCache = new Map();
    this.clipCount = 0;
    this.uid = `bench-chart-${charts.length + 1}`;
    // The outermost tab set: only one of its charts is on screen at a time
    let root = null;
    for (let node = container.parentElement; node; node = node.parentElement) {
      if (node.classList && node.classList.contains("tabbed-set")) root = node;
    }
    this.root = root;

    container.classList.add(`bench-chart--${this.kind}`, "is-ready");
    container.setAttribute("role", "figure");
    container.setAttribute("aria-label", this.title);

    const header = el("div", { class: "bench-chart__header" }, container);
    const parts = this.title.split(/\s+·\s+/);
    el("div", { class: "bench-chart__title", text: parts[0] }, header);
    el("div", { class: "bench-chart__title-rest", text: parts.slice(1).join(" · ") }, header);
    this.subtitle = el("div", { class: "bench-chart__subtitle" }, header);
    const badge = el("div", { class: "bench-chart__badge" }, header);
    icon(this.kind === "latency" ? ICONS.lower : ICONS.higher, badge);
    el("span", { text: this.kind === "latency" ? "Lower is better" : "Higher is better" }, badge);

    this.controls = el("div", { class: "bench-chart__controls" }, container);
    this.buildControls();

    this.plot = el("div", { class: "bench-chart__plot" }, container);
    this.svg = el("svg:svg", { class: "bench-chart__svg", role: "group", width: "100%", height: 0 }, this.plot);
    const defs = el("svg:defs", {}, this.svg);
    ["other", "quill", "pinned"].forEach((series) => {
      // Binary logs are hatched, so they never read as comparable with text output
      const pattern = el("svg:pattern", {
        id: `${this.uid}-hatch-${series}`, class: `bench-chart__hatch bench-chart__hatch--${series}`,
        patternUnits: "userSpaceOnUse", width: 6, height: 6, patternTransform: "rotate(45)",
      }, defs);
      el("svg:rect", { class: "bench-chart__hatch-ground", width: 6, height: 6 }, pattern);
      el("svg:rect", { class: "bench-chart__hatch-line", width: 2.4, height: 6 }, pattern);
    });
    this.axisLayer = el("svg:g", { class: "bench-chart__axis-layer", "aria-hidden": "true" }, this.svg);
    this.floorLayer = el("svg:g", { class: "bench-chart__floor", "aria-hidden": "true" }, this.svg);
    this.floorBand = el("svg:path", { class: "bench-chart__floor-band" }, this.floorLayer);
    this.floorLine = el("svg:path", { class: "bench-chart__floor-line" }, this.floorLayer);
    this.rowLayer = el("svg:g", { class: "bench-chart__rows" }, this.svg);
    this.noteLayer = el("svg:g", { class: "bench-chart__notes", "aria-hidden": "true" }, this.svg);
    this.floorLabel = el("svg:text", { class: "bench-chart__floor-label" }, this.noteLayer);
    el("svg:tspan", { class: "bench-chart__floor-name", text: "Timer only" }, this.floorLabel);
    this.floorValue = el("svg:tspan", { class: "bench-chart__floor-value", dx: "0.45em" }, this.floorLabel);
    this.floorTitle = el("svg:title", {}, this.floorLabel);
    this.caption = el("svg:text", { class: "bench-chart__caption", "text-anchor": "end" }, this.noteLayer);

    // Text is measured where it is drawn, so the widths are those of the page's own font
    const ruler = el("svg:svg", { class: "bench-chart__ruler", "aria-hidden": "true", focusable: "false", width: 0, height: 0 }, container);
    this.ruler = el("svg:text", {}, ruler);

    const footer = el("div", { class: "bench-chart__footer" }, container);
    this.legend = el("div", { class: "bench-chart__legend" }, footer);
    this.hint = el("div", { class: "bench-chart__hint" }, footer);
    this.tooltip = el("div", { class: "bench-chart__tooltip", "aria-hidden": "true" }, container);
  }

  Chart.prototype.buildControls = function () {
    if (this.kind === "latency") {
      this.viewControl = segmented(this.controls, "view", [
        ["ranking", "Ranking", "One mark per library at the chosen percentile", "ranking"],
        ["distribution", "Distribution", "Every percentile from the 50th to the 99.9th", "distribution"],
      ], (view) => setState({ view }));
      this.percentileControl = segmented(this.controls, "percentile",
        PERCENTILES.map((p) => [p, p, `${p} percentile`]), (percentile) => setState({ percentile }));
      this.scaleControl = segmented(this.controls, "scale", [
        ["auto", "Auto", "Log scale when the libraries differ by more than 30 times, linear otherwise"],
        ["linear", "Linear", "Linear scale from zero"],
        ["log", "Log", "Logarithmic scale, to compare libraries that differ by orders of magnitude"],
      ], (scale) => setState({ scale }));
    } else {
      this.binaryControl = segmented(this.controls, "binary", [
        [false, "Text logs", "Human-readable output only"],
        [true, "Include binary logs", "Add binary-output modes, which are not directly comparable"],
      ], (binary) => setState({ binary }));
    }
  };

  Chart.prototype.syncControls = function () {
    if (this.kind === "latency") {
      this.viewControl.sync(state.view, "View");
      this.percentileControl.sync(state.percentile, state.view === "ranking" ? "Percentile" : "Sort by");
      this.scaleControl.sync(state.scale, "Scale");
    } else {
      this.binaryControl.sync(state.binary, "Show");
    }
  };

  Chart.prototype.measure = function (text, cls) {
    const key = `${cls}\n${text}`;
    let width = this.textCache.get(key);
    if (width === undefined) {
      this.ruler.setAttribute("class", cls);
      this.ruler.textContent = text;
      width = this.ruler.getComputedTextLength();
      this.textCache.set(key, width);
    }
    return width;
  };

  // The longest start of a text that fits a width, with an ellipsis when it had to be cut
  Chart.prototype.fit = function (text, cls, max) {
    if (this.measure(text, cls) <= max) return text;
    let lo = 0;
    let hi = text.length;
    while (lo < hi) {
      const mid = Math.ceil((lo + hi) / 2);
      if (this.measure(`${text.slice(0, mid).trimEnd()}…`, cls) <= max) lo = mid;
      else hi = mid - 1;
    }
    return `${text.slice(0, lo).trimEnd()}…`;
  };

  // Called when a setting changed; a chart in a hidden tab is redrawn when its tab is shown
  Chart.prototype.update = function (pinOnly) {
    if (!this.visible) {
      this.stale = true;
      this.markTable();
      return;
    }
    if (pinOnly && this.layoutCache) this.syncPin();
    else this.render(true);
    this.markTable();
  };

  Chart.prototype.markTable = function () {
    this.data.rows.forEach((row) => row.tr.classList.toggle("bench-row--pinned", isPinned(row.name)));
  };

  /* ---------- layout: every number the drawing needs, for the current settings and width ---------- */

  Chart.prototype.layout = function () {
    const width = this.width;
    const narrow = width < NARROW_WIDTH;
    const latency = this.kind === "latency";
    const data = this.data;
    const px = (v) => Math.round(v * unit);
    const p = state.percentile;
    const pi = PERCENTILES.indexOf(p);
    const distribution = latency && state.view === "distribution";

    const m = {
      narrow,
      row: px(narrow ? 46 : 32),
      labelY: px(narrow ? 14 : 16),
      markY: px(narrow ? 33 : 16),
      dot: 6.5 * unit,
      stem: px(4),
      bar: px(14),
      band: px(12),
    };

    let rows;
    let floor = null;
    let valueOf;
    if (latency) {
      // The timer-only row is the measurement floor: drawn as a reference line, not as a library
      floor = data.rows.find((r) => r.floor) || null;
      // Ties are broken by the higher percentiles, as in the tables, then by name
      const later = PERCENTILES.slice(pi + 1);
      rows = data.rows.filter((r) => !r.floor)
        .sort((a, b) => a.values[p] - b.values[p] || later.reduce((d, q) => d || a.values[q] - b.values[q], 0) ||
          a.name.localeCompare(b.name));
      valueOf = (r) => r.values[p];
    } else {
      rows = data.rows.filter((r) => state.binary || !r.binary)
        .sort((a, b) => b.throughput - a.throughput || a.name.localeCompare(b.name));
      valueOf = (r) => r.throughput;
    }
    const textOf = (r) => formatNumber(valueOf(r), data.decimals);

    const valueWidth = Math.ceil(Math.max(0, ...rows.map((r) => this.measure(textOf(r), "bench-chart__value"))));
    const fullLabel = Math.ceil(Math.max(0, ...rows.map((r) => this.measure(r.name, "bench-chart__label is-strong"))));
    let labelWidth;
    let plotLeft;
    let plotWidth;
    if (narrow) {
      labelWidth = Math.max(px(60), width - valueWidth - px(12));
      plotLeft = px(7);
      plotWidth = Math.max(px(80), width - 2 * px(7));
    } else {
      labelWidth = Math.min(fullLabel, Math.max(px(120), Math.round(width * 0.36)));
      plotLeft = labelWidth + px(14);
      plotWidth = Math.max(px(80), width - plotLeft - Math.ceil(m.dot + px(8) + valueWidth + 2));
    }

    // The scale covers what the view shows: one percentile, or all of them
    const shown = rows.flatMap((r) => (latency ? (distribution ? PERCENTILES.map((q) => r.values[q]) : [r.values[p]]) : [r.throughput]));
    if (floor) shown.push(floor.values[p], ...(distribution ? [floor.values["50th"]] : []));
    const positive = shown.filter((v) => v > 0);
    const lo = positive.length ? Math.min(...positive) : 1;
    const hi = shown.length ? Math.max(...shown, lo) : 1;
    const wantLog = latency && positive.length > 0 && (state.scale === "log" || (state.scale === "auto" && hi / lo > AUTO_LOG_RATIO));
    const scale = wantLog ? logScale(lo, hi, plotWidth, !narrow) : linearScale(hi, plotWidth);
    const x = (v) => snap(plotLeft + scale.map(v));

    const head = latency ? px(24) : narrow ? px(22) : px(8);
    const bottom = head + rows.length * m.row;
    const axisHeight = px(narrow ? 46 : 32);
    const columnValues = narrow || distribution;

    const geometry = new Map();
    const details = new Map();
    rows.forEach((row, index) => {
      const text = textOf(row);
      const textWidth = this.measure(text, "bench-chart__value");
      const xv = x(valueOf(row));
      // A latency stem starts at the timer floor, a throughput bar at zero
      const g = { y: head + index * m.row, a: 1, x0: latency && floor ? x(floor.values[p]) : plotLeft, xv, d: distribution ? 1 : 0, b: latency ? 0 : 1 };
      g.lx = columnValues ? width - textWidth : xv + (latency ? m.dot : 0) + px(8);
      PERCENTILES.forEach((q, i) => (g[`p${i}`] = latency ? x(row.values[q]) : xv));
      geometry.set(row.name, g);
      details.set(row.name, { row, text, label: this.fit(row.name, "bench-chart__label is-strong", labelWidth) });
    });

    const floorGeometry = floor
      ? { x: x(floor.values[p]), a: 1, lo: x(floor.values["50th"]), hi: x(floor.values["99.9th"]), d: distribution ? 1 : 0 }
      : { x: plotLeft, a: 0, lo: plotLeft, hi: plotLeft, d: 0 };

    return {
      m, width, narrow, latency, distribution, p, rows, floor, scale, plotLeft, plotWidth, head, bottom,
      height: bottom + axisHeight, labelWidth, valueWidth, columnValues, details,
      frame: { rows: geometry, floor: floorGeometry },
    };
  };

  /* ---------- drawing ---------- */

  Chart.prototype.render = function (animate, from, fromHeight) {
    if (!this.visible || !this.width) return;
    this.stale = false;
    this.hideTooltip();
    const container = this.container;
    if (this.unit !== unit) {
      // The widths that were measured belong to the old text size
      this.textCache.clear();
      this.unit = unit;
    }
    container.style.setProperty("--bench-u", String(unit));
    container.classList.toggle("is-narrow", this.width < NARROW_WIDTH);
    container.classList.toggle("is-compact", this.width < 380);
    container.classList.toggle("is-distribution", this.kind === "latency" && state.view === "distribution");

    const L = this.layoutCache = this.layout();
    L.spans = this.spans(L);
    const start = from || this.current;
    const moving = Boolean(animate && start && motionAllowed());

    this.syncControls();
    this.subtitle.textContent = L.latency
      ? (L.distribution
        ? `Latency from the 50th to the 99.9th percentile, in nanoseconds, sorted by the ${L.p}`
        : `${L.p} percentile latency, in nanoseconds`)
      : "Million messages per second written to the log file";
    this.svg.setAttribute("aria-label",
      `${this.title}. ${L.rows.length} libraries in the order of the table, ${L.latency ? `by the ${L.p} percentile` : "by throughput"}. ` +
      (L.floor ? `Timer only, with no logging: ${formatNumber(L.floor.values[L.p], this.data.decimals)} nanoseconds. ` : "") +
      "Use the arrow keys to move between libraries and Enter to highlight one in every chart. The table below lists every value.");

    if (moving && fromHeight) {
      // Taking over from the chart of another tab: start at its height, so the page does not jump
      this.plot.classList.remove("is-moving");
      this.svg.style.height = `${fromHeight}px`;
      void this.svg.getBoundingClientRect();
    }
    this.plot.classList.toggle("is-moving", moving);
    this.svg.style.height = `${L.height}px`;
    this.svg.setAttribute("height", L.height);

    this.drawAxis(L, moving);
    this.drawNotes(L);
    this.order = L.rows.map((row) => row.name);
    L.rows.forEach((row) => this.rowView(row).place(L));
    // Rows are kept in reading order for assistive technology, without stealing the focus
    const focused = document.activeElement;
    this.order.forEach((name, index) => {
      const g = this.views.get(name).g;
      if (this.rowLayer.children[index] !== g) this.rowLayer.insertBefore(g, this.rowLayer.children[index] || null);
    });
    if (focused && focused !== document.activeElement && this.rowLayer.contains(focused)) focused.focus({ preventScroll: true });
    this.syncPin();
    this.play(L.frame, start, moving);
  };

  // Where the vertical lines of the plot run: the whole height beside the names, or, on a narrow
  // chart, only the band of each row that holds its mark, so no line crosses a name
  Chart.prototype.spans = function (L) {
    const px = (v) => Math.round(v * unit);
    if (!L.narrow) return [[L.head - px(2), L.bottom + px(4)]];
    return L.rows.map((row, i) => [L.head + i * L.m.row + L.m.markY - px(10), L.head + i * L.m.row + L.m.markY + px(10)]);
  };

  Chart.prototype.drawAxis = function (L, moving) {
    const old = this.axisGroup;
    if (old) {
      if (moving) {
        old.setAttribute("class", "bench-chart__axis is-leaving");
        window.setTimeout(() => old.remove(), 260);
      } else {
        old.remove();
      }
    }
    const g = this.axisGroup = el("svg:g", { class: `bench-chart__axis${moving ? " is-entering" : ""}` }, this.axisLayer);
    const { scale, plotLeft, bottom, m, width } = L;
    const px = (v) => Math.round(v * unit);
    const spans = L.spans;

    const lines = {};
    scale.ticks.forEach((tick) => {
      const gx = Math.round(plotLeft + tick.x) + 0.5;
      const cls = scale.kind === "linear" && tick.value === 0 ? "is-zero" : `is-level-${tick.level}`;
      lines[cls] = (lines[cls] || "") + spans.map(([y1, y2]) => `M${gx},${y1}V${y2}`).join("");
    });
    Object.keys(lines).sort().reverse().forEach((cls) => el("svg:path", { class: `bench-chart__gridline ${cls}`, d: lines[cls] }, g));

    // Labels never collide: decades win over the ends of the axis, which win over the 2s and 5s
    const labels = scale.ticks.filter((tick) => tick.label).map((tick) => {
      const text = formatTick(tick.value);
      const w = this.measure(text, "bench-chart__tick");
      const cx = clamp(plotLeft + tick.x, w / 2 - px(6), width + px(6) - w / 2);
      return { tick, text, w, cx };
    }).sort((a, b) => b.tick.priority - a.tick.priority);
    const kept = [];
    labels.forEach((label) => {
      if (kept.every((other) => Math.abs(other.cx - label.cx) >= (other.w + label.w) / 2 + px(14))) kept.push(label);
    });
    kept.forEach((label) => {
      el("svg:text", {
        class: `bench-chart__tick${label.tick.level === 0 ? " is-major" : ""}`, x: snap(label.cx), y: bottom + px(19),
        "text-anchor": "middle", text: label.text,
      }, g);
    });

    const unitNames = L.latency
      ? (scale.kind === "log" ? ["nanoseconds · log scale", "ns · log scale", "ns · log"] : ["nanoseconds", "ns"])
      : ["million messages / second", "million msg/s", "M msg/s"];
    if (m.narrow) {
      el("svg:text", { class: "bench-chart__axis-title", x: plotLeft, y: bottom + px(37), text: unitNames[0] }, g);
    } else {
      // The unit sits under the names, clear of the first tick label
      const end = Math.min(plotLeft - px(14), ...kept.map((label) => label.cx - label.w / 2 - px(10)));
      const name = unitNames.find((text) => this.measure(text, "bench-chart__axis-title") <= end) || unitNames[unitNames.length - 1];
      el("svg:text", { class: "bench-chart__axis-title", x: snap(end), y: bottom + px(19), "text-anchor": "end", text: name }, g);
    }
  };

  // The labels that are not part of a row: the timer floor and the heading of the value column
  Chart.prototype.drawNotes = function (L) {
    const px = (v) => Math.round(v * unit);
    if (L.floor) {
      const value = `${formatNumber(L.floor.values[L.p], this.data.decimals)} ns`;
      this.floorValue.textContent = value;
      this.floorTitle.textContent = `Timer only (no logging): ${value} at the ${L.p} percentile, the cost of timing a sample with a call that logs nothing`;
      this.floorLabelWidth = this.measure("Timer only", "bench-chart__floor-label bench-chart__floor-name") + this.measure(value, "bench-chart__floor-label bench-chart__floor-value") + px(8);
    }
    this.floorLayer.style.display = L.floor ? "" : "none";
    this.floorLabel.style.display = L.floor ? "" : "none";
    const caption = !L.columnValues ? "" : L.latency ? `${L.p} · ns` : "M msg/s";
    this.caption.textContent = caption;
    this.captionWidth = caption ? this.measure(caption, "bench-chart__caption") : 0;
    this.caption.setAttribute("x", L.width);
    this.caption.setAttribute("y", L.head - px(9));
  };

  function barPath(x0, x1, mid, thickness) {
    // Square at the baseline, rounded at the data end
    const w = Math.max(x1 - x0, 0);
    const r = Math.min(4 * unit, w, thickness / 2);
    const top = mid - thickness / 2;
    const bottom = mid + thickness / 2;
    return `M${x0},${top}H${x0 + w - r}Q${x0 + w},${top} ${x0 + w},${top + r}V${bottom - r}` +
      `Q${x0 + w},${bottom} ${x0 + w - r},${bottom}H${x0}Z`;
  }

  Chart.prototype.rowView = function (row) {
    let view = this.views.get(row.name);
    if (view) return view;
    const chart = this;
    const latency = this.kind === "latency";
    const g = el("svg:g", { class: "bench-chart__row", role: "button", tabindex: -1, "data-library": row.name }, this.rowLayer);
    if (row.quill) g.classList.add("is-quill");
    if (row.binary) g.classList.add("is-binary");
    if (row.reference) g.classList.add("is-reference");
    const hit = el("svg:rect", { class: "bench-chart__hit" }, g);
    const body = el("svg:g", { class: "bench-chart__body" }, g);
    const label = el("svg:text", { class: "bench-chart__label", dy: "0.35em" }, body);
    const marks = el("svg:g", { class: "bench-chart__marks" }, body);
    const dist = el("svg:g", { class: "bench-chart__dist" }, marks);
    // The band from the 50th to the 99th percentile is one shape with rounded ends, cut into steps
    const clipId = `${this.uid}-band-${++this.clipCount}`;
    const clip = latency ? el("svg:rect", {}, el("svg:clipPath", { id: clipId }, dist)) : null;
    const band = el("svg:g", { "clip-path": latency ? `url(#${clipId})` : null }, dist);
    const segments = latency ? [1, 2, 3, 4].map((i) => el("svg:rect", { class: `bench-chart__seg bench-chart__seg--${i}` }, band)) : [];
    const whisker = latency ? el("svg:line", { class: "bench-chart__whisker" }, dist) : null;
    const cap = latency ? el("svg:line", { class: "bench-chart__cap" }, dist) : null;
    const bar = el("svg:path", { class: latency ? "bench-chart__stem" : "bench-chart__bar" }, marks);
    const dot = latency ? el("svg:circle", { class: "bench-chart__dot" }, marks) : null;
    const value = el("svg:text", { class: "bench-chart__value", dy: "0.35em" }, body);

    view = {
      row, g, hit, body,
      // Everything that does not move during a transition
      place(L) {
        const m = L.m;
        const px = (v) => Math.round(v * unit);
        const detail = L.details.get(row.name);
        view.detail = detail;
        view.m = m;
        hit.setAttribute("x", -px(8));
        hit.setAttribute("y", 1);
        hit.setAttribute("width", L.width + 2 * px(8));
        hit.setAttribute("height", m.row - 2);
        hit.setAttribute("rx", px(7));
        // Beside the plot a name ends where its mark begins; above it, the name starts the row
        label.setAttribute("x", L.narrow ? 0 : L.labelWidth);
        label.setAttribute("text-anchor", L.narrow ? "start" : "end");
        label.setAttribute("y", m.labelY);
        label.textContent = "";
        // "(binary log)" stays in the name, in a lighter voice
        const suffix = /^(.*?)(\s*\(binary log\))$/i.exec(detail.label);
        if (suffix) {
          el("svg:tspan", { text: suffix[1] }, label);
          el("svg:tspan", { class: "bench-chart__label-note", text: suffix[2] }, label);
        } else {
          label.textContent = detail.label;
        }
        value.textContent = detail.text;
        value.setAttribute("y", L.columnValues ? m.labelY : m.markY);
        g.setAttribute("aria-label", chart.describe(detail, L));
      },
      // Everything that does
      draw(f) {
        const m = view.m;
        g.setAttribute("transform", `translate(0,${f.y})`);
        body.setAttribute("opacity", f.a);
        const thickness = lerp(m.stem, m.bar, f.b);
        bar.setAttribute("d", barPath(Math.min(f.x0, f.xv), Math.max(f.x0, f.xv), m.markY, thickness));
        value.setAttribute("x", f.lx);
        if (!latency) return;
        bar.setAttribute("opacity", 1 - f.d);
        dot.setAttribute("cx", f.xv);
        dot.setAttribute("cy", m.markY);
        dot.setAttribute("r", m.dot);
        // The bands open out from the dot, which stays where the chosen percentile is
        const q = [f.p0, f.p1, f.p2, f.p3, f.p4, f.p5].map((v) => f.xv + (v - f.xv) * f.d);
        dist.setAttribute("opacity", f.d);
        clip.setAttribute("x", q[0]);
        clip.setAttribute("y", m.markY - m.band / 2);
        clip.setAttribute("width", Math.max(q[4] - q[0], 0));
        clip.setAttribute("height", m.band);
        clip.setAttribute("rx", Math.min(3 * unit, Math.max(q[4] - q[0], 0) / 2));
        segments.forEach((seg, i) => {
          seg.setAttribute("x", q[i]);
          seg.setAttribute("y", m.markY - m.band / 2);
          seg.setAttribute("width", Math.max(q[i + 1] - q[i], 0));
          seg.setAttribute("height", m.band);
        });
        whisker.setAttribute("x1", q[4]);
        whisker.setAttribute("x2", q[5]);
        whisker.setAttribute("y1", m.markY);
        whisker.setAttribute("y2", m.markY);
        cap.setAttribute("x1", q[5]);
        cap.setAttribute("x2", q[5]);
        cap.setAttribute("y1", m.markY - m.band / 2);
        cap.setAttribute("y2", m.markY + m.band / 2);
      },
    };

    let touch = false;
    g.addEventListener("pointerdown", (event) => (touch = event.pointerType === "touch"));
    // The row under a mouse or a pen is marked by hand: a finger would leave a :hover behind
    g.addEventListener("pointerenter", (event) => {
      if (event.pointerType === "touch") return;
      g.classList.add("is-hover");
      chart.showTooltip(view, event.clientX);
    });
    g.addEventListener("pointermove", (event) => { if (event.pointerType !== "touch") chart.showTooltip(view, event.clientX); });
    g.addEventListener("pointerleave", (event) => {
      g.classList.remove("is-hover");
      if (event.pointerType !== "touch") chart.hideTooltip(80);
    });
    g.addEventListener("focus", () => {
      chart.active = row.name;
      chart.syncTabStops();
      let keyboard = true;
      try { keyboard = g.matches(":focus-visible"); } catch (e) { /* older browsers: always show */ }
      if (keyboard) chart.showTooltip(view, null);
    });
    g.addEventListener("blur", () => chart.hideTooltip());
    g.addEventListener("click", () => {
      const pin = isPinned(row.name) ? null : row.name;
      setState({ pinned: pin });
      // A finger has no hover: the numbers of the highlighted library stay on screen instead
      if (touch) {
        if (pin) chart.showTooltip(view, null);
        else chart.hideTooltip();
      }
    });
    g.addEventListener("keydown", (event) => {
      const index = chart.order.indexOf(row.name);
      let next = -1;
      if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        setState({ pinned: isPinned(row.name) ? null : row.name });
        chart.showTooltip(view, null);
      } else if (event.key === "Escape") {
        if (state.pinned) setState({ pinned: null });
        chart.hideTooltip();
      } else if (event.key === "ArrowDown") next = Math.min(index + 1, chart.order.length - 1);
      else if (event.key === "ArrowUp") next = Math.max(index - 1, 0);
      else if (event.key === "Home") next = 0;
      else if (event.key === "End") next = chart.order.length - 1;
      if (next >= 0) {
        event.preventDefault();
        chart.views.get(chart.order[next]).g.focus();
      }
    });

    this.views.set(row.name, view);
    return view;
  };

  // What a screen reader says for a row: its value and, for latency, every percentile
  Chart.prototype.describe = function (detail, L) {
    const row = detail.row;
    if (L.latency) {
      const all = PERCENTILES.map((q) => `${q} ${formatNumber(row.values[q], this.data.decimals)}`).join(", ");
      return `${row.name}. ${detail.text} nanoseconds at the ${L.p} percentile. All percentiles: ${all}.`;
    }
    return `${row.name}. ${detail.text} million messages per second, elapsed ${row.elapsed}, producers done ${row.producers}.`;
  };

  // One tab stop per chart: the arrow keys move between its rows
  Chart.prototype.syncTabStops = function () {
    const active = this.order.includes(this.active) ? this.active : this.order[0];
    this.views.forEach((view, name) => view.g.setAttribute("tabindex", name === active ? 0 : -1));
  };

  Chart.prototype.syncPin = function () {
    const L = this.layoutCache;
    const present = this.order.some(isPinned);
    this.views.forEach((view, name) => {
      const pinned = isPinned(name);
      view.g.classList.toggle("is-pinned", pinned);
      view.g.classList.toggle("is-dimmed", present && !pinned);
      view.g.setAttribute("aria-pressed", String(pinned));
      if (view.row.binary) {
        const series = pinned ? "pinned" : view.row.quill ? "quill" : "other";
        view.body.querySelector(".bench-chart__bar").style.fill = `url(#${this.uid}-hatch-${series})`;
      }
    });
    this.container.classList.toggle("has-pinned", present);
    this.syncTabStops();
    this.drawLegend(L, present);
    // An open tooltip takes the colour of its library's new state
    if (this.tipView && this.tipView.detail && this.tooltip.classList.contains("is-visible")) this.fillTooltip(this.tipView, L);
    else this.tipStamp = null;
  };

  Chart.prototype.drawLegend = function (L, present) {
    const legend = this.legend;
    legend.textContent = "";
    const item = (cls, text) => {
      const node = el("span", { class: "bench-chart__legend-item" }, legend);
      el("span", { class: `bench-chart__key ${cls}` }, node);
      el("span", { text }, node);
      return node;
    };
    const shape = L.latency ? "dot" : "bar";
    if (L.rows.some((r) => r.quill)) item(`bench-chart__key--${shape} bench-chart__key--quill`, "Quill");
    item(`bench-chart__key--${shape} bench-chart__key--other`, L.rows.some((r) => r.quill) ? "Other libraries" : "Libraries");
    if (L.distribution) {
      // The key of the distribution view is the mark itself, with a percentile under every step
      const node = el("span", { class: "bench-chart__legend-item bench-chart__legend-item--strip" }, legend);
      const block = el("span", { class: "bench-chart__strip-block" }, node);
      const strip = el("span", { class: "bench-chart__strip" }, block);
      [1, 2, 3, 4].forEach((i) => el("i", { class: `bench-chart__strip-seg bench-chart__strip-seg--${i}` }, strip));
      el("i", { class: "bench-chart__strip-line" }, strip);
      const names = el("span", { class: "bench-chart__strip-names" }, block);
      PERCENTILES.forEach((q, i) => el("b", { class: q === L.p ? "is-current" : null, style: `--i:${i}`, text: q.replace("th", "") }, names));
      el("span", { text: "percentile" }, node);
    }
    if (L.floor) item("bench-chart__key--floor", "Timer only, the measurement floor");
    if (!L.latency && state.binary && L.rows.some((r) => r.binary)) item("bench-chart__key--binary", "Binary log, not directly comparable");

    const hint = this.hint;
    hint.textContent = "";
    if (state.pinned) {
      const chip = el("button", {
        type: "button", class: `bench-chart__pin${present ? "" : " is-absent"}`,
        title: "Clear the highlight", "aria-label": `Clear the highlight of ${pinKey(state.pinned)}`,
      }, hint);
      el("span", { class: "bench-chart__pin-dot" }, chip);
      el("span", { class: "bench-chart__pin-name", text: present ? pinKey(state.pinned) : `${pinKey(state.pinned)} is not in this chart` }, chip);
      icon(ICONS.close, chip);
      chip.addEventListener("click", () => setState({ pinned: null }));
    } else {
      el("span", { text: `${coarsePointer.matches ? "Tap" : "Click"} a library to highlight it in every chart` }, hint);
    }
  };

  /* ---------- motion ---------- */

  Chart.prototype.draw = function (frame) {
    frame.rows.forEach((f, name) => {
      const view = this.views.get(name);
      if (view) view.draw(f);
    });
    this.drawFloor(frame.floor);
    this.current = frame;
  };

  Chart.prototype.drawFloor = function (f) {
    const L = this.layoutCache;
    const px = (v) => Math.round(v * unit);
    const spans = L.spans;
    const lx = Math.round(f.x) + 0.5;
    this.floorLayer.setAttribute("opacity", f.a);
    this.floorLabel.setAttribute("opacity", f.a);
    // Beside the names the line reaches up to the label that names it; on a narrow chart it stays
    // within the bands of the marks, like the grid, so it never crosses a name
    this.floorLine.setAttribute("d", spans.map(([y1, y2], i) => `M${lx},${i === 0 && !L.narrow ? L.head - px(6) : y1}V${y2}`).join(""));
    // In the distribution view the floor is a range too: the timer's own 50th to 99.9th percentile
    const w = Math.max(f.hi - f.lo, 0);
    this.floorBand.setAttribute("d", spans.map(([y1, y2]) => `M${f.lo},${y1}h${w}V${y2}h${-w}Z`).join(""));
    this.floorBand.setAttribute("opacity", f.d);
    // The label starts at its line, and steps back where it would meet the heading of the values
    const limit = L.width - (this.captionWidth ? this.captionWidth + px(10) : 0);
    const end = lx + px(6) + (this.floorLabelWidth || 0) > limit;
    this.floorLabel.setAttribute("x", end ? Math.max(Math.min(lx - px(6), limit), this.floorLabelWidth || 0) : lx + px(6));
    this.floorLabel.setAttribute("y", L.head - px(9));
    this.floorLabel.setAttribute("text-anchor", end ? "end" : "start");
  };

  // Moves the chart from where it is (or from the chart it replaces in its tab) to its new picture.
  // With a stagger the rows follow one another, which is how a chart arrives on screen.
  Chart.prototype.play = function (target, start, moving, stagger) {
    if (this.raf) window.cancelAnimationFrame(this.raf);
    this.raf = 0;
    const leaving = [];
    this.views.forEach((view, name) => { if (!target.rows.has(name)) leaving.push(name); });
    const finish = () => {
      this.settle = null;
      leaving.forEach((name) => {
        const view = this.views.get(name);
        if (view && !target.rows.has(name)) {
          view.g.remove();
          this.views.delete(name);
        }
      });
      this.plot.classList.remove("is-moving");
      this.draw(target);
    };
    if (!moving) {
      finish();
      return;
    }
    // A chart that is hidden in the middle of a move goes straight to its end
    this.settle = () => {
      window.cancelAnimationFrame(this.raf);
      this.raf = 0;
      finish();
    };
    const from = new Map();
    const delay = new Map();
    target.rows.forEach((g, name) => {
      const before = start.rows.get(name);
      // A library that was not on screen grows in from where its mark starts
      const entering = Object.assign({}, g, { a: 0, xv: g.x0, lx: g.x0 + (g.lx - g.xv) });
      PERCENTILES.forEach((q, i) => (entering[`p${i}`] = g.x0));
      from.set(name, before ? Object.assign({}, entering, before, { b: g.b }) : entering);
      delay.set(name, (stagger || 0) * delay.size);
    });
    const gone = new Map();
    leaving.forEach((name) => { if (start.rows.has(name)) gone.set(name, start.rows.get(name)); });
    const ease = stagger ? easeOut : easeInOut;
    const total = DURATION + (stagger || 0) * Math.max(target.rows.size - 1, 0);

    const began = window.performance.now();
    const step = (now) => {
      const elapsed = Math.max(0, now - began);
      const k = ease(clamp(elapsed / DURATION, 0, 1));
      const frame = { rows: new Map(), floor: mixNumbers(start.floor, target.floor, k) };
      target.rows.forEach((g, name) => {
        frame.rows.set(name, mixNumbers(from.get(name), g, ease(clamp((elapsed - delay.get(name)) / DURATION, 0, 1))));
      });
      gone.forEach((g, name) => frame.rows.set(name, Object.assign({}, g, { a: g.a * Math.max(0, 1 - k * 2.5) })));
      this.draw(frame);
      if (elapsed < total) {
        this.raf = window.requestAnimationFrame(step);
      } else {
        this.raf = 0;
        finish();
      }
    };
    step(began);
    this.raf = window.requestAnimationFrame(step);
  };

  // The first time a chart scrolls into view, its libraries arrive one after another
  Chart.prototype.reveal = function () {
    const L = this.layoutCache;
    if (!this.visible || !L || this.raf || !motionAllowed()) return;
    this.hideTooltip();
    this.play(L.frame, { rows: new Map(), floor: Object.assign({}, L.frame.floor, { a: 0 }) }, true, 24);
  };

  /* ---------- tooltip ---------- */

  Chart.prototype.showTooltip = function (view, clientX) {
    const L = this.layoutCache;
    if (!L || !view.detail) return;
    window.clearTimeout(this.tipTimer);
    const tip = this.tooltip;
    if (this.tipView !== view || this.tipStamp !== L) {
      this.tipView = view;
      this.tipStamp = L;
      this.fillTooltip(view, L);
    }
    const first = !tip.classList.contains("is-visible");
    const box = this.container.getBoundingClientRect();
    const rowBox = view.hit.getBoundingClientRect();
    const w = tip.offsetWidth;
    const h = tip.offsetHeight;
    let left;
    if (clientX === null || clientX === undefined) {
      // No pointer to follow (keyboard, touch): the numbers sit under the mark they belong to
      const f = this.current && this.current.rows.get(view.row.name);
      left = this.svg.getBoundingClientRect().left - box.left + (f ? f.xv : 0) + 14;
      if (left + w > box.width - 8) left = left - w - 28;
    } else {
      left = clientX - box.left + 16;
      if (left + w > box.width - 8) left = clientX - box.left - w - 16;
    }
    left = clamp(left, 8, Math.max(8, box.width - w - 8));
    // The theme's header stays at the top of the window: a tooltip above a row must clear it
    if (stickyHeader === undefined) stickyHeader = document.querySelector(".md-header");
    const roof = stickyHeader ? Math.max(0, stickyHeader.getBoundingClientRect().bottom) : 0;
    const fitsBelow = rowBox.bottom + 8 + h <= window.innerHeight - 8;
    const fitsAbove = rowBox.top - 8 - h >= roof + 4;
    const top = fitsBelow || !fitsAbove ? rowBox.bottom - box.top + 6 : rowBox.top - box.top - h - 6;
    if (first) tip.classList.add("is-jumping");
    tip.style.transform = `translate(${Math.round(left)}px, ${Math.round(top)}px)`;
    if (first) {
      void tip.offsetWidth;
      tip.classList.remove("is-jumping");
      tip.classList.add("is-visible");
    }
  };

  Chart.prototype.fillTooltip = function (view, L) {
    const tip = this.tooltip;
    const row = view.row;
    const decimals = this.data.decimals;
    tip.textContent = "";
    tip.setAttribute("class", `bench-chart__tooltip${tip.classList.contains("is-visible") ? " is-visible" : ""}` +
      `${row.quill ? " is-quill" : ""}${row.binary ? " is-binary" : ""}${isPinned(row.name) ? " is-pinned" : ""}`);
    const head = el("div", { class: "bench-chart__tooltip-head" }, tip);
    el("span", { class: "bench-chart__tooltip-swatch" }, head);
    el("span", { class: "bench-chart__tooltip-title", text: row.name }, head);
    if (row.binary) el("div", { class: "bench-chart__tooltip-note", text: "Binary log, not directly comparable with text output" }, tip);
    const list = el("div", { class: "bench-chart__tooltip-list" }, tip);
    const add = (label, value, unitText, share, current) => {
      const line = el("div", { class: `bench-chart__tooltip-row${current ? " is-current" : ""}` }, list);
      el("span", { class: "bench-chart__tooltip-label", text: label }, line);
      const track = el("span", { class: "bench-chart__tooltip-track" }, line);
      if (share !== null) el("span", { class: "bench-chart__tooltip-fill", style: `width:${clamp(share * 100, 0, 100).toFixed(1)}%` }, track);
      const number = el("span", { class: "bench-chart__tooltip-value" }, line);
      el("span", { text: value }, number);
      if (unitText) el("span", { class: "bench-chart__tooltip-unit", text: ` ${unitText}` }, number);
    };

    if (L.latency) {
      // The small bars compare this library's own percentiles, on a linear scale from zero
      const top = Math.max(...PERCENTILES.map((q) => row.values[q]));
      PERCENTILES.forEach((q) => add(q, formatNumber(row.values[q], decimals), "ns", top > 0 ? row.values[q] / top : 0, q === L.p));
      if (L.floor) {
        el("div", {
          class: "bench-chart__tooltip-foot",
          text: `Timer only at the ${L.p}: ${formatNumber(L.floor.values[L.p], decimals)} ns`,
        }, tip);
      }
    } else {
      add("Throughput", formatNumber(row.throughput, decimals), "M msg/s", null, true);
      add("Elapsed time", row.elapsed, "", null, false);
      add("Producers done", row.producers, "", null, false);
      // The gap between the two times of the table: how long the backend still wrote
      const elapsed = parseFloat(row.elapsed.replace(/,/g, ""));
      const producers = parseFloat(row.producers.replace(/,/g, ""));
      const unitOf = (text) => text.replace(/[\d.,\s]/g, "");
      if (Number.isFinite(elapsed) && Number.isFinite(producers) && unitOf(row.elapsed) === unitOf(row.producers) && elapsed > producers) {
        el("div", {
          class: "bench-chart__tooltip-foot",
          text: `The backend wrote for ${formatNumber(elapsed - producers, Math.max(decimalsOf(row.elapsed), decimalsOf(row.producers)))} ` +
            `${unitOf(row.elapsed)} after the producers were done`,
        }, tip);
      }
    }
  };

  // With a delay the tooltip survives the step from one row to the next, and glides there
  Chart.prototype.hideTooltip = function (delay) {
    window.clearTimeout(this.tipTimer);
    if (delay) this.tipTimer = window.setTimeout(() => this.tooltip.classList.remove("is-visible"), delay);
    else this.tooltip.classList.remove("is-visible");
  };

  /* ---------- startup ---------- */

  function refreshUnit() {
    // The theme enlarges its text on wide screens; the charts follow it
    const root = parseFloat(window.getComputedStyle(document.documentElement).fontSize) || 20;
    const next = clamp(root / 20, 1, 1.3);
    if (Math.abs(next - unit) < 0.001) return false;
    unit = next;
    return true;
  }

  function init() {
    refreshUnit();
    // A chart is drawn from the first table after it, or with data-table="previous" from the last
    // table before it
    const nodes = Array.from(document.querySelectorAll("table, .bench-chart"));
    nodes.forEach((node, index) => {
      if (node.tagName === "TABLE") return;
      const step = node.dataset.table === "previous" ? -1 : 1;
      for (let i = index + step; i >= 0 && i < nodes.length; i += step) {
        if (nodes[i].tagName === "TABLE") {
          nodes[i].querySelectorAll("tbody tr").forEach((tr) => {
            if (tr.cells.length && isQuill(cellText(tr.cells[0]))) tr.classList.add("bench-row--quill");
            if (tr.cells.length && /^(timer|argument) only/i.test(cellText(tr.cells[0]))) tr.classList.add("bench-row--floor");
          });
          const chart = new Chart(node, nodes[i]);
          if (chart.data.rows.length) {
            charts.push(chart);
            chart.markTable();
          } else {
            node.textContent = "";
            node.classList.remove("is-ready");
          }
          break;
        }
        if (step === 1) break;
      }
    });
    if (!charts.length) return;
    // Marks the page for the tab styles, for browsers that cannot select it with :has()
    document.body.classList.add("bench-page");

    const byPlot = new Map(charts.map((chart) => [chart.plot, chart]));
    let lastHidden = [];
    const resized = (entries) => {
      const rescaled = refreshUnit();
      const now = window.performance.now();
      const shown = [];
      entries.forEach((entry) => {
        const chart = byPlot.get(entry.target);
        const width = Math.floor(entry.contentRect.width);
        if (width > 0) {
          shown.push({ chart, width });
        } else if (chart.visible) {
          chart.visible = false;
          chart.hideTooltip();
          // What it showed at this instant is where the chart that replaces it starts from
          lastHidden.push({ chart, at: now, frame: chart.current, height: chart.layoutCache ? chart.layoutCache.height : 0, narrow: chart.layoutCache ? chart.layoutCache.narrow : null });
          if (chart.settle) chart.settle();
        }
      });
      lastHidden = lastHidden.filter((item) => now - item.at < 250);
      shown.forEach(({ chart, width }) => {
        const appeared = !chart.visible;
        const changed = width !== chart.width || rescaled;
        chart.visible = true;
        chart.width = width;
        if (!appeared && !changed && !chart.stale) return;
        // A chart shown by a tab starts from the chart it replaces, so the libraries move to
        // their new places instead of being swapped for other marks
        const previous = appeared && chart.root
          ? lastHidden.find((item) => item.chart !== chart && item.chart.root === chart.root && item.chart.width === width && item.frame)
          : null;
        if (previous && previous.narrow === (width < NARROW_WIDTH)) chart.render(true, previous.frame, previous.height);
        else chart.render(false);
      });
    };
    if (typeof ResizeObserver === "function") {
      const observer = new ResizeObserver(resized);
      charts.forEach((chart) => observer.observe(chart.plot));
    } else {
      // Older browsers: measure when the window changes and when a tab is chosen
      const measureAll = () => resized(charts.map((chart) => ({ target: chart.plot, contentRect: { width: chart.plot.clientWidth } })));
      window.addEventListener("resize", measureAll);
      document.addEventListener("change", () => window.setTimeout(measureAll, 0));
      measureAll();
    }

    // The dark palette follows the surface the charts sit on, whatever sets it
    const readTheme = () => {
      const match = /rgba?\(\s*([\d.]+)[,\s]+([\d.]+)[,\s]+([\d.]+)/.exec(window.getComputedStyle(charts[0].container).backgroundColor);
      if (!match) return;
      const [r, g, b] = match.slice(1, 4).map((v) => {
        const c = parseFloat(v) / 255;
        return c <= 0.03928 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4);
      });
      const dark = 0.2126 * r + 0.7152 * g + 0.0722 * b < 0.18;
      charts.forEach((chart) => chart.container.setAttribute("data-bench-theme", dark ? "dark" : "light"));
    };
    readTheme();
    let themeTimer = 0;
    const themeChanged = () => {
      window.clearTimeout(themeTimer);
      themeTimer = window.setTimeout(readTheme, 60);
    };
    const themeWatch = new MutationObserver(themeChanged);
    [document.documentElement, document.body].forEach((node) => themeWatch.observe(node, {
      attributes: true, attributeFilter: ["class", "style", "data-md-color-scheme", "data-md-color-primary", "data-theme"],
    }));
    if (window.matchMedia) {
      const media = window.matchMedia("(prefers-color-scheme: dark)");
      if (media.addEventListener) media.addEventListener("change", themeChanged);
    }

    // A chart that scrolls into view under the reader's own hand arrives with a short entrance.
    // A chart is never left waiting for it: until then it is simply drawn, and a jump to an
    // anchor, a script or a screenshot gets the finished chart.
    let lastGesture = -Infinity;
    const gesture = () => { lastGesture = window.performance.now(); };
    window.addEventListener("wheel", gesture, { passive: true });
    window.addEventListener("touchmove", gesture, { passive: true });
    window.addEventListener("keydown", (event) => {
      if (["PageDown", "PageUp", "ArrowDown", "ArrowUp", "End", "Home", " "].includes(event.key)) gesture();
    });
    if ("IntersectionObserver" in window) {
      const watcher = new IntersectionObserver((entries) => {
        entries.forEach((entry) => {
          const chart = byPlot.get(entry.target);
          if (!entry.isIntersecting || chart.seen) return;
          chart.seen = true;
          if (window.performance.now() - lastGesture < 400) chart.reveal();
        });
      }, { rootMargin: "0px 0px 96px 0px" });
      charts.forEach((chart) => watcher.observe(chart.plot));
    }

    const redrawAll = () => charts.forEach((chart) => {
      chart.textCache.clear();
      if (chart.visible) chart.render(false);
      else chart.stale = true;
    });
    // Web fonts change the width of every label when they arrive
    if (document.fonts && document.fonts.ready) {
      document.fonts.ready.then(redrawAll);
      if (document.fonts.addEventListener) document.fonts.addEventListener("loadingdone", redrawAll);
    }
    // Another tab or window of the documentation changed the settings
    window.addEventListener("storage", (event) => {
      if (event.key !== STORAGE_KEY) return;
      setState(Object.assign({}, defaults, loadState()), true);
    });
    // On a touch screen a tap anywhere else puts the numbers away
    document.addEventListener("pointerdown", (event) => {
      charts.forEach((chart) => {
        if (chart.visible && !chart.rowLayer.contains(event.target)) chart.hideTooltip();
      });
    }, true);
    window.addEventListener("beforeprint", () => charts.forEach((chart) => { if (chart.visible) chart.render(false); }));
  }

  // Compile-time profiles load in Speedscope only when the reader asks for one
  function initProfiles() {
    document.querySelectorAll(".speedscope-frame[data-src]").forEach((frame) => {
      const button = frame.querySelector(".speedscope-load");
      if (!button) return;
      button.addEventListener("click", () => {
        const iframe = document.createElement("iframe");
        iframe.src = frame.dataset.src;
        iframe.title = frame.dataset.title || "Speedscope profile";
        frame.textContent = "";
        frame.classList.add("is-loaded");
        frame.appendChild(iframe);
      });
    });
  }

  function start() {
    init();
    initProfiles();
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", start);
  else start();
})();
