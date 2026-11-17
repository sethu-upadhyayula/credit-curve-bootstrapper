"""A hand-rolled bisection root-finder, used purely as an independent
cross-check against scipy.optimize.brentq in bootstrap.py / the test suite.

The C++ implementation uses its own Brent's method (cpp/src/root_finder.cpp)
as the primary solver. In Python, scipy.optimize.brentq is the primary solver
and this bisection exists only to verify it independently.
"""
from __future__ import annotations

from typing import Callable


def bisection_solve(
    f: Callable[[float], float], a: float, b: float, tol: float = 1e-10, max_iter: int = 200
) -> float:
    fa, fb = f(a), f(b)
    if fa * fb > 0.0:
        raise ValueError("bisection_solve: root is not bracketed by [a, b]")
    for _ in range(max_iter):
        mid = 0.5 * (a + b)
        fmid = f(mid)
        if abs(fmid) < tol or 0.5 * (b - a) < tol:
            return mid
        if fa * fmid < 0.0:
            b, fb = mid, fmid
        else:
            a, fa = mid, fmid
    return 0.5 * (a + b)
