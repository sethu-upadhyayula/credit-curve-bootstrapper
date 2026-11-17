#pragma once

#include <functional>

namespace creditcurve {

// Brent's method root finder (Numerical Recipes / Brent 1973 formulation):
// combines bisection with secant and inverse-quadratic-interpolation steps,
// falling back to bisection whenever the faster step would leave the bracket
// or fails to converge quickly enough. Guaranteed to converge given a valid
// bracket [a, b] with f(a) and f(b) of opposite sign.
//
// Implemented directly to avoid a dependency on an external root-finding
// library.
double brentSolve(const std::function<double(double)>& f, double a, double b,
                   double tol = 1e-12, int maxIter = 100);

}  // namespace creditcurve
