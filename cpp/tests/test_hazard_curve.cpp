#include <cmath>
#include <stdexcept>

#include "creditcurve/hazard_curve.hpp"
#include "test_framework.hpp"

using namespace creditcurve;

TEST(survival_probability_at_zero_is_one) {
    HazardCurve curve;
    curve.addSegment(5.0, 0.02);
    CHECK_CLOSE(curve.survivalProbability(0.0), 1.0, 1e-12);
}

TEST(single_segment_matches_closed_form_exponential) {
    HazardCurve curve;
    const double h = 0.025;
    curve.addSegment(10.0, h);
    for (double t : {0.5, 1.0, 3.0, 7.0, 10.0}) {
        CHECK_CLOSE(curve.survivalProbability(t), std::exp(-h * t), 1e-12);
    }
}

TEST(two_segments_matches_manual_piecewise_integral) {
    HazardCurve curve;
    curve.addSegment(2.0, 0.01);  // hazard 1% on (0, 2]
    curve.addSegment(5.0, 0.03);  // hazard 3% on (2, 5]
    // At t=3: cumulative hazard = 0.01*2 + 0.03*1 = 0.05
    CHECK_CLOSE(curve.survivalProbability(3.0), std::exp(-0.05), 1e-12);
    // At the first knot exactly:
    CHECK_CLOSE(curve.survivalProbability(2.0), std::exp(-0.02), 1e-12);
}

TEST(survival_probability_is_monotonically_non_increasing) {
    HazardCurve curve;
    curve.addSegment(1.0, 0.01);
    curve.addSegment(3.0, 0.04);
    curve.addSegment(7.0, 0.02);
    double prev = 1.0;
    for (double t : {0.1, 0.5, 1.0, 2.0, 3.0, 5.0, 7.0, 12.0}) {
        const double s = curve.survivalProbability(t);
        CHECK(s <= prev + 1e-15);
        prev = s;
    }
}

TEST(extrapolation_beyond_last_knot_uses_final_hazard_rate) {
    HazardCurve curve;
    curve.addSegment(2.0, 0.01);
    curve.addSegment(5.0, 0.03);
    const double sAt5 = curve.survivalProbability(5.0);
    const double sAt7 = curve.survivalProbability(7.0);
    CHECK_CLOSE(sAt7, sAt5 * std::exp(-0.03 * 2.0), 1e-12);
}

TEST(negative_hazard_rate_is_rejected) {
    HazardCurve curve;
    bool threw = false;
    try {
        curve.addSegment(1.0, -0.01);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

TEST(non_increasing_knot_time_is_rejected) {
    HazardCurve curve;
    curve.addSegment(5.0, 0.02);
    bool threw = false;
    try {
        curve.addSegment(3.0, 0.02);  // 3.0 < previous knot 5.0
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}
