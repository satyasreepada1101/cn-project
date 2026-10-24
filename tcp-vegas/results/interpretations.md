# TCP Vegas experiments: results and interpretation

These interpretations describe the recorded results only. Each experiment
uses TCP Vegas. The source for the Veno baseline was not present in the project
folder, so exact parity with the Veno topology and loss-model placement could
not be independently checked.

## Experiment 1 — Bottleneck bandwidth

Throughput increased as bottleneck capacity rose: 0.989594 Mbps at 2 Mbps,
1.84156 Mbps at 5 Mbps, and 1.99967 Mbps at 10 Mbps. In these runs, the
increase from 5 to 10 Mbps of link capacity was smaller than the increase from
2 to 5 Mbps, so throughput did not scale proportionally with link capacity.

## Experiment 2 — Bottleneck delay

Throughput was 1.99967 Mbps at 10 ms, 1.11002 Mbps at 50 ms, 0.589005 Mbps at
100 ms, and 0.622528 Mbps at 200 ms. The results are lower at longer delays
overall, with a small increase from 100 ms to 200 ms.

## Experiment 3 — Packet loss

Throughput was 1.99967 Mbps at both 0% and 0.1% loss, then decreased to
1.77439 Mbps at 0.5% and 1.57123 Mbps at 1%. The 0.1% run matching the
zero-loss run is an observed result of this simulation, not evidence that the
two loss rates always have equivalent effects.

Loss is applied by a packet-based `RateErrorModel` on the receiver-side device
of the point-to-point link. The Veno source was not available to check whether
it applies loss at the same location.

## Experiment 4 — Same-algorithm flow fairness

For 1, 2, and 4 Vegas flows, aggregate throughput was 1.99967, 3.99933, and
5.21748 Mbps, respectively. Jain fairness was 1, 1, and 0.998517. The four
flows have similar but not identical throughputs (1.3353, 1.33366, 1.3312,
and 1.21733 Mbps), corresponding to high fairness in this run.

## Measurement note

Throughput is calculated as received bytes multiplied by eight and divided by
the full 10-second simulation duration, matching the existing Vegas
Experiment 1 and 2 result calculations. The sender application starts at 1
second, so this is an average over the full simulation interval, not only the
active application interval.
