import math

import pytest

from creditcurve import bisection_solve


def test_bisection_finds_sqrt_two():
    root = bisection_solve(lambda x: x * x - 2.0, 0.0, 2.0, tol=1e-12)
    assert root == pytest.approx(math.sqrt(2.0), abs=1e-8)


def test_bisection_finds_known_cubic_root():
    root = bisection_solve(lambda x: x**3 - x - 2.0, 1.0, 2.0, tol=1e-12)
    assert root == pytest.approx(1.5213797068, abs=1e-6)


def test_bisection_raises_when_root_not_bracketed():
    with pytest.raises(ValueError):
        bisection_solve(lambda x: x * x + 1.0, -1.0, 1.0)
