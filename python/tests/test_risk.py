import pytest

from creditcurve import CDSConventions, CDSQuote, DiscountCurve, compute_cs01

ILLUSTRATIVE_QUOTES = [
    CDSQuote(1.0, 0.0045),
    CDSQuote(2.0, 0.0065),
    CDSQuote(3.0, 0.0085),
    CDSQuote(5.0, 0.0120),
    CDSQuote(7.0, 0.0145),
    CDSQuote(10.0, 0.0165),
]


def test_cs01_is_exactly_zero_for_every_pillar_except_the_positions_own_tenor():
    # Structural identity (see cpp/tests/test_risk.cpp for the full algebraic
    # argument): a position struck exactly at the current par spread for its
    # own maturity has MTM = notional * riskyAnnuity * (parSpread(T) - contractSpread).
    # The sequential bootstrap guarantees parSpread(T) always reproduces T's
    # own (unbumped) quote exactly, regardless of what happens to any other
    # pillar's hazard, so every bucket except the position's own tenor must be
    # zero to near machine precision, not merely small.
    discount_curve = DiscountCurve.flat(0.045)
    contract_spread = ILLUSTRATIVE_QUOTES[3].spread  # 5Y par spread
    cs01 = compute_cs01(ILLUSTRATIVE_QUOTES, 10_000_000.0, 5.0, contract_spread, discount_curve,
                         CDSConventions())
    for i, bucket in enumerate(cs01.bucketed):
        if i == 3:
            assert bucket > 1.0
        else:
            assert abs(bucket) < 1e-3


def test_cs01_dominant_bucket_is_the_positions_own_tenor_for_an_off_market_position():
    discount_curve = DiscountCurve.flat(0.045)
    contract_spread = 0.0080  # struck away from the current ~120bp par level
    cs01 = compute_cs01(ILLUSTRATIVE_QUOTES, 10_000_000.0, 5.0, contract_spread, discount_curve,
                         CDSConventions())
    assert cs01.parallel > 0.0
    own_tenor_magnitude = abs(cs01.bucketed[3])
    for i, bucket in enumerate(cs01.bucketed):
        if i == 3:
            continue
        assert own_tenor_magnitude > abs(bucket)
    assert cs01.bucketed[4] == pytest.approx(0.0, abs=1e-6)  # 7Y: beyond position maturity
    assert cs01.bucketed[5] == pytest.approx(0.0, abs=1e-6)  # 10Y: beyond position maturity


def test_sum_of_bucketed_cs01_approximately_equals_parallel_cs01():
    discount_curve = DiscountCurve.flat(0.045)
    contract_spread = ILLUSTRATIVE_QUOTES[3].spread
    cs01 = compute_cs01(ILLUSTRATIVE_QUOTES, 10_000_000.0, 5.0, contract_spread, discount_curve,
                         CDSConventions())
    sum_bucketed = sum(cs01.bucketed)
    rel_diff = abs(sum_bucketed - cs01.parallel) / abs(cs01.parallel)
    assert rel_diff < 0.01
