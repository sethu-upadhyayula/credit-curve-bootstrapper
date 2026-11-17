#include <cmath>

#include "creditcurve/cds_pricer.hpp"
#include "test_framework.hpp"

using namespace creditcurve;

// "Credit triangle" identity: in the continuous-payment limit, the par CDS
// spread for a FLAT hazard curve equals (1 - recovery) * hazard, independent
// of the discount curve and maturity (the discounting cancels between legs).
// Our quarterly-payment-with-accrued-on-default approximation should track
// this closely; verified numerically in Python during development to have
// ~0.38% relative discretization error at these parameters, so we test at 1%.
TEST(flat_hazard_par_spread_matches_credit_triangle_approximation) {
    const double h = 0.02, R = 0.4;
    DiscountCurve discountCurve = DiscountCurve::flat(0.03);
    HazardCurve hazardCurve;
    hazardCurve.addSegment(30.0, h);
    const double expectedContinuous = (1.0 - R) * h;

    for (double T : {1.0, 3.0, 5.0, 10.0}) {
        const CDSLegs legs = priceCDS(T, discountCurve, hazardCurve, CDSConventions{});
        const double relError = std::abs(legs.parSpread - expectedContinuous) / expectedContinuous;
        CHECK(relError < 0.01);
    }
}

TEST(credit_triangle_error_is_insensitive_to_discount_rate) {
    // The continuous-limit derivation shows discounting cancels between legs
    // for a flat hazard curve; confirm the par spread barely moves with r.
    const double h = 0.015, R = 0.4;
    HazardCurve hazardCurve;
    hazardCurve.addSegment(30.0, h);
    double parAtZeroRate =
        priceCDS(5.0, DiscountCurve::flat(1e-6), hazardCurve, CDSConventions{}).parSpread;
    double parAtFivePercent =
        priceCDS(5.0, DiscountCurve::flat(0.05), hazardCurve, CDSConventions{}).parSpread;
    CHECK(std::abs(parAtZeroRate - parAtFivePercent) / parAtZeroRate < 0.01);
}

TEST(par_spread_increases_with_hazard_rate) {
    DiscountCurve discountCurve = DiscountCurve::flat(0.04);
    double prevSpread = 0.0;
    for (double h : {0.005, 0.01, 0.02, 0.05, 0.10}) {
        HazardCurve hazardCurve;
        hazardCurve.addSegment(5.0, h);
        const double spread = priceCDS(5.0, discountCurve, hazardCurve, CDSConventions{}).parSpread;
        CHECK(spread > prevSpread);
        prevSpread = spread;
    }
}

TEST(par_spread_increases_with_loss_given_default) {
    // Higher LGD (lower recovery) means protection is worth more per unit of
    // default probability, so the fair spread should rise as recovery falls.
    DiscountCurve discountCurve = DiscountCurve::flat(0.04);
    HazardCurve hazardCurve;
    hazardCurve.addSegment(5.0, 0.02);
    CDSConventions highRecovery;
    highRecovery.recoveryRate = 0.60;
    CDSConventions lowRecovery;
    lowRecovery.recoveryRate = 0.20;
    const double spreadHighR = priceCDS(5.0, discountCurve, hazardCurve, highRecovery).parSpread;
    const double spreadLowR = priceCDS(5.0, discountCurve, hazardCurve, lowRecovery).parSpread;
    CHECK(spreadLowR > spreadHighR);
}

TEST(protection_leg_pv_is_zero_for_zero_hazard) {
    DiscountCurve discountCurve = DiscountCurve::flat(0.04);
    HazardCurve hazardCurve;
    hazardCurve.addSegment(5.0, 1e-9);  // effectively riskless
    const CDSLegs legs = priceCDS(5.0, discountCurve, hazardCurve, CDSConventions{});
    CHECK(legs.protectionLegPV < 1e-6);
    CHECK(legs.parSpread < 1e-6);
}

TEST(mark_to_market_is_zero_at_the_par_spread) {
    DiscountCurve discountCurve = DiscountCurve::flat(0.035);
    HazardCurve hazardCurve;
    hazardCurve.addSegment(5.0, 0.018);
    const CDSLegs legs = priceCDS(5.0, discountCurve, hazardCurve, CDSConventions{});
    const double pv = markToMarket(10'000'000.0, legs.parSpread, 5.0, discountCurve, hazardCurve,
                                    CDSConventions{});
    CHECK_CLOSE(pv, 0.0, 1e-4);
}

TEST(protection_buyer_gains_when_market_spreads_widen) {
    // Buy protection at a low contract spread; if hazard (and hence the fair
    // par spread) later rises, the buyer's position should show positive PV.
    DiscountCurve discountCurve = DiscountCurve::flat(0.035);
    HazardCurve lowHazardAtTrade;
    lowHazardAtTrade.addSegment(5.0, 0.01);
    const double contractSpread =
        priceCDS(5.0, discountCurve, lowHazardAtTrade, CDSConventions{}).parSpread;

    HazardCurve widenedHazard;
    widenedHazard.addSegment(5.0, 0.03);  // credit deteriorated
    const double pv = markToMarket(10'000'000.0, contractSpread, 5.0, discountCurve, widenedHazard,
                                    CDSConventions{});
    CHECK(pv > 0.0);
}
