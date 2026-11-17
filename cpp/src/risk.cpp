#include "creditcurve/risk.hpp"

#include "creditcurve/bootstrap.hpp"
#include "creditcurve/cds_pricer.hpp"

namespace creditcurve {

namespace {

double priceWithQuotes(const std::vector<CDSQuote>& quotes, double notional,
                        double positionMaturityYears, double contractSpread,
                        const DiscountCurve& discountCurve, const CDSConventions& conventions) {
    const BootstrapResult calibration = bootstrapHazardCurve(quotes, discountCurve, conventions);
    return markToMarket(notional, contractSpread, positionMaturityYears, discountCurve,
                         calibration.curve, conventions);
}

}  // namespace

CS01Result computeCS01(const std::vector<CDSQuote>& baseQuotes, double notional,
                        double positionMaturityYears, double contractSpread,
                        const DiscountCurve& discountCurve, const CDSConventions& conventions,
                        double bumpBps) {
    const double bump = bumpBps / 10000.0;
    const double basePV = priceWithQuotes(baseQuotes, notional, positionMaturityYears,
                                           contractSpread, discountCurve, conventions);

    CS01Result result;
    result.bucketed.reserve(baseQuotes.size());
    for (std::size_t i = 0; i < baseQuotes.size(); ++i) {
        std::vector<CDSQuote> bumped = baseQuotes;
        bumped[i].spread += bump;
        const double bumpedPV = priceWithQuotes(bumped, notional, positionMaturityYears,
                                                 contractSpread, discountCurve, conventions);
        result.bucketed.push_back(bumpedPV - basePV);
    }

    std::vector<CDSQuote> parallelBumped = baseQuotes;
    for (auto& q : parallelBumped) q.spread += bump;
    const double parallelPV = priceWithQuotes(parallelBumped, notional, positionMaturityYears,
                                               contractSpread, discountCurve, conventions);
    result.parallel = parallelPV - basePV;

    return result;
}

}  // namespace creditcurve
