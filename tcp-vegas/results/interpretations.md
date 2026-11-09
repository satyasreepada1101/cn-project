# TCP Vegas experiments: results and interpretation

These interpretations describe the recorded results only. Each experiment uses
TCP Vegas. The programs now use the same baseline topology, traffic pattern,
sender timing, measurement window, and CSV schema as the Veno experiments, so
the two sets can be compared directly. The remaining throughput differences are
due to the congestion-control algorithms themselves.

## Experiment 1 — Bottleneck bandwidth

Throughput increased with bottleneck capacity: 1.81028 Mbps at 2 Mbps,
3.88186 Mbps at 5 Mbps, and 4.15915 Mbps at 10 Mbps. The gain from 5 to
10 Mbps (0.27729 Mbps) was much smaller than the gain from 2 to 5 Mbps
(2.07158 Mbps), so throughput did not scale proportionally with link capacity
in these runs.

## Experiment 2 — Bottleneck delay

Throughput fell as propagation delay increased: 4.15915 Mbps at 10 ms,
2.54591 Mbps at 50 ms, 0.822606 Mbps at 100 ms, and 0.880256 Mbps at 200 ms.
The 200 ms run was slightly higher than the 100 ms run; that is an observed
point in this simulation, not evidence of a general increase at longer delays.

## Experiment 3 — Packet loss

Throughput was 4.15915 Mbps at 0% loss, 4.39927 Mbps at 0.1%, 3.80086 Mbps at
0.5%, and 2.60007 Mbps at 1%. Throughput stayed high through 0.1% and dropped
as loss rose to 0.5% and 1%. The 0.1% run being higher than the zero-loss run
is an observed result, not evidence that 0.1% loss always improves throughput.

Loss is applied by a packet-based `RateErrorModel` on the receiver-side device
of the point-to-point link, the same location used by Veno and YeAH.

## Experiment 4 — Same-algorithm flow fairness

For 1, 2, and 4 Vegas flows, aggregate throughput was 4.1719, 9.05538, and
9.06826 Mbps, respectively. Jain fairness was 1 in all three cases.

- 1 flow: 4.1719 Mbps
- 2 flows: 4.52825 and 4.52712 Mbps (aggregate 9.05538 Mbps)
- 4 flows: 2.26825, 2.26757, 2.26622, and 2.26622 Mbps
  (aggregate 9.06826 Mbps)

The four-flow run shares the bottleneck almost exactly equally in this
simulation, which is why Jain's index rounds to 1.

## Measurement note

Throughput is received bytes multiplied by eight and divided by the active
sender window, matching Veno and YeAH:

- Experiments 1–3: 10 s simulation minus 1 s sender start = 9 s window.
- Experiment 4: 20 s simulation minus 1 s sender start = 19 s window.

The sender's `OnTime` and `OffTime` are explicitly set to 1 s and 0 s, so the
offered load is continuous at 5 Mbps, as in the Veno experiments.
