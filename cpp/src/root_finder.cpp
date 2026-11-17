#include "creditcurve/root_finder.hpp"

#include <cmath>
#include <stdexcept>

namespace creditcurve {

double brentSolve(const std::function<double(double)>& f, double a, double b, double tol,
                   int maxIter) {
    double fa = f(a), fb = f(b);
    if (fa * fb > 0.0) {
        throw std::invalid_argument("brentSolve: root is not bracketed by [a, b]");
    }
    if (std::abs(fa) < std::abs(fb)) {
        std::swap(a, b);
        std::swap(fa, fb);
    }
    double c = a, fc = fa;
    bool mflag = true;
    double d = a;  // only meaningful once mflag is false; initialized to silence warnings

    for (int iter = 0; iter < maxIter; ++iter) {
        if (fb == 0.0 || std::abs(b - a) < tol) return b;

        double s;
        if (fa != fc && fb != fc) {
            // Inverse quadratic interpolation.
            s = a * fb * fc / ((fa - fb) * (fa - fc)) + b * fa * fc / ((fb - fa) * (fb - fc)) +
                c * fa * fb / ((fc - fa) * (fc - fb));
        } else {
            // Secant method.
            s = b - fb * (b - a) / (fb - fa);
        }

        const double lo = std::min((3.0 * a + b) / 4.0, b);
        const double hi = std::max((3.0 * a + b) / 4.0, b);
        const bool conditionsFail =
            (s < lo || s > hi) ||
            (mflag && std::abs(s - b) >= std::abs(b - c) / 2.0) ||
            (!mflag && std::abs(s - b) >= std::abs(c - d) / 2.0) ||
            (mflag && std::abs(b - c) < tol) ||
            (!mflag && std::abs(c - d) < tol);

        if (conditionsFail) {
            s = (a + b) / 2.0;  // bisection fallback
            mflag = true;
        } else {
            mflag = false;
        }

        const double fs = f(s);
        d = c;
        c = b;
        fc = fb;

        if (fa * fs < 0.0) {
            b = s;
            fb = fs;
        } else {
            a = s;
            fa = fs;
        }

        if (std::abs(fa) < std::abs(fb)) {
            std::swap(a, b);
            std::swap(fa, fb);
        }
    }
    return b;  // best estimate after maxIter iterations
}

}  // namespace creditcurve
