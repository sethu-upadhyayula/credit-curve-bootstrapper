"""Runs the full bootstrap + risk pipeline on an illustrative (synthetic,
clearly-labeled-as-such) single-name CDS curve and writes results/*.csv +
results/summary.txt for the plotting script and README to consume.
"""
import pathlib

import numpy as np

from creditcurve import (
    CDSConventions,
    CDSQuote,
    DiscountCurve,
    HazardCurve,
    bootstrap_hazard_curve_cross_checked,
    compute_cs01,
    price_cds,
)

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "results"
OUT.mkdir(exist_ok=True)

DISCOUNT_CURVE = DiscountCurve(
    np.array([0.5, 1, 2, 3, 5, 7, 10]),
    np.array([0.0530, 0.0510, 0.0470, 0.0450, 0.0430, 0.0435, 0.0440]),
)
QUOTES = [
    CDSQuote(1.0, 0.0045),
    CDSQuote(2.0, 0.0065),
    CDSQuote(3.0, 0.0085),
    CDSQuote(5.0, 0.0120),
    CDSQuote(7.0, 0.0145),
    CDSQuote(10.0, 0.0165),
]
CONVENTIONS = CDSConventions()


def main():
    result = bootstrap_hazard_curve_cross_checked(QUOTES, DISCOUNT_CURVE, CONVENTIONS)

    with open(OUT / "hazard_curve.csv", "w") as f:
        f.write("tenor_years,quoted_spread_bp,hazard_rate_pct,survival_probability\n")
        for quote, hazard in zip(QUOTES, result.curve.hazard_rates):
            survival = float(result.curve.survival_probability(quote.tenor_years))
            f.write(f"{quote.tenor_years},{quote.spread * 1e4},{hazard * 100.0},{survival}\n")

    with open(OUT / "calibration_check.csv", "w") as f:
        f.write("tenor_years,model_spread_bp,quoted_spread_bp,error_bp\n")
        max_abs_error = 0.0
        for quote, implied in zip(QUOTES, result.implied_par_spreads):
            error_bp = (implied - quote.spread) * 1e4
            max_abs_error = max(max_abs_error, abs(error_bp))
            f.write(f"{quote.tenor_years},{implied * 1e4},{quote.spread * 1e4},{error_bp}\n")

    notional = 10_000_000.0
    contract_spread = QUOTES[3].spread  # 5Y, struck at par
    cs01 = compute_cs01(QUOTES, notional, 5.0, contract_spread, DISCOUNT_CURVE, CONVENTIONS)
    sum_bucketed = sum(cs01.bucketed)
    with open(OUT / "cs01.csv", "w") as f:
        f.write("tenor_years,cs01_usd_per_bp\n")
        for quote, bucket in zip(QUOTES, cs01.bucketed):
            f.write(f"{quote.tenor_years},{bucket}\n")

    # Naive single flat hazard, calibrated to the 5Y quote only.
    from scipy.optimize import brentq

    def flat_objective(h):
        trial = HazardCurve()
        trial.add_segment(30.0, h)
        return price_cds(5.0, DISCOUNT_CURVE, trial, CONVENTIONS).par_spread - QUOTES[3].spread

    flat_hazard = brentq(flat_objective, 1e-8, 5.0, xtol=1e-12)
    flat_curve = HazardCurve()
    flat_curve.add_segment(30.0, flat_hazard)

    with open(OUT / "naive_flat_comparison.csv", "w") as f:
        f.write("tenor_years,flat_implied_bp,quoted_bp,miss_bp\n")
        for quote in QUOTES:
            flat_spread = price_cds(quote.tenor_years, DISCOUNT_CURVE, flat_curve, CONVENTIONS).par_spread
            f.write(f"{quote.tenor_years},{flat_spread * 1e4},{quote.spread * 1e4},"
                    f"{(flat_spread - quote.spread) * 1e4}\n")

    with open(OUT / "summary.txt", "w") as f:
        f.write("Credit curve bootstrap: illustrative single-name CDS term structure\n")
        f.write(f"Pillars (years): {[q.tenor_years for q in QUOTES]}\n")
        f.write(f"Quoted spreads (bp): {[q.spread * 1e4 for q in QUOTES]}\n")
        f.write(f"Bootstrapped hazard rates (%): "
                f"{[round(h * 100, 4) for h in result.curve.hazard_rates]}\n")
        f.write(f"Max abs calibration error: {max_abs_error:.3e} bp "
                "(brentq bootstrap, cross-checked against hand-rolled bisection to 1e-6)\n\n")
        f.write(f"CS01 ($10mm 5Y protection buyer, struck at par {contract_spread * 1e4:.2f}bp):\n")
        for quote, bucket in zip(QUOTES, cs01.bucketed):
            f.write(f"  {quote.tenor_years:>5.1f}Y: ${bucket:>10.2f} per 1bp\n")
        f.write(f"  sum of buckets: ${sum_bucketed:.2f}\n")
        f.write(f"  parallel CS01:  ${cs01.parallel:.2f}  "
                f"(diff {sum_bucketed - cs01.parallel:.2f}, "
                f"{100 * (sum_bucketed - cs01.parallel) / cs01.parallel:.3f}% of parallel)\n\n")
        f.write(f"Naive flat hazard (matches 5Y only): {flat_hazard * 100:.4f}%\n")
        f.write("  Mispricing at every other tenor (bp): see naive_flat_comparison.csv\n")

    print("Wrote results/hazard_curve.csv, calibration_check.csv, cs01.csv, "
          "naive_flat_comparison.csv, summary.txt")


if __name__ == "__main__":
    main()
