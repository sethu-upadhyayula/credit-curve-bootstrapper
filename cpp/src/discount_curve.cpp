#include "creditcurve/discount_curve.hpp"

#include <cmath>
#include <stdexcept>

namespace creditcurve {

DiscountCurve::DiscountCurve(std::vector<double> pillarTimes, std::vector<double> zeroRates)
    : times_(std::move(pillarTimes)), rates_(std::move(zeroRates)) {
    if (times_.empty() || times_.size() != rates_.size()) {
        throw std::invalid_argument("DiscountCurve: pillarTimes/zeroRates size mismatch or empty");
    }
    for (std::size_t i = 0; i < times_.size(); ++i) {
        if (times_[i] <= 0.0) throw std::invalid_argument("DiscountCurve: pillar times must be > 0");
        if (i > 0 && times_[i] <= times_[i - 1]) {
            throw std::invalid_argument("DiscountCurve: pillar times must be strictly increasing");
        }
    }
}

DiscountCurve DiscountCurve::flat(double rate, double maxTenorYears) {
    // Two identical pillars so interpolation degenerates to a constant rate
    // everywhere in [0, maxTenorYears] (and flat-extrapolated beyond it).
    return DiscountCurve({maxTenorYears * 0.5, maxTenorYears}, {rate, rate});
}

double DiscountCurve::zeroRate(double t) const {
    if (t <= times_.front()) return rates_.front();
    if (t >= times_.back()) return rates_.back();
    for (std::size_t i = 1; i < times_.size(); ++i) {
        if (t <= times_[i]) {
            const double t0 = times_[i - 1], t1 = times_[i];
            const double r0 = rates_[i - 1], r1 = rates_[i];
            const double w = (t - t0) / (t1 - t0);
            return r0 + w * (r1 - r0);
        }
    }
    return rates_.back();  // unreachable given the checks above
}

double DiscountCurve::discountFactor(double t) const {
    if (t <= 0.0) return 1.0;
    return std::exp(-zeroRate(t) * t);
}

}  // namespace creditcurve
