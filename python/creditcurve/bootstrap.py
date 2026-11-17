"""Sequential bootstrap of a piecewise-constant hazard curve from a CDS
spread term structure. Same algorithm as cpp/src/bootstrap.cpp: solve for
each new segment's hazard rate, holding earlier segments fixed, so the model
par spread at each pillar reproduces its quoted market spread exactly.

Root finding uses `scipy.optimize.brentq`.
`bootstrap_hazard_curve_cross_checked` additionally re-solves every segment
with the hand-written bisection in root_finder.py and asserts that the two
methods agree.
"""
from __future__ import annotations

from dataclasses import dataclass, field

from scipy.optimize import brentq

from .cds_pricer import price_cds
from .discount_curve import DiscountCurve
from .hazard_curve import HazardCurve
from .root_finder import bisection_solve
from .types import CDSConventions, CDSQuote


@dataclass
class BootstrapResult:
    curve: HazardCurve
    implied_par_spreads: list = field(default_factory=list)


def _make_objective(quote: CDSQuote, prior_curve: HazardCurve, discount_curve: DiscountCurve,
                     conventions: CDSConventions):
    def objective(candidate_hazard: float) -> float:
        trial = HazardCurve(list(prior_curve.knot_times), list(prior_curve.hazard_rates))
        trial.add_segment(quote.tenor_years, candidate_hazard)
        legs = price_cds(quote.tenor_years, discount_curve, trial, conventions)
        return legs.par_spread - quote.spread

    return objective


def bootstrap_hazard_curve(
    quotes: list[CDSQuote],
    discount_curve: DiscountCurve,
    conventions: CDSConventions = CDSConventions(),
    min_hazard: float = 1e-8,
    max_hazard: float = 5.0,
    root_tol: float = 1e-12,
) -> BootstrapResult:
    if not quotes:
        raise ValueError("no quotes supplied")
    tenors = [q.tenor_years for q in quotes]
    if tenors != sorted(tenors):
        raise ValueError("quotes must be sorted ascending by tenor")

    result = BootstrapResult(curve=HazardCurve())
    for quote in quotes:
        objective = _make_objective(quote, result.curve, discount_curve, conventions)
        h = brentq(objective, min_hazard, max_hazard, xtol=root_tol)
        result.curve.add_segment(quote.tenor_years, h)
        legs = price_cds(quote.tenor_years, discount_curve, result.curve, conventions)
        result.implied_par_spreads.append(legs.par_spread)
    return result


def bootstrap_hazard_curve_cross_checked(
    quotes: list[CDSQuote],
    discount_curve: DiscountCurve,
    conventions: CDSConventions = CDSConventions(),
    min_hazard: float = 1e-8,
    max_hazard: float = 5.0,
    agreement_tol: float = 1e-6,
) -> BootstrapResult:
    """Bootstraps with scipy.optimize.brentq, then independently re-solves
    every segment with the hand-rolled bisection method and asserts the two
    converge to the same hazard rates. Raises AssertionError if they diverge
    by more than agreement_tol (absolute, in hazard-rate units)."""
    primary = bootstrap_hazard_curve(quotes, discount_curve, conventions, min_hazard, max_hazard)

    cross_check_curve = HazardCurve()
    for quote in quotes:
        objective = _make_objective(quote, cross_check_curve, discount_curve, conventions)
        h_bisection = bisection_solve(objective, min_hazard, max_hazard, tol=1e-10)
        cross_check_curve.add_segment(quote.tenor_years, h_bisection)

    for i, (h_primary, h_check) in enumerate(
        zip(primary.curve.hazard_rates, cross_check_curve.hazard_rates)
    ):
        diff = abs(h_primary - h_check)
        assert diff < agreement_tol, (
            f"brentq vs. bisection disagree at pillar {i} "
            f"({quotes[i].tenor_years}Y): {h_primary!r} vs {h_check!r}, diff={diff:.2e}"
        )

    return primary
