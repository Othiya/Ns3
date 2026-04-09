#!/usr/bin/env python3


from __future__ import annotations

import argparse
from pathlib import Path
from typing import Dict, List, Tuple


def load_dat(path: Path, variant: str) -> List[dict]:
    if not path.exists():
        return []

    rows: List[dict] = []
    with path.open("r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 6:
                continue
            rows.append(
                {
                    "time_s": float(parts[0]),
                    "bdp_kb": float(parts[1]),
                    "conv_s": float(parts[2]),
                    "event": parts[3],
                    "base_rtt_s": float(parts[4]),
                    "conv_base_rtts": float(parts[5]),
                    "variant": variant,
                }
            )
    return rows


def to_xy(points: List[Tuple[float, float]],
          x_min: float,
          x_max: float,
          y_min: float,
          y_max: float,
          left: float,
          top: float,
          plot_w: float,
          plot_h: float) -> List[Tuple[float, float]]:
    xy: List[Tuple[float, float]] = []
    for x, y in points:
        xp = left + (x - x_min) * plot_w / max(1e-9, (x_max - x_min))
        yp = top + plot_h - (y - y_min) * plot_h / max(1e-9, (y_max - y_min))
        xy.append((xp, yp))
    return xy


def plot_event(rows: List[dict], event_name: str, title: str, output_svg: Path) -> None:
    subset = [r for r in rows if r["event"] == event_name]
    if not subset:
        print(f"Skipping {event_name}: no rows found")
        return

    grouped: Dict[str, List[dict]] = {}
    for row in subset:
        grouped.setdefault(row["variant"], []).append(row)

    all_x = [r["bdp_kb"] for r in subset]
    all_y = [r["conv_s"] for r in subset]
    x_min, x_max = min(all_x), max(all_x)
    y_min, y_max = min(all_y), max(all_y)
    if x_min == x_max:
        x_min -= 1.0
        x_max += 1.0
    if y_min == y_max:
        y_min = 0.0
        y_max += 1.0

    width = 980
    height = 620
    left = 90
    right = 40
    top = 70
    bottom = 80
    plot_w = width - left - right
    plot_h = height - top - bottom
    colors = {"Vegas": "#1f77b4", "Quick Vegas": "#d62728"}

    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">',
        f'<rect x="0" y="0" width="{width}" height="{height}" fill="white"/>',
        f'<text x="{width/2}" y="36" text-anchor="middle" font-size="26" font-family="sans-serif">{title}</text>',
        f'<line x1="{left}" y1="{top + plot_h}" x2="{left + plot_w}" y2="{top + plot_h}" stroke="black"/>',
        f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top + plot_h}" stroke="black"/>',
    ]

    for i in range(6):
        frac = i / 5.0
        x = left + frac * plot_w
        y = top + plot_h - frac * plot_h
        xv = x_min + frac * (x_max - x_min)
        yv = y_min + frac * (y_max - y_min)
        lines.append(f'<line x1="{x}" y1="{top}" x2="{x}" y2="{top + plot_h}" stroke="#dddddd"/>')
        lines.append(f'<line x1="{left}" y1="{y}" x2="{left + plot_w}" y2="{y}" stroke="#dddddd"/>')
        lines.append(f'<text x="{x}" y="{top + plot_h + 24}" text-anchor="middle" font-size="14" font-family="sans-serif">{xv:.0f}</text>')
        lines.append(f'<text x="{left - 12}" y="{y + 4}" text-anchor="end" font-size="14" font-family="sans-serif">{yv:.1f}</text>')

    for variant, grp in grouped.items():
        grp_sorted = sorted(grp, key=lambda r: r["bdp_kb"])
        pts = [(r["bdp_kb"], r["conv_s"]) for r in grp_sorted]
        xy = to_xy(pts, x_min, x_max, y_min, y_max, left, top, plot_w, plot_h)
        poly = " ".join([f"{x:.2f},{y:.2f}" for x, y in xy])
        color = colors.get(variant, "#444444")
        lines.append(f'<polyline points="{poly}" fill="none" stroke="{color}" stroke-width="2.5"/>')
        for x, y in xy:
            lines.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="4.5" fill="{color}"/>')

    lines.append(f'<text x="{width/2}" y="{height - 18}" text-anchor="middle" font-size="18" font-family="sans-serif">BDP (Kb)</text>')
    lines.append(
        f'<text x="26" y="{height/2}" text-anchor="middle" font-size="18" font-family="sans-serif" transform="rotate(-90 26,{height/2})">Convergence Time (s)</text>'
    )

    legend_x = left + plot_w - 170
    legend_y = top + 20
    lines.append(f'<rect x="{legend_x}" y="{legend_y}" width="150" height="58" fill="#f8f8f8" stroke="#cccccc"/>')
    idx = 0
    for variant in ["Vegas", "Quick Vegas"]:
        if variant not in grouped:
            continue
        y = legend_y + 20 + idx * 22
        color = colors.get(variant, "#444444")
        lines.append(f'<line x1="{legend_x + 10}" y1="{y}" x2="{legend_x + 40}" y2="{y}" stroke="{color}" stroke-width="2.5"/>')
        lines.append(f'<circle cx="{legend_x + 25}" cy="{y}" r="4" fill="{color}"/>')
        lines.append(f'<text x="{legend_x + 48}" y="{y + 4}" font-size="14" font-family="sans-serif">{variant}</text>')
        idx += 1

    lines.append("</svg>")
    output_svg.write_text("\n".join(lines), encoding="utf-8")
    print(f"Saved: {output_svg}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Plot Vegas vs Quick Vegas convergence-vs-BDP")
    parser.add_argument("--results-dir", default="convergence", help="Directory containing .dat files")
    args = parser.parse_args()

    results_dir = Path(args.results_dir)
    vegas = load_dat(results_dir / "TcpVegas-convergence-vs-bdp.dat", "Vegas")
    quick = load_dat(results_dir / "TcpQuickVegas-convergence-vs-bdp.dat", "Quick Vegas")

    all_rows = vegas + quick
    if not all_rows:
        raise SystemExit("No input data found. Run simulations first.")

    plot_event(
        all_rows,
        "new_connection",
        "Convergence Time of New Connections",
        results_dir / "convergence-new-connection.svg",
    )
    plot_event(
        all_rows,
        "bandwidth_halved",
        "Convergence Time when Available Bandwidth is Halved",
        results_dir / "convergence-bandwidth-halved.svg",
    )
    plot_event(
        all_rows,
        "bandwidth_doubled",
        "Convergence Time when Available Bandwidth is Doubled",
        results_dir / "convergence-bandwidth-doubled.svg",
    )


if __name__ == "__main__":
    main()
