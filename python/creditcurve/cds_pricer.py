"""CDS premium/protection leg pricing.

Same model as cpp/src/cds_pricer.cpp. Python idiom contrast: both legs are
priced with vectorized NumPy array operations (one `np.arange` + a handful of
array ops) instead of the C++ version's explicit `for` loops over payment
dates / integration steps. See scripts/benchmark.py for the measured speedup.
"""
from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from .discount_curve import DiscountCurve
from .hazard_curve import HazardCurve
from .types import CDSConventions


@dataclass(frozen=True)
class CDSLegs:
    risky_annuity: float
    protection_leg_pv: float
    par_spread: float


def price_cds(
    maturity_years: float,
    discount_curve: DiscountCurve,
    hazard_curve: HazardCurve,
    conventions: CDSConventions = CDSConventions(),
) -> CDSLegs:
    if maturity_years <= 0.0:
        raise ValueError("maturity must be > 0")

    # --- Premium leg: quarterly (or conventions.payments_per_year) schedule ---
    n_payments = int(round(maturity_years * conventions.payments_per_year))
    dt = 1.0 / conventions.payments_per_year
    payment_times = np.arange(1, n_payments + 1) * dt

    q = hazard_curve.survival_probability(payment_times)
    q_prev = np.concatenate(([1.0], q[:-1]))
    df = discount_curve.discount_factor(payment_times)

    risky_annuity = float(
        np.sum(dt * q * df + conventions.accrual_fraction * dt * (q_prev - q) * df)
    )

    # --- Protection leg: fine numerical integration of DF(t) * (-dS(t)) ---
    steps_per_year = 200
    total_steps = int(round(maturity_years * steps_per_year))
    step_dt = maturity_years / total_steps
    step_times = np.arange(1, total_steps + 1) * step_dt

    q_steps = hazard_curve.survival_probability(step_times)
    q_steps_prev = np.concatenate(([1.0], q_steps[:-1]))
    mid_times = step_times - 0.5 * step_dt
    df_mid = discount_curve.discount_factor(mid_times)

    protection_leg_pv = (1.0 - conventions.recovery_rate) * float(
        np.sum(df_mid * (q_steps_prev - q_steps))
    )

    par_spread = protection_leg_pv / risky_annuity
    return CDSLegs(risky_annuity, protection_leg_pv, par_spread)


def mark_to_market(
    notional: float,
    contract_spread: float,
    maturity_years: float,
    discount_curve: DiscountCurve,
    hazard_curve: HazardCurve,
    conventions: CDSConventions = CDSConventions(),
) -> float:
    legs = price_cds(maturity_years, discount_curve, hazard_curve, conventions)
    return notional * (legs.protection_leg_pv - contract_spread * legs.risky_annuity)
