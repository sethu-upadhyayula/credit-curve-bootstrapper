// CLI driver: bootstraps a hazard-rate / survival-probability curve from a
// synthetic (illustrative, not live-market) term structure of CDS spreads for
// a hypothetical BBB-rated single name, prices the calibration, computes
// bucketed CS01 risk, and shows why a single flat hazard rate under-fits the
// term structure that a proper bootstrap captures exactly.
#include <cstdio>
#include <vector>

#include "creditcurve/bootstrap.hpp"
#include "creditcurve/cds_pricer.hpp"
#include "creditcurve/risk.hpp"
#include "creditcurve/root_finder.hpp"
#include "creditcurve/types.hpp"

using namespace creditcurve;

namespace {

DiscountCurve buildIllustrativeDiscountCurve() {
    // Illustrative USD OIS-style zero curve (mildly inverted short end,
    // consistent with a higher-for-longer rate environment). Not live market
    // data - a fixed, documented set of inputs for reproducibility.
    return DiscountCurve({0.5, 1, 2, 3, 5, 7, 10},
                          {0.0530, 0.0510, 0.0470, 0.0450, 0.0430, 0.0435, 0.0440});
}

std::vector<CDSQuote> buildIllustrativeCDSQuotes() {
    // Illustrative upward-sloping investment-grade single-name CDS curve
    // (spreads in decimal; 0.0045 = 45bp). Term structure shape is typical of
    // a BBB credit: short-dated protection is cheap, long-dated compensates
    // for cumulative default risk.
    return {{1.0, 0.0045}, {2.0, 0.0065}, {3.0, 0.0085},
            {5.0, 0.0120}, {7.0, 0.0145}, {10.0, 0.0165}};
}

}  // namespace

int main() {
    const DiscountCurve discountCurve = buildIllustrativeDiscountCurve();
    const std::vector<CDSQuote> quotes = buildIllustrativeCDSQuotes();
    const CDSConventions conventions;

    std::printf("=== Credit curve bootstrap (illustrative single-name CDS quotes) ===\n\n");
    std::printf("%-8s %-14s %-18s %-18s\n", "Tenor", "Mkt spread(bp)", "Hazard rate(%)",
                "Survival prob");

    const BootstrapResult bootstrap = bootstrapHazardCurve(quotes, discountCurve, conventions);
    for (std::size_t i = 0; i < quotes.size(); ++i) {
        const double survival = bootstrap.curve.survivalProbability(quotes[i].tenorYears);
        std::printf("%-8.1f %-14.2f %-18.4f %-18.6f\n", quotes[i].tenorYears,
                    quotes[i].spread * 1e4, bootstrap.curve.hazardRates()[i] * 100.0, survival);
    }

    std::printf("\n--- Calibration check: model par spread vs. quoted spread ---\n");
    double maxAbsErrorBps = 0.0;
    for (std::size_t i = 0; i < quotes.size(); ++i) {
        const double errBps = (bootstrap.impliedParSpreads[i] - quotes[i].spread) * 1e4;
        maxAbsErrorBps = std::max(maxAbsErrorBps, std::abs(errBps));
        std::printf("  %5.1fY: model=%.6f bp  quoted=%.2f bp  error=%.2e bp\n",
                    quotes[i].tenorYears, bootstrap.impliedParSpreads[i] * 1e4,
                    quotes[i].spread * 1e4, errBps);
    }
    std::printf("  max abs calibration error: %.2e bp\n", maxAbsErrorBps);

    // --- CS01 risk on a 5Y protection-buyer position ---
    const double notional = 10'000'000.0;
    const double contractSpread = quotes[3].spread;  // struck at the 5Y par spread (index 3 = 5.0Y)
    const CS01Result cs01 =
        computeCS01(quotes, notional, 5.0, contractSpread, discountCurve, conventions);

    std::printf("\n--- CS01: $10mm 5Y protection buyer, struck at par (%.2f bp) ---\n",
                contractSpread * 1e4);
    double sumBucketed = 0.0;
    for (std::size_t i = 0; i < quotes.size(); ++i) {
        std::printf("  bucket %5.1fY: $%9.2f per 1bp\n", quotes[i].tenorYears, cs01.bucketed[i]);
        sumBucketed += cs01.bucketed[i];
    }
    std::printf("  sum of buckets:   $%9.2f\n", sumBucketed);
    std::printf("  parallel CS01:    $%9.2f  (difference: $%.2f, %.3f%% of parallel)\n",
                cs01.parallel, sumBucketed - cs01.parallel,
                100.0 * (sumBucketed - cs01.parallel) / cs01.parallel);

    // --- Why bootstrap instead of a single flat hazard rate? ---
    std::printf("\n--- Naive single flat hazard (calibrated to the 5Y quote only) ---\n");
    auto flatObjective = [&](double h) {
        HazardCurve trial;
        trial.addSegment(30.0, h);  // one segment covering the whole curve
        return priceCDS(5.0, discountCurve, trial, conventions).parSpread - quotes[3].spread;
    };
    const double flatHazard = brentSolve(flatObjective, 1e-8, 5.0);
    HazardCurve flatCurve;
    flatCurve.addSegment(30.0, flatHazard);
    std::printf("  flat hazard rate: %.4f%% (matches the 5Y quote by construction)\n",
                flatHazard * 100.0);
    std::printf("  %-8s %-16s %-16s %-10s\n", "Tenor", "Flat-implied(bp)", "Quoted(bp)", "Miss(bp)");
    for (const auto& q : quotes) {
        const double flatSpread = priceCDS(q.tenorYears, discountCurve, flatCurve, conventions).parSpread;
        std::printf("  %-8.1f %-16.2f %-16.2f %-10.2f\n", q.tenorYears, flatSpread * 1e4,
                    q.spread * 1e4, (flatSpread - q.spread) * 1e4);
    }
    std::printf(
        "\n  A single flat hazard rate can only match one point on the curve by\n"
        "  construction; it misprices every other tenor. The bootstrap above\n"
        "  matches all %zu quoted tenors simultaneously (max error %.2e bp).\n",
        quotes.size(), maxAbsErrorBps);

    return 0;
}
