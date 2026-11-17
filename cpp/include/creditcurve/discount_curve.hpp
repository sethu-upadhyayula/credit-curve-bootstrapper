#pragma once

#include <vector>

namespace creditcurve {

// Continuously-compounded zero-rate curve. The zero rate z(t) is linearly
// interpolated between pillar times (flat-extrapolated beyond the first and
// last pillar), and the discount factor is DF(t) = exp(-z(t) * t).
//
// This is a standard, simple curve construction, good enough to isolate the
// credit-curve bootstrap (the actual subject of this library) from rate-curve
// construction details.
class DiscountCurve {
public:
    // pillarTimes must be strictly increasing, with pillarTimes[0] > 0.
    // zeroRates[i] is the continuously-compounded zero rate to pillarTimes[i].
    DiscountCurve(std::vector<double> pillarTimes, std::vector<double> zeroRates);

    static DiscountCurve flat(double rate, double maxTenorYears = 30.0);

    double zeroRate(double t) const;
    double discountFactor(double t) const;

private:
    std::vector<double> times_;
    std::vector<double> rates_;
};

}  // namespace creditcurve
