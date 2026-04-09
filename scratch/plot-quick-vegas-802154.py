#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

import matplotlib.pyplot as plt

QUICK_STYLE = dict(color="#1f77b4", linestyle="-", marker="o", linewidth=2)
VEGAS_STYLE = dict(color="#ff7f0e", linestyle="--", marker="s", linewidth=2)

PLOTS = [
    ("results_802154_sweep_nodes.dat", "x_value", "throughput_mbps", "Number of Nodes", "Throughput (Mbps)",
     "Network Throughput vs Number of Nodes (802.15.4)", "graph1_nodes_throughput.png", None),
    ("results_802154_sweep_nodes.dat", "x_value", "energy_joules", "Number of Nodes", "Energy (J)",
     "Energy Consumption vs Number of Nodes (802.15.4)", "graph2_nodes_energy.png", None),
    ("results_802154_sweep_flows.dat", "x_value", "delay_ms", "Number of Flows", "Delay (ms)",
     "End-to-End Delay vs Number of Flows (802.15.4)", "graph3_flows_delay.png", None),
    ("results_802154_sweep_flows.dat", "x_value", "pdr", "Number of Flows", "PDR",
     "Packet Delivery Ratio vs Number of Flows (802.15.4)", "graph4_flows_pdr.png", (0.0, 1.0)),
    ("results_802154_sweep_pkts.dat", "x_value", "throughput_mbps", "Packets per Second", "Throughput (Mbps)",
     "Network Throughput vs Packets per Second (802.15.4)", "graph5_pkts_throughput.png", None),
    ("results_802154_sweep_pkts.dat", "x_value", "drop_ratio", "Packets per Second", "Drop Ratio",
     "Packet Drop Ratio vs Packets per Second (802.15.4)", "graph6_pkts_drop.png", (0.0, 1.0)),
    ("results_802154_sweep_coverage.dat", "x_value", "throughput_mbps", "Coverage Area (× Tx_range)", "Throughput (Mbps)",
     "Network Throughput vs Coverage Area (802.15.4)", "graph7_coverage_throughput.png", None),
    ("results_802154_sweep_coverage.dat", "x_value", "delay_ms", "Coverage Area (× Tx_range)", "Delay (ms)",
     "End-to-End Delay vs Coverage Area (802.15.4)", "graph8_coverage_delay.png", None),
    ("results_802154_sweep_coverage.dat", "x_value", "pdr", "Coverage Area (× Tx_range)", "PDR",
     "Packet Delivery Ratio vs Coverage Area (802.15.4)", "graph9_coverage_pdr.png", (0.0, 1.0)),
    ("results_802154_sweep_coverage.dat", "x_value", "energy_joules", "Coverage Area (× Tx_range)", "Energy (J)",
     "Energy Consumption vs Coverage Area (802.15.4)", "graph10_coverage_energy.png", None),
]


def load_rows(path: Path):
    rows = []
    with path.open("r", encoding="ascii") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            parts = line.split()
            if len(parts) != 12:
                raise ValueError(f"Unexpected .dat format in {path}: {line}")

            rows.append(
                {
                    "x_value": float(parts[0]),
                    "algorithm": parts[1],
                    "throughput_mbps": float(parts[2]),
                    "delay_ms": float(parts[3]),
                    "pdr": float(parts[4]),
                    "drop_ratio": float(parts[5]),
                    "energy_joules": float(parts[6]),
                    "nodes": float(parts[7]),
                    "flows": float(parts[8]),
                    "packets_per_sec": float(parts[9]),
                    "coverage": float(parts[10]),
                    "sim_duration": float(parts[11]),
                }
            )
    return rows


def split_by_algorithm(rows):
    quick = sorted([r for r in rows if r["algorithm"] == "TcpQuickVegas"], key=lambda r: r["x_value"])
    vegas = sorted([r for r in rows if r["algorithm"] == "TcpVegas"], key=lambda r: r["x_value"])
    return quick, vegas


def plot_metric(ax, rows, x_key, y_key, xlabel, ylabel, title, ylim=None):
    quick, vegas = split_by_algorithm(rows)
    if quick:
        ax.plot([r[x_key] for r in quick], [r[y_key] for r in quick], label="QuickVegas", **QUICK_STYLE)
    if vegas:
        ax.plot([r[x_key] for r in vegas], [r[y_key] for r in vegas], label="Vegas", **VEGAS_STYLE)
    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel(ylabel, fontsize=12)
    ax.set_title(title, fontsize=13, fontweight="bold")
    ax.grid(True, linestyle="--", alpha=0.4)
    if ylim is not None:
        ax.set_ylim(*ylim)
    ax.legend(loc="best", fontsize=10, framealpha=0.9)


def main():
    results_dir = Path("results/quick-vegas-802154")
    data_dir = results_dir / "data"
    plots_dir = results_dir / "plots"
    plots_dir.mkdir(parents=True, exist_ok=True)

    loaded = {spec[0]: load_rows(data_dir / spec[0]) for spec in PLOTS}

    summary_fig, summary_axes = plt.subplots(2, 5, figsize=(18, 20), dpi=150)
    summary_axes = summary_axes.flatten()

    for idx, spec in enumerate(PLOTS):
        csv_name, x_key, y_key, xlabel, ylabel, title, output_name, ylim = spec
        rows = loaded[csv_name]

        fig, ax = plt.subplots(figsize=(7, 5), dpi=150)
        plot_metric(ax, rows, x_key, y_key, xlabel, ylabel, title, ylim)
        plt.tight_layout()
        fig.savefig(plots_dir / output_name, dpi=150)
        plt.close(fig)

        plot_metric(summary_axes[idx], rows, x_key, y_key, xlabel, ylabel, title, ylim)

    summary_fig.tight_layout()
    summary_fig.savefig(plots_dir / "graph_summary_802154.png", dpi=150)
    plt.close(summary_fig)


if __name__ == "__main__":
    main()
