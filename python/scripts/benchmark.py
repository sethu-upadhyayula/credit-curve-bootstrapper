"""Benchmarks the vectorized (NumPy array) CDS pricer against a naive
pure-Python loop implementation to quantify the vectorization speedup.
"""
import math
import time

from creditcurve import CDSConventions, DiscountCurve, HazardCurve, price_cds


def naive_python_price_cds(maturity_years, discount_curve, hazard_curve, conventions):
    """Element-by-element re-implementation of price_cds, using Python
    scalars and a `for` loop instead of NumPy array operations, the
    Python-loop analogue of what the C++ engine does natively."""
    n_payments = round(maturity_years * conventions.payments_per_year)
    dt = 1.0 / conventions.payments_per_year

    risky_annuity = 0.0
    prev_q = 1.0
    for i in range(1, n_payments + 1):
        t = i * dt
        q = float(hazard_curve.survival_probability(t))
        df = float(discount_curve.discount_factor(t))
        risky_annuity += dt * q * df
        risky_annuity += conventions.accrual_fraction * dt * (prev_q - q) * df
        prev_q = q

    steps_per_year = 200
    total_steps = round(maturity_years * steps_per_year)
    step_dt = maturity_years / total_steps
    protection_leg_pv = 0.0
    q_prev = 1.0
    for i in range(1, total_steps + 1):
        t = i * step_dt
        q_cur = float(hazard_curve.survival_probability(t))
        t_mid = t - 0.5 * step_dt
        df_mid = float(discount_curve.discount_factor(t_mid))
        protection_leg_pv += df_mid * (q_prev - q_cur)
        q_prev = q_cur
    protection_leg_pv *= (1.0 - conventions.recovery_rate)

    return protection_leg_pv / risky_annuity


def main():
    discount_curve = DiscountCurve.flat(0.045)
    hazard_curve = HazardCurve()
    hazard_curve.add_segment(30.0, 0.02)
    conventions = CDSConventions()

    n_reps = 200

    start = time.perf_counter()
    for _ in range(n_reps):
        price_cds(5.0, discount_curve, hazard_curve, conventions)
    vectorized_time = time.perf_counter() - start

    start = time.perf_counter()
    for _ in range(n_reps):
        naive_python_price_cds(5.0, discount_curve, hazard_curve, conventions)
    naive_time = time.perf_counter() - start

    vectorized_par = price_cds(5.0, discount_curve, hazard_curve, conventions).par_spread
    naive_par = naive_python_price_cds(5.0, discount_curve, hazard_curve, conventions)
    assert math.isclose(vectorized_par, naive_par, rel_tol=1e-9), "vectorized/naive results diverge"

    print(f"{n_reps} full CDS pricings (5Y, 200 protection-leg steps/year):")
    print(f"  vectorized (NumPy):  {vectorized_time:.4f}s total, {vectorized_time / n_reps * 1e3:.4f}ms/call")
    print(f"  naive (Python loop): {naive_time:.4f}s total, {naive_time / n_reps * 1e3:.4f}ms/call")
    print(f"  speedup: {naive_time / vectorized_time:.1f}x")
    print(f"  results agree: par spread {vectorized_par * 1e4:.6f}bp (vectorized) vs "
          f"{naive_par * 1e4:.6f}bp (naive)")


if __name__ == "__main__":
    main()
