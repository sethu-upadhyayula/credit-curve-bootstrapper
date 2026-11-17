#pragma once

#include <vector>

#include "creditcurve/discount_curve.hpp"
#include "creditcurve/hazard_curve.hpp"
#include "creditcurve/types.hpp"

namespace creditcurve {

struct BootstrapResult {
    HazardCurve curve;
    std::vector<double> impliedParSpreads;  // model par spread per pillar, post-calibration
    std::vector<int> iterationsUsed;        // Brent iterations per segment (diagnostic)
};

// Bootstraps a piecewise-constant hazard curve from a term structure of CDS
// quotes, sorted ascending by tenor. Standard sequential bootstrap: solve for
// the hazard rate of segment k, holding all earlier segments fixed, such that
// the model par spread at tenor k matches the quoted market spread exactly.
//
// Par spread is monotonically increasing in the new segment's hazard rate (more
// default risk => wider break-even spread), so each segment has a unique root,
// found via Brent's method on the bracket [minHazard, maxHazard].
BootstrapResult bootstrapHazardCurve(const std::vector<CDSQuote>& quotes,
                                      const DiscountCurve& discountCurve,
                                      const CDSConventions& conventions,
                                      double minHazard = 1e-8, double maxHazard = 5.0,
                                      double rootTol = 1e-12);

}  // namespace creditcurve
