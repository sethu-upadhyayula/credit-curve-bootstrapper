// Cross-language check against the Python engine in ../python.
//
// The reference values below were produced by the Python implementation
// (bootstrap_hazard_curve and compute_cs01) on the same illustrative quotes
// and discount curve, printed at full double precision. The C++ engine must
// reproduce them to at least 9 significant figures. The Python suite holds
// the mirror-image test (python/tests/test_cross_language.py) against C++
// output.
#include <cmath>
#include <vector>

#include "creditcurve/bootstrap.hpp"
#include "creditcurve/risk.hpp"
#include "test_framework.hpp"

using namespace creditcurve;

namespace {

constexpr double kRelTol = 1e-9;  // 9 significant figures

void checkRelClose(double actual, double expected) {
    CHECK_CLOSE(actual, expected, kRelTol * std::abs(expected));
}

}  // namespace

TEST(bootstrap_and_cs01_match_python_engine_to_nine_significant_figures) {
    const DiscountCurve discountCurve({0.5, 1, 2, 3, 5, 7, 10},
                                      {0.0530, 0.0510, 0.0470, 0.0450, 0.0430, 0.0435, 0.0440});
    const std::vector<CDSQuote> quotes = {{1.0, 0.0045}, {2.0, 0.0065}, {3.0, 0.0085},
                                          {5.0, 0.0120}, {7.0, 0.0145}, {10.0, 0.0165}};
    const CDSConventions conventions;

    const std::vector<double> pythonHazardRates = {
        7.452399054737280e-03, 1.428540691964473e-02, 2.133789696214754e-02,
        3.012170088196480e-02, 3.734549663791195e-02, 3.879024929219255e-02};
    const double pythonParallelCS01 = 4.285823821676277e+03;
    const double python5YBucketCS01 = 4.286817366998263e+03;

    const BootstrapResult result = bootstrapHazardCurve(quotes, discountCurve, conventions);
    for (std::size_t i = 0; i < pythonHazardRates.size(); ++i) {
        checkRelClose(result.curve.hazardRates()[i], pythonHazardRates[i]);
    }

    const CS01Result cs01 =
        computeCS01(quotes, 10'000'000.0, 5.0, quotes[3].spread, discountCurve, conventions);
    checkRelClose(cs01.parallel, pythonParallelCS01);
    checkRelClose(cs01.bucketed[3], python5YBucketCS01);
}
