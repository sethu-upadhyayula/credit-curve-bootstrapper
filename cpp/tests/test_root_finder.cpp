#include <cmath>
#include <stdexcept>

#include "creditcurve/root_finder.hpp"
#include "test_framework.hpp"

using namespace creditcurve;

TEST(brent_finds_sqrt_two) {
    const double root = brentSolve([](double x) { return x * x - 2.0; }, 0.0, 2.0);
    CHECK_CLOSE(root, std::sqrt(2.0), 1e-9);
}

TEST(brent_finds_known_cubic_root) {
    // x^3 - x - 2 = 0 has one real root, approx 1.5213797068.
    const double root = brentSolve([](double x) { return x * x * x - x - 2.0; }, 1.0, 2.0);
    CHECK_CLOSE(root, 1.5213797068, 1e-8);
}

TEST(brent_handles_root_at_bracket_endpoint) {
    const double root = brentSolve([](double x) { return x - 1.0; }, 1.0, 3.0);
    CHECK_CLOSE(root, 1.0, 1e-9);
}

TEST(brent_throws_when_root_not_bracketed) {
    bool threw = false;
    try {
        brentSolve([](double x) { return x * x + 1.0; }, -1.0, 1.0);  // never crosses zero
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}
