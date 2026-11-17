#include <cmath>

#include "creditcurve/discount_curve.hpp"
#include "test_framework.hpp"

using namespace creditcurve;

TEST(discount_factor_at_zero_is_one) {
    DiscountCurve curve({1, 5, 10}, {0.03, 0.035, 0.04});
    CHECK_CLOSE(curve.discountFactor(0.0), 1.0, 1e-12);
}

TEST(flat_curve_matches_closed_form_exponential) {
    const double r = 0.045;
    DiscountCurve curve = DiscountCurve::flat(r);
    for (double t : {0.5, 1.0, 3.0, 7.0, 15.0}) {
        CHECK_CLOSE(curve.discountFactor(t), std::exp(-r * t), 1e-10);
    }
}

TEST(discount_factor_exact_at_pillars) {
    DiscountCurve curve({1, 5, 10}, {0.03, 0.035, 0.04});
    CHECK_CLOSE(curve.discountFactor(1.0), std::exp(-0.03 * 1.0), 1e-12);
    CHECK_CLOSE(curve.discountFactor(5.0), std::exp(-0.035 * 5.0), 1e-12);
    CHECK_CLOSE(curve.discountFactor(10.0), std::exp(-0.04 * 10.0), 1e-12);
}

TEST(zero_rate_interpolates_linearly_between_pillars) {
    DiscountCurve curve({1, 5}, {0.03, 0.05});
    // Midpoint (t=3) should get the average zero rate under linear interpolation.
    CHECK_CLOSE(curve.zeroRate(3.0), 0.04, 1e-12);
    CHECK_CLOSE(curve.discountFactor(3.0), std::exp(-0.04 * 3.0), 1e-12);
}

TEST(flat_extrapolation_beyond_pillars) {
    DiscountCurve curve({1, 5, 10}, {0.03, 0.035, 0.04});
    CHECK_CLOSE(curve.zeroRate(20.0), 0.04, 1e-12);
    CHECK_CLOSE(curve.zeroRate(0.1), 0.03, 1e-12);
}

TEST(discount_factor_strictly_decreasing_in_time) {
    DiscountCurve curve({1, 5, 10}, {0.03, 0.035, 0.04});
    double prev = curve.discountFactor(0.1);
    for (double t : {0.5, 1.0, 2.0, 5.0, 8.0, 10.0, 15.0}) {
        const double df = curve.discountFactor(t);
        CHECK(df < prev);
        prev = df;
    }
}
