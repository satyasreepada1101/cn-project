# TCP Vegas experiment handoff

This folder contains the completed TCP Vegas experiments 1–4: their per-run
CSVs, summary CSVs, plots, data-grounded interpretations, and build/run
transcript. The programs follow the same structure, command-line interface,
measurement window, and CSV schema as the Veno and YeAH experiments, so the
three TCP variants can be compared directly. The `plot_results.py` script
regenerates the SVG plots from the four summary CSVs using only the Python
standard library.

## Package layout

The `tcp-vegas` handoff folder keeps the four source directories alongside
`results/`, which contains the per-run CSVs, summary CSVs, plots, and run
evidence.

## Running in ns-3.43

Copy the four `experiment-N-vegas` source directories into the ns-3.43
`scratch/` directory. From the ns-3.43 root, build and run:

```text
./ns3 build
./ns3 run "scratch/experiment-1-vegas/vegas-experiment-1 --linkRateMbps=10 --outputFile=results/experiment-1/vegas_10Mbps.csv"
./ns3 run "scratch/experiment-1-vegas/vegas-experiment-1 --linkRateMbps=5  --outputFile=results/experiment-1/vegas_5Mbps.csv"
./ns3 run "scratch/experiment-1-vegas/vegas-experiment-1 --linkRateMbps=2  --outputFile=results/experiment-1/vegas_2Mbps.csv"
./ns3 run "scratch/experiment-2-vegas/vegas-experiment-2 --delayMs=10  --outputFile=scratch/results/experiment-2/vegas_10ms.csv"
./ns3 run "scratch/experiment-2-vegas/vegas-experiment-2 --delayMs=50  --outputFile=scratch/results/experiment-2/vegas_50ms.csv"
./ns3 run "scratch/experiment-2-vegas/vegas-experiment-2 --delayMs=100 --outputFile=scratch/results/experiment-2/vegas_100ms.csv"
./ns3 run "scratch/experiment-2-vegas/vegas-experiment-2 --delayMs=200 --outputFile=scratch/results/experiment-2/vegas_200ms.csv"
./ns3 run "scratch/experiment-3-vegas/vegas-experiment-3 --lossRate=0     --outputFile=scratch/results/experiment-3/vegas_0percent.csv"
./ns3 run "scratch/experiment-3-vegas/vegas-experiment-3 --lossRate=0.001 --outputFile=scratch/results/experiment-3/vegas_0.1percent.csv"
./ns3 run "scratch/experiment-3-vegas/vegas-experiment-3 --lossRate=0.005 --outputFile=scratch/results/experiment-3/vegas_0.5percent.csv"
./ns3 run "scratch/experiment-3-vegas/vegas-experiment-3 --lossRate=0.01  --outputFile=scratch/results/experiment-3/vegas_1percent.csv"
./ns3 run "scratch/experiment-4-vegas/vegas-experiment-4 --numFlows=1 --outputFile=scratch/results/experiment-4/vegas_1flow.csv"
./ns3 run "scratch/experiment-4-vegas/vegas-experiment-4 --numFlows=2 --outputFile=scratch/results/experiment-4/vegas_2flow.csv"
./ns3 run "scratch/experiment-4-vegas/vegas-experiment-4 --numFlows=4 --outputFile=scratch/results/experiment-4/vegas_4flow.csv"
```

Each run writes one CSV row. The `summary.csv` files are the concatenation of
the per-run CSVs for that experiment, with a single header row.

## Model and measurement notes

All four programs explicitly select `ns3::TcpVegas`. Each run sends 1024-byte
packets at a 5 Mbps application rate from a sender that starts at 1 second.

Throughput is calculated over the active sender window, matching the Veno and
YeAH experiments:

- Experiments 1–3: 10 s simulation minus 1 s sender start = **9 s** window.
- Experiment 4: 20 s simulation minus 1 s sender start = **19 s** window,
  with 1, 2, or 4 competing flows.

Experiment 3 applies packet loss with a packet-based `RateErrorModel` to the
receiver-side point-to-point device, the same location used by Veno and YeAH.
