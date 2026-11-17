# Credit Curve Bootstrapper (Python)

The Python implementation, built on NumPy and SciPy, of the engine in `../cpp`. The methodology, results and key findings are in `../README.md`. This file covers the Python code layout, the vectorization benchmark and the cross-checks specific to this implementation.

## Layout

```
creditcurve/
  types.py            CDSQuote, CDSConventions (frozen dataclasses)
  discount_curve.py   Log-linear zero-rate curve via numpy.interp
  hazard_curve.py     Piecewise-constant hazard rate and survival probability,
                      vectorized over arrays of query times
  root_finder.py      Hand-written bisection, used to cross-check scipy.optimize.brentq
  cds_pricer.py       Premium and protection leg pricing with NumPy array operations
  bootstrap.py        Sequential bootstrap via scipy.optimize.brentq, plus
                      bootstrap_hazard_curve_cross_checked() for the
                      brentq vs. bisection agreement check
  risk.py             Bucketed and parallel CS01
tests/                34 pytest tests
scripts/
  generate_results.py   Runs the pipeline and writes results/*.csv and summary.txt
  plot_results.py       Renders results/*.png
  benchmark.py          Vectorized vs. naive Python loop timing
```

## Install and run

```bash
pip install -e ".[dev]"
python3 -m pytest -q
python3 scripts/generate_results.py
python3 scripts/plot_results.py
python3 scripts/benchmark.py
```

## Differences from the C++ implementation

The C++ engine implements its own Brent's method to avoid adding a numerical library dependency. The Python engine uses `scipy.optimize.brentq` as its primary solver, and the hand-written bisection in `root_finder.py` serves only as an independent check. `bootstrap_hazard_curve_cross_checked()` runs both and asserts they agree to `1e-6` in hazard-rate units on every pillar, which `test_brentq_and_hand_rolled_bisection_agree` verifies.

Leg pricing follows the same pattern. `cds_pricer.py` prices both legs with vectorized NumPy operations (one `np.arange` and array arithmetic per leg), where the C++ version loops explicitly over the payment schedule and integration grid.

## Vectorization speedup

Measured with `scripts/benchmark.py` over 200 full CDS pricings (5Y maturity, quarterly premiums, 200 protection-leg integration steps per year) in one run:

| | Time (200 calls) | Per call |
|---|---|---|
| Vectorized (NumPy) | 0.028 s | 0.14 ms |
| Naive Python loop | 7.32 s | 36.6 ms |

The vectorized pricer was 264x faster in this run, and consistently over 250x across runs, with the exact figure depending on hardware. Both implementations produce the same par spread (120.677852 bp), and the script asserts this with `math.isclose`.

The speedup is large even though each call runs only a few hundred integration steps, because each step in the naive version is a separate Python call into `hazard_curve.survival_probability()` and `discount_curve.discount_factor()`.

## What is not vectorized

`compute_cs01` in `risk.py` loops over pillars in plain Python and fully re-bootstraps the curve for each bucket. Vectorization does not apply here, because each re-bootstrap is sequential: the hazard rate for segment k depends on segments `0..k-1`, so there is no array operation to batch across buckets.

## Cross-language validation

`tests/test_cross_language.py` bootstraps the same illustrative quotes and discount curve as the C++ CLI and asserts that the hazard rates and CS01 figures match the C++ engine's full-precision output to at least 9 significant figures. The C++ suite holds the mirror-image test. For example, the 5Y hazard rate is `3.0121700882%` in both engines.
