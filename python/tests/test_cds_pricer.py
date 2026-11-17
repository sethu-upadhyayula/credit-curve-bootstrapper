import pytest

from creditcurve import CDSConventions, DiscountCurve, HazardCurve, mark_to_market, price_cds


def test_flat_hazard_par_spread_matches_credit_triangle_approximation():
    # See cpp/tests/test_cds_pricer.cpp for the derivation: in the continuous-
    # payment limit, par spread -> (1 - recovery) * hazard exactly, independent
    # of the discount curve. Discretization error measured at ~0.38% in
    # development, so tested here at a 1% tolerance.
    h, recovery = 0.02, 0.4
    discount_curve = DiscountCurve.flat(0.03)
    hazard_curve = HazardCurve()
    hazard_curve.add_segment(30.0, h)
    expected_continuous = (1.0 - recovery) * h

    for maturity in (1.0, 3.0, 5.0, 10.0):
        legs = price_cds(maturity, discount_curve, hazard_curve, CDSConventions())
        rel_error = abs(legs.par_spread - expected_continuous) / expected_continuous
        assert rel_error < 0.01


def test_par_spread_increases_with_hazard_rate():
    discount_curve = DiscountCurve.flat(0.04)
    prev_spread = 0.0
    for h in (0.005, 0.01, 0.02, 0.05, 0.10):
        hazard_curve = HazardCurve()
        hazard_curve.add_segment(5.0, h)
        spread = price_cds(5.0, discount_curve, hazard_curve, CDSConventions()).par_spread
        assert spread > prev_spread
        prev_spread = spread


def test_par_spread_increases_with_loss_given_default():
    discount_curve = DiscountCurve.flat(0.04)
    hazard_curve = HazardCurve()
    hazard_curve.add_segment(5.0, 0.02)
    high_recovery = CDSConventions(recovery_rate=0.60)
    low_recovery = CDSConventions(recovery_rate=0.20)
    spread_high_r = price_cds(5.0, discount_curve, hazard_curve, high_recovery).par_spread
    spread_low_r = price_cds(5.0, discount_curve, hazard_curve, low_recovery).par_spread
    assert spread_low_r > spread_high_r


def test_mark_to_market_is_zero_at_the_par_spread():
    discount_curve = DiscountCurve.flat(0.035)
    hazard_curve = HazardCurve()
    hazard_curve.add_segment(5.0, 0.018)
    legs = price_cds(5.0, discount_curve, hazard_curve, CDSConventions())
    pv = mark_to_market(10_000_000.0, legs.par_spread, 5.0, discount_curve, hazard_curve,
                         CDSConventions())
    assert pv == pytest.approx(0.0, abs=1e-4)


def test_protection_buyer_gains_when_market_spreads_widen():
    discount_curve = DiscountCurve.flat(0.035)
    low_hazard_at_trade = HazardCurve()
    low_hazard_at_trade.add_segment(5.0, 0.01)
    contract_spread = price_cds(5.0, discount_curve, low_hazard_at_trade, CDSConventions()).par_spread

    widened_hazard = HazardCurve()
    widened_hazard.add_segment(5.0, 0.03)
    pv = mark_to_market(10_000_000.0, contract_spread, 5.0, discount_curve, widened_hazard,
                         CDSConventions())
    assert pv > 0.0


def test_rejects_non_positive_maturity():
    discount_curve = DiscountCurve.flat(0.03)
    hazard_curve = HazardCurve()
    hazard_curve.add_segment(5.0, 0.02)
    with pytest.raises(ValueError):
        price_cds(0.0, discount_curve, hazard_curve, CDSConventions())
