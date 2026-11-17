#pragma once

#include "creditcurve/discount_curve.hpp"
#include "creditcurve/hazard_curve.hpp"
#include "creditcurve/types.hpp"

namespace creditcurve {

struct CDSLegs {
    double riskyAnnuity;     // PV of the premium leg per unit of spread, per unit notional
    double protectionLegPV;  // PV of the protection leg, per unit notional
    double parSpread;        // protectionLegPV / riskyAnnuity, the break-even spread
};

// Prices the premium and protection legs of a single-name CDS to maturityYears,
// given a discount curve, a (candidate or calibrated) hazard curve, and
// contract conventions.
//
// Premium leg: PV = sum over quarterly payment dates of
//   dt * S(t_i) * DF(t_i)                      [regular premium]
//   + accrualFraction * dt * (S(t_{i-1}) - S(t_i)) * DF(t_i)   [accrued-on-default]
//
// Protection leg: PV = (1 - R) * \int_0^T DF(t) * (-dS(t))
// evaluated by fine numerical integration (200 steps/year) since the
// zero-rate curve is not piecewise-flat, so no simpler closed form applies.
CDSLegs priceCDS(double maturityYears, const DiscountCurve& discountCurve,
                  const HazardCurve& hazardCurve, const CDSConventions& conventions);

// Mark-to-market of an existing CDS position (protection buyer's perspective):
//   PV = notional * (protectionLegPV - contractSpread * riskyAnnuity)
// Positive PV means the position has gained value to the protection buyer
// (market spreads have widened since the contract was struck).
double markToMarket(double notional, double contractSpread, double maturityYears,
                     const DiscountCurve& discountCurve, const HazardCurve& hazardCurve,
                     const CDSConventions& conventions);

}  // namespace creditcurve
