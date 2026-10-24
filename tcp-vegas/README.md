# TCP Vegas experiment handoff

This folder contains the completed TCP Vegas experiments 1–4: their summary
CSVs, plots, data-grounded interpretations, and build/run transcript. The
`plot_results.py` script regenerates the SVG plots from the four CSV files
using only the Python standard library.

## Package layout

The `tcp-vegas` handoff folder keeps the four source directories alongside
`results/`, which contains the CSV summaries, plots, and run evidence.

## Running in ns-3.43

Copy the four `experiment-N-vegas` source directories into the ns-3.43
`scratch/` directory. From the ns-3.43 root, build and run:

```text
./ns3 build
./ns3 run scratch/experiment-1-vegas/vegas-experiment-1 --bandwidth=10
./ns3 run scratch/experiment-1-vegas/vegas-experiment-1 --bandwidth=5
./ns3 run scratch/experiment-1-vegas/vegas-experiment-1 --bandwidth=2
./ns3 run scratch/experiment-2-vegas/vegas-experiment-2
./ns3 run scratch/experiment-3-vegas/vegas-experiment-3
./ns3 run scratch/experiment-4-vegas/vegas-experiment-4
```

The simulation programs write their CSVs to `scratch/results/tcp-vegas/`.
Ensure the `experiment-1` through `experiment-4` output directories exist
there before running the programs.

## Model and measurement notes

All four programs explicitly select `ns3::TcpVegas`. Throughput is calculated
as received bytes times eight divided by the full 10-second simulation
duration. Experiment 3 applies packet loss with a packet-based
`RateErrorModel` to the receiver-side point-to-point device.
