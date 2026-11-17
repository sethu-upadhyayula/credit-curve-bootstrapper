#include "creditcurve/hazard_curve.hpp"

#include <cmath>
#include <stdexcept>

namespace creditcurve {

void HazardCurve::addSegment(double knotTime, double hazardRate) {
    const double prevKnot = knotTimes_.empty() ? 0.0 : knotTimes_.back();
    if (knotTime <= prevKnot) {
        throw std::invalid_argument("HazardCurve::addSegment: knot times must be strictly increasing");
    }
    if (hazardRate < 0.0) {
        throw std::invalid_argument("HazardCurve::addSegment: hazard rate must be non-negative");
    }
    knotTimes_.push_back(knotTime);
    hazardRates_.push_back(hazardRate);
}

double HazardCurve::survivalProbability(double t) const {
    if (t <= 0.0) return 1.0;
    double cumulativeHazard = 0.0;
    double segStart = 0.0;
    for (std::size_t i = 0; i < knotTimes_.size(); ++i) {
        const double segEnd = knotTimes_[i];
        if (t <= segEnd) {
            cumulativeHazard += hazardRates_[i] * (t - segStart);
            return std::exp(-cumulativeHazard);
        }
        cumulativeHazard += hazardRates_[i] * (segEnd - segStart);
        segStart = segEnd;
    }
    // t is beyond the last knot: flat-extrapolate the final segment's hazard rate.
    const double lastHazard = hazardRates_.empty() ? 0.0 : hazardRates_.back();
    cumulativeHazard += lastHazard * (t - segStart);
    return std::exp(-cumulativeHazard);
}

double HazardCurve::hazardRateAt(double t) const {
    if (knotTimes_.empty()) return 0.0;
    for (std::size_t i = 0; i < knotTimes_.size(); ++i) {
        if (t <= knotTimes_[i]) return hazardRates_[i];
    }
    return hazardRates_.back();
}

}  // namespace creditcurve
