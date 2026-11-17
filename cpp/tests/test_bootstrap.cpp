#include <cmath>
#include <vector>

#include "creditcurve/bootstrap.hpp"
#include "creditcurve/cds_pricer.hpp"
#include "test_framework.hpp"

using namespace creditcurve;

namespace {

// Generates "market" CDS quotes by pricing CDS at each tenor off a KNOWN
// piecewise-constant hazard curve. Bootstrapping from those quotes should
// recover exactly the same hazard curve, which makes a strong round-trip
// check.
std::vector<CDSQuote> generateQuotesFromKnownCurve(const std::vector<double>& knots,
                                                     const std::vector<double>& hazards,
                                                     const DiscountCurve& discountCurve,
                                                     const CDSConventions& conventions) {
    HazardCurve trueCurve;
    for (std::size_t i = 0; i < knots.size(); ++i) trueCurve.addSegment(knots[i], hazards[i]);

    std::vector<CDSQuote> quotes;
    for (double knot : knots) {
        const double spread = priceCDS(knot, discountCurve, trueCurve, conventions).parSpread;
        quotes.push_back({knot, spread});
    }
    return quotes;
}

}  // namespace

TEST(bootstrap_recovers_known_hazard_curve_exactly) {
    const std::vector<double> knots = {1.0, 3.0, 5.0, 10.0};
    const std::vector<double> trueHazards = {0.008, 0.015, 0.022, 0.030};
    DiscountCurve discountCurve = DiscountCurve::flat(0.04);
    CDSConventions conventions;

    const auto quotes = generateQuotesFromKnownCurve(knots, trueHazards, discountCurve, conventions);
    const BootstrapResult result = bootstrapHazardCurve(quotes, discountCurve, conventions);

    for (std::size_t i = 0; i < trueHazards.size(); ++i) {
        CHECK_CLOSE(result.curve.hazardRates()[i], trueHazards[i], 1e-8);
    }
}

TEST(bootstrap_calibrates_model_spreads_to_quotes_essentially_exactly) {
    const std::vector<CDSQuote> quotes = {{1.0, 0.0045}, {2.0, 0.0065}, {3.0, 0.0085},
                                           {5.0, 0.0120}, {7.0, 0.0145}, {10.0, 0.0165}};
    DiscountCurve discountCurve({0.5, 1, 2, 3, 5, 7, 10},
                                 {0.0530, 0.0510, 0.0470, 0.0450, 0.0430, 0.0435, 0.0440});
    CDSConventions conventions;

    const BootstrapResult result = bootstrapHazardCurve(quotes, discountCurve, conventions);
    for (std::size_t i = 0; i < quotes.size(); ++i) {
        CHECK_CLOSE(result.impliedParSpreads[i], quotes[i].spread, 1e-9);
    }
}

TEST(bootstrapped_hazard_rates_are_non_negative) {
    const std::vector<CDSQuote> quotes = {{1.0, 0.0045}, {2.0, 0.0065}, {3.0, 0.0085},
                                           {5.0, 0.0120}, {7.0, 0.0145}, {10.0, 0.0165}};
    DiscountCurve discountCurve = DiscountCurve::flat(0.045);
    const BootstrapResult result = bootstrapHazardCurve(quotes, discountCurve, CDSConventions{});
    for (double h : result.curve.hazardRates()) CHECK(h >= 0.0);
}

TEST(survival_curve_from_bootstrap_is_monotonically_decreasing) {
    const std::vector<CDSQuote> quotes = {{1.0, 0.0045}, {2.0, 0.0065}, {3.0, 0.0085},
                                           {5.0, 0.0120}, {7.0, 0.0145}, {10.0, 0.0165}};
    DiscountCurve discountCurve = DiscountCurve::flat(0.045);
    const BootstrapResult result = bootstrapHazardCurve(quotes, discountCurve, CDSConventions{});

    double prev = 1.0;
    for (double t : {0.5, 1.0, 2.0, 3.0, 5.0, 7.0, 10.0, 15.0}) {
        const double s = result.curve.survivalProbability(t);
        CHECK(s <= prev + 1e-15);
        prev = s;
    }
}

TEST(steeper_spread_curve_implies_increasing_hazard_term_structure) {
    // An upward-sloping CDS curve (typical investment-grade shape) should
    // bootstrap to an increasing hazard-rate term structure.
    const std::vector<CDSQuote> quotes = {{1.0, 0.0045}, {2.0, 0.0065}, {3.0, 0.0085},
                                           {5.0, 0.0120}, {7.0, 0.0145}, {10.0, 0.0165}};
    DiscountCurve discountCurve = DiscountCurve::flat(0.045);
    const BootstrapResult result = bootstrapHazardCurve(quotes, discountCurve, CDSConventions{});

    const auto& hazards = result.curve.hazardRates();
    for (std::size_t i = 1; i < hazards.size(); ++i) CHECK(hazards[i] > hazards[i - 1]);
}
