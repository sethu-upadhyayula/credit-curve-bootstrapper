# Credit Curve Bootstrapper (C++)

The C++23 implementation. The methodology, results and key findings are in `../README.md`. This file covers the C++ code layout and test suite.

## Layout

```
include/creditcurve/
  types.hpp            CDSQuote, CDSConventions
  discount_curve.hpp   Log-linear zero-rate curve with flat extrapolation
  hazard_curve.hpp     Piecewise-constant hazard rate and survival probability
  root_finder.hpp      Brent's method
  cds_pricer.hpp       Premium and protection leg pricing, mark-to-market
  bootstrap.hpp        Sequential hazard-curve bootstrap from CDS quotes
  risk.hpp             Bucketed and parallel CS01
src/                   Implementations
apps/main.cpp          CLI: bootstraps an illustrative curve and prints hazard rates,
                       the calibration check, CS01 and a flat-hazard comparison
tests/                 33 tests using a custom TEST/CHECK/CHECK_CLOSE framework (tests/test_framework.hpp)
```

## Build and run

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
./run_tests
./bootstrap_cli
```

Requires CMake 3.16 or later and a C++23 compiler. There are no external dependencies: the root finder and linear interpolation are implemented directly.

## Test suite (33 tests)

- `test_discount_curve.cpp` (6): interpolation, extrapolation and exact values at pillars
- `test_hazard_curve.cpp` (7): closed-form single- and multi-segment survival probabilities, monotonicity and input validation
- `test_root_finder.cpp` (4): Brent's method against known roots (√2 and a cubic with a known real root) and rejection of unbracketed roots
- `test_cds_pricer.cpp` (7): the continuous-limit credit triangle (par spread to `(1-R)×hazard`), monotonicity in hazard and loss given default, and mark-to-market algebra
- `test_bootstrap.cpp` (5): round-trip recovery of a known hazard curve from generated quotes, exact calibration to quoted spreads, and non-negativity and monotonicity of the recovered curve
- `test_risk.cpp` (3): CS01 sign and the structural zero described in the root README's "Key findings"
- `test_cross_language.cpp` (1): hazard rates and CS01 match the Python engine's full-precision output to at least 9 significant figures

## An assumption corrected by the tests

The first version of `test_risk.cpp` asserted that every bucketed CS01 for an at-the-money protection buyer should be strictly positive, on the intuition that wider spreads anywhere should benefit a protection buyer. The test failed immediately: every bucket except the position's own maturity came back as exactly zero. Working through the algebra showed the zero is structural. For a position struck at its own tenor's par spread, `MTM` factors as `riskyAnnuity × (parSpread(T) − contractSpread)`, and the bootstrap guarantees that `parSpread(T)` reproduces the unbumped quote for T regardless of other pillars. The test now asserts the correct behavior as `cs01_is_exactly_zero_for_every_pillar_except_the_positions_own_tenor`, with the derivation in a comment.
