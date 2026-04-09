#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt

SWEEPS = [
    ("sweep1_nodes", "Nodes"),
    ("sweep2_flows", "Flows"),
    ("sweep3_packets_per_sec", "Packets/s"),
    ("sweep4_flows_delay_drop", "Flows"),
]

METRICS = [
    ("throughput-mbps", "Throughput (Mbps)", "throughput.png"),
    ("delay-ms", "End-to-End Delay (ms)", "delay.png"),
    ("pdr", "Packet Delivery Ratio", "pdr.png"),
    ("drop-ratio", "Packet Drop Ratio", "drop_ratio.png"),
]

SWEEP_TITLES = {
    "sweep1_nodes": "Sweep 1: Nodes",
    "sweep2_flows": "Sweep 2: Flows",
    "sweep3_packets_per_sec": "Sweep 3: Packets/s",
    "sweep4_flows_delay_drop": "Sweep 4: Flows (Delay/Drop Focus)",
}


def load_dat(path: Path):
    xs = []
    vegas = []
    quick = []
    with path.open("r", encoding="ascii") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            parts = line.split()
            if len(parts) == 3:
                x, y1, y2 = parts
                xs.append(float(x))
                vegas.append(float(y1))
                quick.append(float(y2))
            elif len(parts) == 2:
                x, y = parts
                xs.append(float(x))
                quick.append(float(y))
            else:
                raise ValueError(f"Unexpected column count in {path}: {line}")

    if not xs:
        raise ValueError(
            f"No data rows found in {path}. The file has headers only, so the simulation output was not written."
        )

    return xs, vegas, quick


def build_metric_figure(results_dir: Path, metric_key: str, y_label: str, output_name: str):
    fig, axes = plt.subplots(2, 2, figsize=(12, 8), constrained_layout=True)
    axes = axes.flatten()
    plotted_algorithms = set()

    for axis, (sweep_id, x_label) in zip(axes, SWEEPS):
        dat_path = results_dir / f"{sweep_id}-{metric_key}.dat"
        xs, vegas, quick = load_dat(dat_path)

        if vegas:
            axis.plot(xs, vegas, marker="o", linewidth=2, label="TcpVegas")
            plotted_algorithms.add("TcpVegas")
        if quick:
            axis.plot(xs, quick, marker="s", linewidth=2, label="QuickVegas")
            plotted_algorithms.add("QuickVegas")
        axis.set_title(SWEEP_TITLES[sweep_id])
        axis.set_xlabel(x_label)
        axis.set_ylabel(y_label)
        axis.grid(True, linestyle="--", alpha=0.4)
        if vegas or quick:
            axis.legend()

    if plotted_algorithms == {"TcpVegas", "QuickVegas"}:
        figure_title = f"TcpVegas vs QuickVegas (Congested Bottleneck): {y_label}"
    elif plotted_algorithms == {"QuickVegas"}:
        figure_title = f"QuickVegas (Congested Bottleneck): {y_label}"
    elif plotted_algorithms == {"TcpVegas"}:
        figure_title = f"TcpVegas (Congested Bottleneck): {y_label}"
    else:
        figure_title = y_label

    fig.suptitle(figure_title, fontsize=14)
    plot_dir = results_dir / "plots"
    plot_dir.mkdir(parents=True, exist_ok=True)
    fig.savefig(plot_dir / output_name, dpi=200)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description="Plot QuickVegas dumbbell sweep results")
    parser.add_argument(
        "--results-dir",
        default="results/quick-vegas-dumbbell",
        help="Directory containing the generated .dat files",
    )
    args = parser.parse_args()

    results_dir = Path(args.results_dir)
    if not results_dir.exists():
        raise SystemExit(f"Results directory not found: {results_dir}")

    for metric_key, y_label, output_name in METRICS:
        build_metric_figure(results_dir, metric_key, y_label, output_name)


if __name__ == "__main__":
    main()

