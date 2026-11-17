"""Bucketed and parallel CS01 ("credit spread 01") risk.

Same model as cpp/src/risk.cpp. The loop over pillars is not vectorized:
each bucket requires an independent full rebootstrap of the curve, which is
itself a sequential calculation, so there is no array operation to batch
across buckets.
"""
from __future__ import annotations

from dataclasses import dataclass, replace

from .bootstrap import bootstrap_hazard_curve
from .cds_pricer import mark_to_market
from .discount_curve import DiscountCurve
from .types import CDSConventions, CDSQuote


@dataclass(frozen=True)
class CS01Result:
    bucketed: list
    parallel: float


def _price_with_quotes(quotes, notional, position_maturity_years, contract_spread,
                        discount_curve: DiscountCurve, conventions: CDSConventions) -> float:
    calibration = bootstrap_hazard_curve(quotes, discount_curve, conventions)
    return mark_to_market(notional, contract_spread, position_maturity_years, discount_curve,
                           calibration.curve, conventions)


def compute_cs01(
    base_quotes: list[CDSQuote],
    notional: float,
    position_maturity_years: float,
    contract_spread: float,
    discount_curve: DiscountCurve,
    conventions: CDSConventions = CDSConventions(),
    bump_bps: float = 1.0,
) -> CS01Result:
    bump = bump_bps / 10000.0
    base_pv = _price_with_quotes(base_quotes, notional, position_maturity_years, contract_spread,
                                  discount_curve, conventions)

    bucketed = []
    for i in range(len(base_quotes)):
        bumped = list(base_quotes)
        bumped[i] = replace(bumped[i], spread=bumped[i].spread + bump)
        bumped_pv = _price_with_quotes(bumped, notional, position_maturity_years, contract_spread,
                                        discount_curve, conventions)
        bucketed.append(bumped_pv - base_pv)

    parallel_bumped = [replace(q, spread=q.spread + bump) for q in base_quotes]
    parallel_pv = _price_with_quotes(parallel_bumped, notional, position_maturity_years,
                                      contract_spread, discount_curve, conventions)

    return CS01Result(bucketed=bucketed, parallel=parallel_pv - base_pv)
