#include "creditcurve/bootstrap.hpp"

#include <algorithm>
#include <stdexcept>

#include "creditcurve/cds_pricer.hpp"
#include "creditcurve/root_finder.hpp"

namespace creditcurve {

BootstrapResult bootstrapHazardCurve(const std::vector<CDSQuote>& quotes,
                                      const DiscountCurve& discountCurve,
                                      const CDSConventions& conventions, double minHazard,
                                      double maxHazard, double rootTol) {
    if (quotes.empty()) throw std::invalid_argument("bootstrapHazardCurve: no quotes supplied");
    if (!std::is_sorted(quotes.begin(), quotes.end(),
                         [](const CDSQuote& a, const CDSQuote& b) { return a.tenorYears < b.tenorYears; })) {
        throw std::invalid_argument("bootstrapHazardCurve: quotes must be sorted ascending by tenor");
    }

    BootstrapResult result;
    for (const auto& quote : quotes) {
        // Objective: with all prior segments frozen in `result.curve`, find the
        // hazard rate of the new segment [prevKnot, quote.tenorYears] that makes
        // the model par spread equal the quoted market spread.
        auto objective = [&](double candidateHazard) {
            HazardCurve trial = result.curve;  // copy: earlier segments unchanged
            trial.addSegment(quote.tenorYears, candidateHazard);
            const CDSLegs legs = priceCDS(quote.tenorYears, discountCurve, trial, conventions);
            return legs.parSpread - quote.spread;
        };

        const double h = brentSolve(objective, minHazard, maxHazard, rootTol);
        result.curve.addSegment(quote.tenorYears, h);

        const CDSLegs calibratedLegs =
            priceCDS(quote.tenorYears, discountCurve, result.curve, conventions);
        result.impliedParSpreads.push_back(calibratedLegs.parSpread);
        result.iterationsUsed.push_back(0);  // brentSolve doesn't currently report iteration count
    }
    return result;
}

}  // namespace creditcurve
