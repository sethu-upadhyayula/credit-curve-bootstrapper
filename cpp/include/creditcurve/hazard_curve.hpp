#pragma once

#include <vector>

namespace creditcurve {

// Piecewise-constant hazard-rate (default-intensity) curve, the standard
// representation for a bootstrapped credit curve. Segment i covers the
// interval (knotTimes[i-1], knotTimes[i]] with knotTimes[-1] := 0, and carries
// a constant hazard rate hazardRates[i].
//
// Survival probability follows directly from the piecewise-constant hazard:
//   S(t) = exp( -\int_0^t h(s) ds )
// which, for piecewise-constant h, is a product of per-segment exponentials.
class HazardCurve {
public:
    HazardCurve() = default;

    // Appends a new segment ending at knotTime with constant hazard hazardRate.
    // knotTime must be strictly greater than the current last knot (or > 0 for
    // the first segment). hazardRate must be >= 0 (a negative hazard rate would
    // imply an arbitrage / non-monotonic survival curve).
    void addSegment(double knotTime, double hazardRate);

    double survivalProbability(double t) const;
    double hazardRateAt(double t) const;

    const std::vector<double>& knotTimes() const { return knotTimes_; }
    const std::vector<double>& hazardRates() const { return hazardRates_; }

private:
    std::vector<double> knotTimes_;
    std::vector<double> hazardRates_;
};

}  // namespace creditcurve
