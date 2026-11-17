#include <cmath>
#include <vector>

#include "creditcurve/risk.hpp"
#include "test_framework.hpp"

using namespace creditcurve;

namespace {
std::vector<CDSQuote> illustrativeQuotes() {
    return {{1.0, 0.0045}, {2.0, 0.0065}, {3.0, 0.0085}, {5.0, 0.0120}, {7.0, 0.0145}, {10.0, 0.0165}};
}
}  // namespace

TEST(cs01_dominant_bucket_is_the_positions_own_tenor_for_an_off_market_position) {
    // A position struck away from current par (bought protection historically
    // at a tighter 80bp, now marking against a curve implying ~120bp at 5Y) is
    // already in-the-money; its overall (parallel) CS01 should be positive
    // (spreads widening further helps the protection buyer), and, even though
    // this position is no longer exactly at par, so the clean zero-elsewhere
    // identity from the test below doesn't strictly apply, the position's own
    // 5Y bucket should still dominate every other bucket in magnitude, and any
    // pillar beyond the position's 5Y maturity should remain exactly zero,
    // since those hazard segments never enter a 5Y position's pricing at all.
    DiscountCurve discountCurve = DiscountCurve::flat(0.045);
    const auto quotes = illustrativeQuotes();
    const double contractSpread = 0.0080;  // struck away from the current ~120bp par level
    const CS01Result cs01 = computeCS01(quotes, 10'000'000.0, 5.0, contractSpread, discountCurve,
                                         CDSConventions{});
    CHECK(cs01.parallel > 0.0);
    const double ownTenorMagnitude = std::abs(cs01.bucketed[3]);  // index 3 = 5.0Y
    for (std::size_t i = 0; i < cs01.bucketed.size(); ++i) {
        if (i == 3) continue;
        CHECK(ownTenorMagnitude > std::abs(cs01.bucketed[i]));
    }
    CHECK_CLOSE(cs01.bucketed[4], 0.0, 1e-6);  // 7Y: beyond the position's own maturity
    CHECK_CLOSE(cs01.bucketed[5], 0.0, 1e-6);  // 10Y: beyond the position's own maturity
}

TEST(sum_of_bucketed_cs01_approximately_equals_parallel_cs01) {
    // Additivity check: for small (1bp) bumps, summing the per-pillar bucketed
    // sensitivities should closely match a single simultaneous parallel bump,
    // since the pricing function is only mildly non-linear in spread over 1bp.
    DiscountCurve discountCurve = DiscountCurve::flat(0.045);
    const auto quotes = illustrativeQuotes();
    const double contractSpread = quotes[3].spread;
    const CS01Result cs01 = computeCS01(quotes, 10'000'000.0, 5.0, contractSpread, discountCurve,
                                         CDSConventions{});
    double sumBucketed = 0.0;
    for (double b : cs01.bucketed) sumBucketed += b;
    const double relDiff = std::abs(sumBucketed - cs01.parallel) / std::abs(cs01.parallel);
    CHECK(relDiff < 0.01);
}

TEST(cs01_is_exactly_zero_for_every_pillar_except_the_positions_own_tenor) {
    // A position struck exactly at the market par spread for its own maturity
    // has an algebraic identity: MTM = notional * (parSpread(T) - contractSpread)
    // * riskyAnnuity(T), and the sequential bootstrap guarantees the model par
    // spread at T always reproduces T's own quote exactly, however the *other*
    // pillars' hazards move underneath it. So bumping any pillar OTHER than the
    // position's own maturity should leave its MTM unchanged to near machine
    // precision, only the 5Y bucket itself should carry any risk. This is a
    // non-obvious, structural consequence of sequential bootstrapping, not an
    // approximation, so it's tested at a tight tolerance.
    DiscountCurve discountCurve = DiscountCurve::flat(0.045);
    const auto quotes = illustrativeQuotes();
    const double contractSpread = quotes[3].spread;  // 5Y par spread (index 3 = 5.0Y)
    const CS01Result cs01 = computeCS01(quotes, 10'000'000.0, 5.0, contractSpread, discountCurve,
                                         CDSConventions{});
    for (std::size_t i = 0; i < quotes.size(); ++i) {
        if (i == 3) {
            CHECK(cs01.bucketed[i] > 1.0);  // the position's own tenor carries real risk
        } else {
            CHECK(std::abs(cs01.bucketed[i]) < 1e-3);  // every other pillar: ~zero, not just small
        }
    }
}
