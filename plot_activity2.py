#!/usr/bin/env python3
"""Generate comparison plots for Activity 2 ns-3 results."""

from pathlib import Path

import matplotlib.pyplot as plt


def read_dat(path: Path):
    xs = []
    ys = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("time_s"):
                continue
            parts = line.split()
            if len(parts) < 2:
                continue
            xs.append(float(parts[0]))
            ys.append(float(parts[1]))
    return xs, ys


def plot_comparison(results_dir: Path):
    reno_t_x, reno_t_y = read_dat(results_dir / "activity2-reno-throughput.dat")
    cubic_t_x, cubic_t_y = read_dat(results_dir / "activity2-cubic-throughput.dat")

    plt.figure(figsize=(10, 5))
    plt.plot(reno_t_x, reno_t_y, label="TCP Reno", linewidth=1.6)
    plt.plot(cubic_t_x, cubic_t_y, label="TCP Cubic", linewidth=1.6)
    plt.title("Throughput TCP no Tempo")
    plt.xlabel("Tempo (s)")
    plt.ylabel("Throughput (Mbps)")
    plt.grid(True, linestyle="--", alpha=0.4)
    plt.legend()
    plt.tight_layout()
    plt.savefig(results_dir / "activity2-throughput-comparison.png", dpi=150)
    plt.close()

    reno_c_x, reno_c_y = read_dat(results_dir / "activity2-reno-cwnd.dat")
    cubic_c_x, cubic_c_y = read_dat(results_dir / "activity2-cubic-cwnd.dat")

    plt.figure(figsize=(10, 5))
    plt.plot(reno_c_x, reno_c_y, label="TCP Reno", linewidth=1.4)
    plt.plot(cubic_c_x, cubic_c_y, label="TCP Cubic", linewidth=1.4)
    plt.title("Evolucao da Janela de Congestionamento (CWND)")
    plt.xlabel("Tempo (s)")
    plt.ylabel("CWND (bytes)")
    plt.grid(True, linestyle="--", alpha=0.4)
    plt.legend()
    plt.tight_layout()
    plt.savefig(results_dir / "activity2-cwnd-comparison.png", dpi=150)
    plt.close()


if __name__ == "__main__":
    base_dir = Path(__file__).resolve().parent
    results = base_dir / "results"
    results.mkdir(parents=True, exist_ok=True)
    plot_comparison(results)
    print("Generated:")
    print(results / "activity2-throughput-comparison.png")
    print(results / "activity2-cwnd-comparison.png")
