#include "creditcurve/cds_pricer.hpp"

#include <cmath>
#include <stdexcept>

namespace creditcurve {

CDSLegs priceCDS(double maturityYears, const DiscountCurve& discountCurve,
                  const HazardCurve& hazardCurve, const CDSConventions& conventions) {
    if (maturityYears <= 0.0) throw std::invalid_argument("priceCDS: maturity must be > 0");

    // --- Premium leg: quarterly (or conventions.paymentsPerYear) payment schedule ---
    const int nPayments =
        static_cast<int>(std::lround(maturityYears * conventions.paymentsPerYear));
    const double dt = 1.0 / conventions.paymentsPerYear;

    double riskyAnnuity = 0.0;
    double prevQ = 1.0;
    for (int i = 1; i <= nPayments; ++i) {
        const double t = i * dt;
        const double q = hazardCurve.survivalProbability(t);
        const double df = discountCurve.discountFactor(t);
        riskyAnnuity += dt * q * df;                                            // regular premium
        riskyAnnuity += conventions.accrualFraction * dt * (prevQ - q) * df;    // accrued-on-default
        prevQ = q;
    }

    // --- Protection leg: fine numerical integration of DF(t) * (-dS(t)) ---
    // The zero-rate curve is only piecewise-linear (not piecewise-flat), so
    // there is no simple closed form; 200 steps/year keeps discretization error
    // far below typical bid-ask spread precision (sub-0.01bp, see tests).
    constexpr int kStepsPerYear = 200;
    const int totalSteps = static_cast<int>(std::lround(maturityYears * kStepsPerYear));
    const double stepDt = maturityYears / totalSteps;

    double protectionLegPV = 0.0;
    double qPrev = 1.0;
    for (int i = 1; i <= totalSteps; ++i) {
        const double t = i * stepDt;
        const double qCur = hazardCurve.survivalProbability(t);
        const double tMid = t - 0.5 * stepDt;
        const double dfMid = discountCurve.discountFactor(tMid);
        protectionLegPV += dfMid * (qPrev - qCur);
        qPrev = qCur;
    }
    protectionLegPV *= (1.0 - conventions.recoveryRate);

    const double parSpread = protectionLegPV / riskyAnnuity;
    return {riskyAnnuity, protectionLegPV, parSpread};
}

double markToMarket(double notional, double contractSpread, double maturityYears,
                     const DiscountCurve& discountCurve, const HazardCurve& hazardCurve,
                     const CDSConventions& conventions) {
    const CDSLegs legs = priceCDS(maturityYears, discountCurve, hazardCurve, conventions);
    return notional * (legs.protectionLegPV - contractSpread * legs.riskyAnnuity);
}

}  // namespace creditcurve
