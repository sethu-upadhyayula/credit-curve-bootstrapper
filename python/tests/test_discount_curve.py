import numpy as np
import pytest

from creditcurve import DiscountCurve


def test_discount_factor_at_zero_is_one():
    curve = DiscountCurve(np.array([1.0, 5.0, 10.0]), np.array([0.03, 0.035, 0.04]))
    assert curve.discount_factor(0.0) == pytest.approx(1.0, abs=1e-12)


def test_flat_curve_matches_closed_form_exponential():
    r = 0.045
    curve = DiscountCurve.flat(r)
    t = np.array([0.5, 1.0, 3.0, 7.0, 15.0])
    np.testing.assert_allclose(curve.discount_factor(t), np.exp(-r * t), atol=1e-10)


def test_discount_factor_exact_at_pillars():
    curve = DiscountCurve(np.array([1.0, 5.0, 10.0]), np.array([0.03, 0.035, 0.04]))
    assert curve.discount_factor(1.0) == pytest.approx(np.exp(-0.03 * 1.0), abs=1e-12)
    assert curve.discount_factor(5.0) == pytest.approx(np.exp(-0.035 * 5.0), abs=1e-12)
    assert curve.discount_factor(10.0) == pytest.approx(np.exp(-0.04 * 10.0), abs=1e-12)


def test_zero_rate_interpolates_linearly_between_pillars():
    curve = DiscountCurve(np.array([1.0, 5.0]), np.array([0.03, 0.05]))
    assert curve.zero_rate(3.0) == pytest.approx(0.04, abs=1e-12)


def test_flat_extrapolation_beyond_pillars():
    curve = DiscountCurve(np.array([1.0, 5.0, 10.0]), np.array([0.03, 0.035, 0.04]))
    assert curve.zero_rate(20.0) == pytest.approx(0.04, abs=1e-12)
    assert curve.zero_rate(0.1) == pytest.approx(0.03, abs=1e-12)


def test_vectorized_call_matches_elementwise_scalar_calls():
    curve = DiscountCurve(np.array([1.0, 5.0, 10.0]), np.array([0.03, 0.035, 0.04]))
    ts = np.array([0.2, 1.0, 2.5, 5.0, 8.0, 12.0])
    vectorized = curve.discount_factor(ts)
    elementwise = np.array([float(curve.discount_factor(t)) for t in ts])
    np.testing.assert_allclose(vectorized, elementwise, atol=1e-14)


def test_rejects_non_increasing_pillars():
    with pytest.raises(ValueError):
        DiscountCurve(np.array([5.0, 1.0]), np.array([0.03, 0.04]))
