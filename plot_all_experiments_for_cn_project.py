#!/usr/bin/env python3
"""
Plot all experiments for the cn-project folder layout shown in the screenshots.

Expected layout:
cn-project/
    tcp-vegas/results/experiment-1/*.csv
    tcp-vegas/results/experiment-2/*.csv
    ...
    tcp-veno/results/experiment-1/*.csv
    tcp-yeah/results/experiment-1/*.csv
    experiment-5/summary.csv

Run this script from inside cn-project:
    python3 plot_all_experiments.py

Install:
    pip install pandas matplotlib numpy

Outputs:
    plots/all_experiments/*.png
    plots/all_experiments/data_inventory.csv

The script tries to recognise common CSV header names and also extracts
experiment settings from filenames such as vegas_2Mbps.csv when needed.
If a CSV uses an unusual schema, the inventory and warnings help identify it.
"""

from pathlib import Path
import re
import argparse
import pandas as pd
import matplotlib.pyplot as plt


ALGORITHMS = {
    "tcp-vegas": "Vegas",
    "tcp-veno": "Veno",
    "tcp-yeah": "YeAH",
}
COLORS = {
    "Vegas": "#38BDF8",
    "Veno": "#34D399",
    "YeAH": "#FBBF24",
}

# Header aliases after removing punctuation/spaces and converting to lowercase.
ALIASES = {
    "throughput": [
        "throughput", "throughputmbps", "throughput_mbps",
        "avgthroughput", "avgthroughputmbps", "goodput", "goodputmbps",
        "aggregatethroughput", "aggregatethroughputmbps",
        "totalthroughput", "totalthroughputmbps"
    ],
    "bandwidth": [
        "bandwidth", "bandwidthmbps", "linkrate", "linkratembps",
        "datarate", "dataratembps"
    ],
    "delay": ["delay", "delayms", "linkdelay", "linkdelayms", "latency", "latencyms"],
    "loss": [
        "loss", "lossrate", "packetloss", "packetlossrate",
        "packetlosspercent", "losspercent", "lossratepercent"
    ],
    "flows": ["flows", "numflows", "numberofflows", "flowcount", "nflows"],
    "fairness": ["fairness", "fairnessindex", "jainfairness", "jainfairnessindex", "jainindex"],
    "algorithm": ["algorithm", "tcp", "protocol", "congestioncontrol"],
}


def norm(value):
    return re.sub(r"[^a-z0-9]", "", str(value).lower())


def locate_column(df, key):
    aliases = {norm(x) for x in ALIASES[key]}
    for col in df.columns:
        if norm(col) in aliases:
            return col
    # More permissive fallback for headers with units or prefixes.
    for col in df.columns:
        c = norm(col)
        if key == "throughput" and "throughput" in c:
            return col
        if key == "bandwidth" and ("bandwidth" in c or "linkrate" in c):
            return col
        if key == "delay" and "delay" in c:
            return col
        if key == "loss" and "loss" in c:
            return col
        if key == "flows" and "flow" in c and ("num" in c or "count" in c or c == "flows"):
            return col
        if key == "fairness" and ("fairness" in c or "jain" in c):
            return col
        if key == "algorithm" and ("algorithm" in c or "protocol" in c):
            return col
    return None


def numeric_series(series):
    # Handles strings like "2 Mbps", "0.5%", etc.
    return pd.to_numeric(
        series.astype(str).str.extract(r"([-+]?\d*\.?\d+)")[0],
        errors="coerce"
    )


def algorithm_from_path(path):
    for folder, algo in ALGORITHMS.items():
        if folder in [p.lower() for p in path.parts]:
            return algo
    name = path.name.lower()
    for token, algo in [("vegas", "Vegas"), ("veno", "Veno"), ("yeah", "YeAH")]:
        if token in name:
            return algo
    return None


def value_from_filename(path, exp_no):
    name = path.stem.lower()
    if exp_no == 1:
        match = re.search(r"(\d+(?:\.\d+)?)\s*mbps", name)
        if match:
            return float(match.group(1))
        match = re.search(r"(?:bandwidth|bw)[_-]?(\d+(?:\.\d+)?)", name)
        if match:
            return float(match.group(1))
    elif exp_no == 2:
        match = re.search(r"(\d+(?:\.\d+)?)\s*ms", name)
        if match:
            return float(match.group(1))
        match = re.search(r"(?:delay|latency)[_-]?(\d+(?:\.\d+)?)", name)
        if match:
            return float(match.group(1))
    elif exp_no == 3:
        match = re.search(r"(\d+(?:\.\d+)?)\s*%", name)
        if match:
            return float(match.group(1))
        match = re.search(r"(?:loss|ploss)[_-]?(\d+(?:\.\d+)?)", name)
        if match:
            return float(match.group(1))
    elif exp_no == 4:
        match = re.search(r"(\d+)\s*(?:flows?|flow)", name)
        if match:
            return float(match.group(1))
        match = re.search(r"(?:flows?|n)[_-]?(\d+)", name)
        if match:
            return float(match.group(1))
    return None


def find_csvs(root, exp_no):
    found = []
    # Per-algorithm result directories.
    for folder in ALGORITHMS:
        result_dir = root / folder / "results" / f"experiment-{exp_no}"
        if result_dir.exists():
            found.extend(sorted(result_dir.glob("*.csv")))

    # Experiment 5 has a top-level summary.csv in the shown project layout.
    if exp_no == 5:
        top = root / "experiment-5"
        if top.exists():
            found.extend(sorted(top.glob("*.csv")))

    # De-duplicate paths while preserving order.
    return list(dict.fromkeys(found))


def read_csv_safely(path):
    try:
        df = pd.read_csv(path)
        if df.empty:
            print(f"[WARN] Empty CSV: {path}")
            return None
        return df
    except Exception as exc:
        print(f"[WARN] Cannot read {path}: {exc}")
        return None


def collect_experiment(root, exp_no):
    rows = []
    files = find_csvs(root, exp_no)
    if not files:
        print(f"[WARN] No CSV files found for Experiment {exp_no}")
        return pd.DataFrame()

    for path in files:
        df = read_csv_safely(path)
        if df is None:
            continue

        algo = algorithm_from_path(path)
        algo_col = locate_column(df, "algorithm")
        if algo_col:
            df["__algorithm"] = df[algo_col].astype(str).map(
                lambda x: next(
                    (v for k, v in [("vegas", "Vegas"), ("veno", "Veno"), ("yeah", "YeAH")]
                     if k in x.lower()),
                    algo or x
                )
            )
        else:
            df["__algorithm"] = algo

        t_col = locate_column(df, "throughput")
        if t_col is None:
            print(f"[WARN] No throughput column in {path}")
            continue
        df["__throughput"] = numeric_series(df[t_col])

        key = {1: "bandwidth", 2: "delay", 3: "loss", 4: "flows"}.get(exp_no)
        if key:
            x_col = locate_column(df, key)
            if x_col:
                df["__x"] = numeric_series(df[x_col])
            else:
                parsed = value_from_filename(path, exp_no)
                if parsed is not None:
                    df["__x"] = parsed
                else:
                    print(f"[WARN] Cannot identify experiment setting in {path}")
                    continue

        fairness_col = locate_column(df, "fairness")
        if fairness_col:
            df["__fairness"] = numeric_series(df[fairness_col])

        # Keep rows with valid throughput.
        df = df.dropna(subset=["__throughput"])
        for _, row in df.iterrows():
            rows.append({
                "algorithm": row.get("__algorithm"),
                "x": row.get("__x"),
                "throughput": row.get("__throughput"),
                "fairness": row.get("__fairness"),
                "source": str(path.relative_to(root)),
            })

    return pd.DataFrame(rows)


def style():
    plt.rcParams.update({
        "figure.facecolor": "#080808",
        "axes.facecolor": "#111111",
        "axes.edgecolor": "#888888",
        "axes.labelcolor": "white",
        "xtick.color": "white",
        "ytick.color": "white",
        "text.color": "white",
        "grid.color": "#666666",
        "font.size": 11,
        "legend.facecolor": "#202020",
        "legend.edgecolor": "#777777",
    })


def save(fig, out, filename):
    out.mkdir(parents=True, exist_ok=True)
    target = out / filename
    fig.savefig(target, dpi=300, bbox_inches="tight", facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f"[SAVED] {target}")


def plot_sweep(data, exp_no, xlabel, title, filename, out):
    if data.empty or data["x"].isna().all():
        print(f"[SKIP] Experiment {exp_no}: no x-axis values.")
        return
    fig, ax = plt.subplots(figsize=(9.5, 5.5))
    plotted = False
    for algo in ("Vegas", "Veno", "YeAH"):
        part = data[data["algorithm"] == algo].dropna(subset=["x", "throughput"])
        if part.empty:
            continue
        grouped = part.groupby("x", as_index=False)["throughput"].mean().sort_values("x")
        ax.plot(grouped["x"], grouped["throughput"], marker="o", linewidth=2.5,
                markersize=7, color=COLORS[algo], label=f"TCP {algo}")
        plotted = True
    if not plotted:
        plt.close(fig)
        print(f"[SKIP] Experiment {exp_no}: no recognised algorithm data.")
        return
    ax.set_title(title, fontsize=15, fontweight="bold", pad=14)
    ax.set_xlabel(xlabel)
    ax.set_ylabel("Throughput (Mbps)")
    ax.grid(True, linestyle="--", alpha=0.35)
    ax.legend()
    save(fig, out, filename)


def plot_exp4(data, out):
    if data.empty:
        return
    valid = data.dropna(subset=["x", "throughput"])
    if valid.empty:
        print("[SKIP] Experiment 4: no flow-count data.")
        return

    fig, ax = plt.subplots(figsize=(9, 5.5))
    plotted = False
    for algo in ("Vegas", "Veno", "YeAH"):
        part = valid[valid["algorithm"] == algo]
        if part.empty:
            continue
        grouped = part.groupby("x", as_index=False)["throughput"].mean().sort_values("x")
        ax.plot(grouped["x"], grouped["throughput"], marker="o", linewidth=2.5,
                color=COLORS[algo], label=f"TCP {algo}")
        plotted = True
    if plotted:
        ax.set_title("Experiment 4: Aggregate Throughput vs Number of Flows",
                     fontsize=15, fontweight="bold")
        ax.set_xlabel("Number of Simultaneous Flows")
        ax.set_ylabel("Aggregate Throughput (Mbps)")
        ax.grid(True, linestyle="--", alpha=0.35)
        ax.legend()
        save(fig, out, "exp4_aggregate_throughput_vs_flows.png")
    else:
        plt.close(fig)

    fair = valid.dropna(subset=["fairness"])
    if not fair.empty:
        fig, ax = plt.subplots(figsize=(9, 5.5))
        for algo in ("Vegas", "Veno", "YeAH"):
            part = fair[fair["algorithm"] == algo]
            if part.empty:
                continue
            grouped = part.groupby("x", as_index=False)["fairness"].mean().sort_values("x")
            ax.plot(grouped["x"], grouped["fairness"], marker="o", linewidth=2.5,
                    color=COLORS[algo], label=f"TCP {algo}")
        ax.set_ylim(0, 1.05)
        ax.set_title("Experiment 4: Jain's Fairness vs Number of Flows",
                     fontsize=15, fontweight="bold")
        ax.set_xlabel("Number of Simultaneous Flows")
        ax.set_ylabel("Jain's Fairness Index (0–1)")
        ax.grid(True, linestyle="--", alpha=0.35)
        ax.legend()
        save(fig, out, "exp4_jain_fairness_vs_flows.png")


def plot_exp5(root, out):
    summary = root / "experiment-5" / "summary.csv"
    if not summary.exists():
        print(f"[WARN] Experiment 5 summary not found: {summary}")
        return
    df = read_csv_safely(summary)
    if df is None:
        return

    algo_col = locate_column(df, "algorithm")
    throughput_col = locate_column(df, "throughput")
    fairness_col = locate_column(df, "fairness")

    # Long format: one row per TCP algorithm.
    if algo_col and throughput_col:
        df["__algo"] = df[algo_col].astype(str).map(
            lambda x: next((v for k, v in [("vegas", "Vegas"), ("veno", "Veno"), ("yeah", "YeAH")]
                            if k in x.lower()), x)
        )
        df["__throughput"] = numeric_series(df[throughput_col])
        plot_exp5_bars(df[["__algo", "__throughput"]].dropna(), out)
    else:
        # Wide format: columns such as Vegas_Mbps, Veno_Mbps, YeAH_Mbps.
        wide = {}
        for col in df.columns:
            c = norm(col)
            for token, algo in [("vegas", "Vegas"), ("veno", "Veno"), ("yeah", "YeAH")]:
                if token in c and ("throughput" in c or "mbps" in c or c == token):
                    wide[algo] = numeric_series(df[col]).dropna().iloc[0] if not numeric_series(df[col]).dropna().empty else None
        if wide:
            plot_exp5_bars(
                pd.DataFrame([{"__algo": k, "__throughput": v} for k, v in wide.items() if v is not None]),
                out
            )
        else:
            print("[WARN] Could not identify Vegas/Veno/YeAH throughput columns in experiment-5/summary.csv")

    if fairness_col:
        vals = numeric_series(df[fairness_col]).dropna()
        if not vals.empty:
            value = float(vals.iloc[0])
            fig, ax = plt.subplots(figsize=(7, 4.5))
            ax.bar(["Vegas + Veno + YeAH"], [value], color="#C084FC", width=0.5)
            ax.set_ylim(0, 1.05)
            ax.text(0, value + 0.025, f"{value:.4f}", ha="center", color="white")
            ax.set_title("Experiment 5: Jain's Fairness Index", fontsize=15, fontweight="bold")
            ax.set_ylabel("Jain's Fairness Index (0–1)")
            ax.grid(axis="y", linestyle="--", alpha=0.35)
            ax.set_axisbelow(True)
            save(fig, out, "exp5_jain_fairness.png")


def plot_exp5_bars(data, out):
    data = data.groupby("__algo", as_index=False)["__throughput"].mean()
    data = data[data["__algo"].isin(["Vegas", "Veno", "YeAH"])]
    if data.empty:
        return
    data["order"] = data["__algo"].map({"Vegas": 0, "Veno": 1, "YeAH": 2})
    data = data.sort_values("order")
    fig, ax = plt.subplots(figsize=(8.5, 5.2))
    bars = ax.bar(data["__algo"], data["__throughput"],
                  color=[COLORS[a] for a in data["__algo"]])
    ax.bar_label(bars, fmt="%.3f", padding=4, color="white")
    ax.set_title("Experiment 5: Throughput During Cross-Algorithm Competition",
                 fontsize=14, fontweight="bold")
    ax.set_xlabel("TCP Algorithm")
    ax.set_ylabel("Throughput (Mbps)")
    ax.grid(axis="y", linestyle="--", alpha=0.35)
    ax.set_axisbelow(True)
    save(fig, out, "exp5_competing_algorithm_throughput.png")

    total = data["__throughput"].sum()
    if total > 0:
        shares = data.copy()
        shares["share"] = shares["__throughput"] / total * 100
        fig, ax = plt.subplots(figsize=(8.5, 5.2))
        bars = ax.bar(shares["__algo"], shares["share"],
                      color=[COLORS[a] for a in shares["__algo"]])
        ax.bar_label(bars, fmt="%.1f%%", padding=4, color="white")
        ax.set_title("Experiment 5: Share of Aggregate Throughput",
                     fontsize=14, fontweight="bold")
        ax.set_xlabel("TCP Algorithm")
        ax.set_ylabel("Throughput Share (%)")
        ax.grid(axis="y", linestyle="--", alpha=0.35)
        ax.set_axisbelow(True)
        save(fig, out, "exp5_throughput_share.png")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".", help="Path to cn-project folder")
    parser.add_argument("--output", default="plots/all_experiments")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    out = root / args.output
    out.mkdir(parents=True, exist_ok=True)
    style()

    all_data = {}
    for exp in range(1, 5):
        data = collect_experiment(root, exp)
        all_data[exp] = data
        if not data.empty:
            data.to_csv(out / f"experiment-{exp}-combined-data.csv", index=False)

    plot_sweep(
        all_data[1], 1, "Bandwidth (Mbps)",
        "Experiment 1: Bandwidth vs TCP Throughput",
        "exp1_bandwidth_vs_throughput.png", out
    )
    plot_sweep(
        all_data[2], 2, "Network Delay (ms)",
        "Experiment 2: Delay vs TCP Throughput",
        "exp2_delay_vs_throughput.png", out
    )
    plot_sweep(
        all_data[3], 3, "Packet Loss Rate (%)",
        "Experiment 3: Packet Loss vs TCP Throughput",
        "exp3_packet_loss_vs_throughput.png", out
    )
    plot_exp4(all_data[4], out)
    plot_exp5(root, out)

    # Inventory: source files and headers, useful if a particular plot is skipped.
    inventory = []
    for exp in range(1, 6):
        for path in find_csvs(root, exp):
            frame = read_csv_safely(path)
            inventory.append({
                "experiment": exp,
                "file": str(path.relative_to(root)),
                "columns": ", ".join(map(str, frame.columns)) if frame is not None else ""
            })
    pd.DataFrame(inventory).to_csv(out / "data_inventory.csv", index=False)

    print("\nFinished. Check:", out)
    print("If an experiment plot is skipped, inspect data_inventory.csv and the warnings above.")


if __name__ == "__main__":
    main()
