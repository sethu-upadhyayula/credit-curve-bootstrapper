"""Cross-language check against the C++ engine in ../cpp.

The reference values below were produced by the C++ implementation
(bootstrapHazardCurve and computeCS01) on the same illustrative quotes and
discount curve, printed at full double precision. The Python engine must
reproduce them to at least 9 significant figures. The C++ suite holds the
mirror-image test (cpp/tests/test_cross_language.cpp) against Python output.
"""
import numpy as np
import pytest

from creditcurve import (
    CDSConventions,
    CDSQuote,
    DiscountCurve,
    bootstrap_hazard_curve,
    compute_cs01,
)

DISCOUNT_CURVE = DiscountCurve(
    np.array([0.5, 1, 2, 3, 5, 7, 10]),
    np.array([0.0530, 0.0510, 0.0470, 0.0450, 0.0430, 0.0435, 0.0440]),
)
QUOTES = [
    CDSQuote(1.0, 0.0045),
    CDSQuote(2.0, 0.0065),
    CDSQuote(3.0, 0.0085),
    CDSQuote(5.0, 0.0120),
    CDSQuote(7.0, 0.0145),
    CDSQuote(10.0, 0.0165),
]

CPP_HAZARD_RATES = [
    7.452399054737648e-03,
    1.428540691971082e-02,
    2.133789696207719e-02,
    3.012170088184083e-02,
    3.734549663805337e-02,
    3.879024929219266e-02,
]
CPP_PARALLEL_CS01 = 4.285823821616117e03
CPP_5Y_BUCKET_CS01 = 4.286817366828608e03

REL_TOL = 1e-9  # 9 significant figures


def test_bootstrap_and_cs01_match_cpp_engine_to_nine_significant_figures():
    conventions = CDSConventions()
    result = bootstrap_hazard_curve(QUOTES, DISCOUNT_CURVE, conventions)
    for python_value, cpp_value in zip(result.curve.hazard_rates, CPP_HAZARD_RATES):
        assert python_value == pytest.approx(cpp_value, rel=REL_TOL)

    cs01 = compute_cs01(QUOTES, 10_000_000.0, 5.0, QUOTES[3].spread, DISCOUNT_CURVE, conventions)
    assert cs01.parallel == pytest.approx(CPP_PARALLEL_CS01, rel=REL_TOL)
    assert cs01.bucketed[3] == pytest.approx(CPP_5Y_BUCKET_CS01, rel=REL_TOL)
