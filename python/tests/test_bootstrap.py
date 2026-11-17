import pytest

from creditcurve import (
    CDSConventions,
    CDSQuote,
    DiscountCurve,
    HazardCurve,
    bootstrap_hazard_curve,
    bootstrap_hazard_curve_cross_checked,
    price_cds,
)

ILLUSTRATIVE_QUOTES = [
    CDSQuote(1.0, 0.0045),
    CDSQuote(2.0, 0.0065),
    CDSQuote(3.0, 0.0085),
    CDSQuote(5.0, 0.0120),
    CDSQuote(7.0, 0.0145),
    CDSQuote(10.0, 0.0165),
]


def _generate_quotes_from_known_curve(knots, hazards, discount_curve, conventions):
    true_curve = HazardCurve()
    for knot, hazard in zip(knots, hazards):
        true_curve.add_segment(knot, hazard)
    return [
        CDSQuote(knot, price_cds(knot, discount_curve, true_curve, conventions).par_spread)
        for knot in knots
    ]


def test_bootstrap_recovers_known_hazard_curve_exactly():
    knots = [1.0, 3.0, 5.0, 10.0]
    true_hazards = [0.008, 0.015, 0.022, 0.030]
    discount_curve = DiscountCurve.flat(0.04)
    conventions = CDSConventions()

    quotes = _generate_quotes_from_known_curve(knots, true_hazards, discount_curve, conventions)
    result = bootstrap_hazard_curve(quotes, discount_curve, conventions)

    for recovered, expected in zip(result.curve.hazard_rates, true_hazards):
        assert recovered == pytest.approx(expected, abs=1e-8)


def test_bootstrap_calibrates_model_spreads_to_quotes_essentially_exactly():
    discount_curve = DiscountCurve(
        __import__("numpy").array([0.5, 1, 2, 3, 5, 7, 10]),
        __import__("numpy").array([0.0530, 0.0510, 0.0470, 0.0450, 0.0430, 0.0435, 0.0440]),
    )
    result = bootstrap_hazard_curve(ILLUSTRATIVE_QUOTES, discount_curve, CDSConventions())
    for implied, quote in zip(result.implied_par_spreads, ILLUSTRATIVE_QUOTES):
        assert implied == pytest.approx(quote.spread, abs=1e-9)


def test_bootstrapped_hazard_rates_are_non_negative():
    discount_curve = DiscountCurve.flat(0.045)
    result = bootstrap_hazard_curve(ILLUSTRATIVE_QUOTES, discount_curve, CDSConventions())
    assert all(h >= 0.0 for h in result.curve.hazard_rates)


def test_survival_curve_from_bootstrap_is_monotonically_decreasing():
    discount_curve = DiscountCurve.flat(0.045)
    result = bootstrap_hazard_curve(ILLUSTRATIVE_QUOTES, discount_curve, CDSConventions())
    prev = 1.0
    for t in (0.5, 1.0, 2.0, 3.0, 5.0, 7.0, 10.0, 15.0):
        s = float(result.curve.survival_probability(t))
        assert s <= prev + 1e-15
        prev = s


def test_steeper_spread_curve_implies_increasing_hazard_term_structure():
    discount_curve = DiscountCurve.flat(0.045)
    result = bootstrap_hazard_curve(ILLUSTRATIVE_QUOTES, discount_curve, CDSConventions())
    hazards = result.curve.hazard_rates
    assert all(hazards[i] > hazards[i - 1] for i in range(1, len(hazards)))


def test_brentq_and_hand_rolled_bisection_agree():
    # The core cross-validation this Python port adds over the C++ version's
    # single hand-rolled solver: two independent root-finding implementations,
    # solving the exact same bootstrap, must agree on every pillar's hazard
    # rate to within the asserted tolerance (see bootstrap_hazard_curve_cross_checked).
    discount_curve = DiscountCurve.flat(0.045)
    result = bootstrap_hazard_curve_cross_checked(ILLUSTRATIVE_QUOTES, discount_curve,
                                                   CDSConventions(), agreement_tol=1e-6)
    assert len(result.curve.hazard_rates) == len(ILLUSTRATIVE_QUOTES)


def test_bootstrap_rejects_unsorted_quotes():
    discount_curve = DiscountCurve.flat(0.04)
    unsorted_quotes = [CDSQuote(5.0, 0.01), CDSQuote(1.0, 0.005)]
    with pytest.raises(ValueError):
        bootstrap_hazard_curve(unsorted_quotes, discount_curve, CDSConventions())
