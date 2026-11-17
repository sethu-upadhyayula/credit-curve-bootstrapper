#pragma once

#include <vector>

#include "creditcurve/discount_curve.hpp"
#include "creditcurve/types.hpp"

namespace creditcurve {

struct CS01Result {
    std::vector<double> bucketed;  // CS01 per pillar (currency per 1bp bump of that pillar's quote)
    double parallel;                // CS01 from bumping every pillar by 1bp simultaneously
};

// Bucketed and parallel CS01 ("credit spread 01") for a reference CDS position:
// a protection buyer, `notional` face, contract spread `contractSpread`, maturing
// at `positionMaturityYears`. For each pillar, bumps that pillar's market quote
// by `bumpBps` basis points, rebootstraps the whole curve, and reprices the
// position; CS01 is the resulting PV change. The parallel figure bumps every
// pillar simultaneously. Bucketed CS01s summed should approximately equal the
// parallel CS01 for small bumps, an additivity check reported in the tests.
CS01Result computeCS01(const std::vector<CDSQuote>& baseQuotes, double notional,
                        double positionMaturityYears, double contractSpread,
                        const DiscountCurve& discountCurve, const CDSConventions& conventions,
                        double bumpBps = 1.0);

}  // namespace creditcurve
