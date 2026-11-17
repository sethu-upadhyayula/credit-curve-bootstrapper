"""Piecewise-constant hazard-rate (default-intensity) curve.

Same model as cpp/include/creditcurve/hazard_curve.hpp: segment i covers
(knot_times[i-1], knot_times[i]] with knot_times[-1] := 0, carrying a constant
hazard rate hazard_rates[i]. Survival probability is S(t) = exp(-integral of
hazard up to t), which for piecewise-constant hazard reduces to a running sum.

Python idiom contrast: survival_probability is fully vectorized over an array
of query times via `numpy.searchsorted` + a precomputed cumulative-hazard-at-
each-knot table, instead of the C++ version's per-call linear scan over knots.
This matters here specifically because CDS pricing below calls this hundreds
of times per valuation (once per protection-leg integration step); the C++
version relies on the compiler/O2 to make the scan cheap, while the Python
version avoids a Python-level loop over knots entirely.
"""
from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np


@dataclass
class HazardCurve:
    knot_times: list = field(default_factory=list)
    hazard_rates: list = field(default_factory=list)

    def add_segment(self, knot_time: float, hazard_rate: float) -> None:
        prev_knot = self.knot_times[-1] if self.knot_times else 0.0
        if knot_time <= prev_knot:
            raise ValueError("knot times must be strictly increasing")
        if hazard_rate < 0.0:
            raise ValueError("hazard rate must be non-negative")
        self.knot_times.append(knot_time)
        self.hazard_rates.append(hazard_rate)

    def _cumulative_hazard_at_knots(self) -> np.ndarray:
        knots = np.asarray(self.knot_times, dtype=float)
        hazards = np.asarray(self.hazard_rates, dtype=float)
        segment_durations = np.diff(np.concatenate(([0.0], knots)))
        return np.cumsum(segment_durations * hazards)

    def survival_probability(self, t) -> np.ndarray:
        t = np.asarray(t, dtype=float)
        if not self.knot_times:
            return np.ones_like(t)

        knots = np.asarray(self.knot_times, dtype=float)
        hazards = np.asarray(self.hazard_rates, dtype=float)
        cum_at_knots = self._cumulative_hazard_at_knots()
        cum_at_knot_start = np.concatenate(([0.0], cum_at_knots[:-1]))
        knot_start_times = np.concatenate(([0.0], knots[:-1]))

        # idx = index of the segment each t falls into; clip beyond-last-knot
        # queries to the final segment (flat extrapolation of the last hazard).
        idx = np.searchsorted(knots, t, side="left")
        idx = np.clip(idx, 0, len(knots) - 1)

        cumulative_hazard = cum_at_knot_start[idx] + hazards[idx] * (t - knot_start_times[idx])
        cumulative_hazard = np.where(t <= 0.0, 0.0, cumulative_hazard)
        return np.exp(-cumulative_hazard)

    def hazard_rate_at(self, t: float) -> float:
        if not self.knot_times:
            return 0.0
        idx = int(np.clip(np.searchsorted(self.knot_times, t, side="left"), 0, len(self.knot_times) - 1))
        return float(self.hazard_rates[idx])
