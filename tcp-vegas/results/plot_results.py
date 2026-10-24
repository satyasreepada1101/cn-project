import csv
from pathlib import Path
from xml.sax.saxutils import escape


RESULTS_DIR = Path(__file__).resolve().parent
PLOT_DIR = RESULTS_DIR / "plots"
WIDTH = 900
HEIGHT = 560
LEFT = 100
RIGHT = 50
TOP = 70
BOTTOM = 90


def read_rows(experiment):
    path = RESULTS_DIR / experiment / "summary.csv"
    with path.open(newline="", encoding="utf-8") as csv_file:
        return list(csv.DictReader(csv_file))


def make_plot(filename, title, x_label, y_label, series, x_ticks, y_max):
    plot_width = WIDTH - LEFT - RIGHT
    plot_height = HEIGHT - TOP - BOTTOM
    x_min, x_max = min(x_ticks), max(x_ticks)

    def x_position(value):
        if x_max == x_min:
            return LEFT + plot_width / 2
        return LEFT + (value - x_min) * plot_width / (x_max - x_min)

    def y_position(value):
        return TOP + plot_height - value * plot_height / y_max

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" '
        f'height="{HEIGHT}" viewBox="0 0 {WIDTH} {HEIGHT}">',
        '<rect width="100%" height="100%" fill="white"/>',
        '<style>text{font-family:Arial,sans-serif;fill:#263238}'
        '.grid{stroke:#d9e1e5;stroke-width:1}.axis{stroke:#455a64;stroke-width:2}'
        '.tick{font-size:14px}.title{font-size:23px;font-weight:bold}'
        '.label{font-size:16px}.legend{font-size:14px}</style>',
        f'<text class="title" x="{WIDTH / 2}" y="36" text-anchor="middle">'
        f"{escape(title)}</text>",
    ]

    y_ticks = [y_max * index / 5 for index in range(6)]
    for tick in y_ticks:
        y = y_position(tick)
        parts.append(
            f'<line class="grid" x1="{LEFT}" y1="{y:.1f}" '
            f'x2="{WIDTH - RIGHT}" y2="{y:.1f}"/>'
        )
        parts.append(
            f'<text class="tick" x="{LEFT - 12}" y="{y + 5:.1f}" '
            f'text-anchor="end">{tick:.1f}</text>'
        )

    for tick in x_ticks:
        x = x_position(tick)
        parts.append(
            f'<line class="grid" x1="{x:.1f}" y1="{TOP}" '
            f'x2="{x:.1f}" y2="{HEIGHT - BOTTOM}"/>'
        )
        parts.append(
            f'<text class="tick" x="{x:.1f}" y="{HEIGHT - BOTTOM + 24}" '
            f'text-anchor="middle">{tick:g}</text>'
        )

    parts.extend(
        [
            f'<line class="axis" x1="{LEFT}" y1="{TOP}" x2="{LEFT}" '
            f'y2="{HEIGHT - BOTTOM}"/>',
            f'<line class="axis" x1="{LEFT}" y1="{HEIGHT - BOTTOM}" '
            f'x2="{WIDTH - RIGHT}" y2="{HEIGHT - BOTTOM}"/>',
            f'<text class="label" x="{LEFT + plot_width / 2}" '
            f'y="{HEIGHT - 24}" text-anchor="middle">{escape(x_label)}</text>',
            f'<text class="label" x="25" y="{TOP + plot_height / 2}" '
            f'text-anchor="middle" transform="rotate(-90 25 '
            f'{TOP + plot_height / 2})">{escape(y_label)}</text>',
        ]
    )

    colors = ["#1565c0", "#ef6c00", "#2e7d32"]
    for series_index, (name, points) in enumerate(series):
        color = colors[series_index % len(colors)]
        coordinates = " ".join(
            f"{x_position(x):.1f},{y_position(y):.1f}" for x, y in points
        )
        parts.append(
            f'<polyline fill="none" stroke="{color}" stroke-width="3" '
            f'points="{coordinates}"/>'
        )
        for x, y in points:
            parts.append(
                f'<circle cx="{x_position(x):.1f}" cy="{y_position(y):.1f}" '
                f'r="5" fill="{color}"/>'
            )

        legend_y = TOP + 20 + series_index * 24
        parts.append(
            f'<line x1="{WIDTH - RIGHT - 180}" y1="{legend_y}" '
            f'x2="{WIDTH - RIGHT - 150}" y2="{legend_y}" '
            f'stroke="{color}" stroke-width="3"/>'
        )
        parts.append(
            f'<text class="legend" x="{WIDTH - RIGHT - 142}" '
            f'y="{legend_y + 5}">{escape(name)}</text>'
        )

    parts.append("</svg>")
    PLOT_DIR.mkdir(parents=True, exist_ok=True)
    (PLOT_DIR / filename).write_text("\n".join(parts), encoding="utf-8")


def main():
    bandwidth_rows = read_rows("experiment-1")
    delay_rows = read_rows("experiment-2")
    loss_rows = read_rows("experiment-3")
    flow_rows = read_rows("experiment-4")

    bandwidth_points = [
        (float(row["Bottleneck_Bandwidth_Mbps"]), float(row["Throughput_Mbps"]))
        for row in bandwidth_rows
    ]
    delay_points = [
        (float(row["Bottleneck_Delay_ms"]), float(row["Throughput_Mbps"]))
        for row in delay_rows
    ]
    loss_points = [
        (float(row["Packet_Loss_Percent"]), float(row["Throughput_Mbps"]))
        for row in loss_rows
    ]

    make_plot(
        "experiment-1-bandwidth-vs-throughput.svg",
        "TCP Vegas: Bottleneck Bandwidth vs Throughput",
        "Bottleneck bandwidth (Mbps)",
        "Throughput (Mbps)",
        [("TCP Vegas", sorted(bandwidth_points))],
        sorted({point[0] for point in bandwidth_points}),
        2.2,
    )
    make_plot(
        "experiment-2-delay-vs-throughput.svg",
        "TCP Vegas: Bottleneck Delay vs Throughput",
        "Bottleneck delay (ms)",
        "Throughput (Mbps)",
        [("TCP Vegas", sorted(delay_points))],
        sorted({point[0] for point in delay_points}),
        2.2,
    )
    make_plot(
        "experiment-3-packet-loss-vs-throughput.svg",
        "TCP Vegas: Packet Loss vs Throughput",
        "Packet loss (%)",
        "Throughput (Mbps)",
        [("TCP Vegas", sorted(loss_points))],
        sorted({point[0] for point in loss_points}),
        2.2,
    )

    flow_series = []
    for flow_count in (1, 2, 4):
        points = [
            (float(row["Flow_ID"]), float(row["Throughput_Mbps"]))
            for row in flow_rows
            if int(row["Number_of_Flows"]) == flow_count
        ]
        flow_series.append((f"{flow_count} flow(s)", sorted(points)))
    make_plot(
        "experiment-4-flow-id-vs-throughput.svg",
        "TCP Vegas: Per-Flow Throughput by Flow Count",
        "Flow ID",
        "Throughput (Mbps)",
        flow_series,
        [1, 2, 3, 4],
        2.2,
    )

    for plot_path in sorted(PLOT_DIR.glob("*.svg")):
        print(plot_path)


if __name__ == "__main__":
    main()
