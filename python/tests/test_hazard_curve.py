import numpy as np
import pytest

from creditcurve import HazardCurve


def test_survival_probability_at_zero_is_one():
    curve = HazardCurve()
    curve.add_segment(5.0, 0.02)
    assert curve.survival_probability(0.0) == pytest.approx(1.0, abs=1e-12)


def test_single_segment_matches_closed_form_exponential():
    h = 0.025
    curve = HazardCurve()
    curve.add_segment(10.0, h)
    t = np.array([0.5, 1.0, 3.0, 7.0, 10.0])
    np.testing.assert_allclose(curve.survival_probability(t), np.exp(-h * t), atol=1e-12)


def test_two_segments_matches_manual_piecewise_integral():
    curve = HazardCurve()
    curve.add_segment(2.0, 0.01)
    curve.add_segment(5.0, 0.03)
    assert curve.survival_probability(3.0) == pytest.approx(np.exp(-0.05), abs=1e-12)
    assert curve.survival_probability(2.0) == pytest.approx(np.exp(-0.02), abs=1e-12)


def test_survival_probability_is_monotonically_non_increasing():
    curve = HazardCurve()
    curve.add_segment(1.0, 0.01)
    curve.add_segment(3.0, 0.04)
    curve.add_segment(7.0, 0.02)
    ts = np.array([0.1, 0.5, 1.0, 2.0, 3.0, 5.0, 7.0, 12.0])
    survivals = curve.survival_probability(ts)
    assert np.all(np.diff(survivals) <= 1e-15)


def test_vectorized_matches_elementwise_scalar_calls():
    curve = HazardCurve()
    curve.add_segment(1.0, 0.01)
    curve.add_segment(3.0, 0.04)
    curve.add_segment(7.0, 0.02)
    ts = np.array([0.2, 0.9, 1.5, 3.0, 4.4, 6.9, 9.0])
    vectorized = curve.survival_probability(ts)
    elementwise = np.array([float(curve.survival_probability(t)) for t in ts])
    np.testing.assert_allclose(vectorized, elementwise, atol=1e-14)


def test_negative_hazard_rate_is_rejected():
    curve = HazardCurve()
    with pytest.raises(ValueError):
        curve.add_segment(1.0, -0.01)


def test_non_increasing_knot_time_is_rejected():
    curve = HazardCurve()
    curve.add_segment(5.0, 0.02)
    with pytest.raises(ValueError):
        curve.add_segment(3.0, 0.02)
