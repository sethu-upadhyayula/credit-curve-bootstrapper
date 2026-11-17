"""Continuously-compounded zero-rate discount curve.

Same model as cpp/include/creditcurve/discount_curve.hpp: the zero rate z(t)
is linearly interpolated between pillar times (flat-extrapolated beyond the
first/last pillar), and DF(t) = exp(-z(t) * t). This deliberately keeps rate-
curve construction simple so the credit-curve bootstrap, the actual subject
of this library, is the interesting part.

Python idiom contrast vs. the C++ port: interpolation is a single
`numpy.interp` call (which already flat-extrapolates outside the given range
by default) instead of a hand-rolled linear scan over pillars.
"""
from __future__ import annotations

from dataclasses import dataclass

import numpy as np


@dataclass(frozen=True)
class DiscountCurve:
    pillar_times: np.ndarray
    zero_rates: np.ndarray

    def __post_init__(self) -> None:
        times = np.asarray(self.pillar_times, dtype=float)
        rates = np.asarray(self.zero_rates, dtype=float)
        if times.ndim != 1 or times.shape != rates.shape or times.size == 0:
            raise ValueError("pillar_times/zero_rates must be equal-length 1-D arrays")
        if np.any(times <= 0.0):
            raise ValueError("pillar times must be > 0")
        if np.any(np.diff(times) <= 0.0):
            raise ValueError("pillar times must be strictly increasing")
        object.__setattr__(self, "pillar_times", times)
        object.__setattr__(self, "zero_rates", rates)

    @staticmethod
    def flat(rate: float, max_tenor_years: float = 30.0) -> "DiscountCurve":
        return DiscountCurve(
            np.array([max_tenor_years * 0.5, max_tenor_years]),
            np.array([rate, rate]),
        )

    def zero_rate(self, t) -> np.ndarray:
        t = np.asarray(t, dtype=float)
        return np.interp(t, self.pillar_times, self.zero_rates)

    def discount_factor(self, t) -> np.ndarray:
        t = np.asarray(t, dtype=float)
        z = self.zero_rate(t)
        return np.where(t <= 0.0, 1.0, np.exp(-z * t))
