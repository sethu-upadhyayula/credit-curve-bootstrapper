"""Renders charts from results/*.csv (written by scripts/generate_results.py).

Run after scripts/generate_results.py:
    python3 scripts/plot_results.py
"""
import pathlib

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from creditcurve import CDSConventions, CDSQuote, DiscountCurve, bootstrap_hazard_curve_cross_checked

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "results"

BLUE = "#2a78d6"
ORANGE = "#eb6834"
AQUA = "#1baf7a"
TEXT_PRIMARY = "#0b0b0b"
TEXT_SECONDARY = "#52514e"
GRID = "#e4e3df"
SURFACE = "#fcfcfb"

plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "axes.edgecolor": GRID,
    "axes.labelcolor": TEXT_PRIMARY, "text.color": TEXT_PRIMARY,
    "xtick.color": TEXT_SECONDARY, "ytick.color": TEXT_SECONDARY,
    "font.size": 11, "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.8,
})

DISCOUNT_CURVE = DiscountCurve(
    np.array([0.5, 1, 2, 3, 5, 7, 10]),
    np.array([0.0530, 0.0510, 0.0470, 0.0450, 0.0430, 0.0435, 0.0440]),
)
QUOTES = [
    CDSQuote(1.0, 0.0045), CDSQuote(2.0, 0.0065), CDSQuote(3.0, 0.0085),
    CDSQuote(5.0, 0.0120), CDSQuote(7.0, 0.0145), CDSQuote(10.0, 0.0165),
]


def plot_hazard_and_survival():
    result = bootstrap_hazard_curve_cross_checked(QUOTES, DISCOUNT_CURVE, CDSConventions())
    tenors = [q.tenor_years for q in QUOTES]
    hazards = [h * 100 for h in result.curve.hazard_rates]

    t_grid = np.linspace(0.01, 10.5, 400)
    survival = result.curve.survival_probability(t_grid)

    fig, axes = plt.subplots(1, 2, figsize=(11, 4.2))

    axes[0].step(tenors, hazards, where="post", color=BLUE, linewidth=2)
    axes[0].scatter(tenors, hazards, color=BLUE, zorder=3, s=28)
    axes[0].set_xlabel("Tenor (years)")
    axes[0].set_ylabel("Bootstrapped hazard rate (%)")
    axes[0].set_title("Piecewise-constant hazard term structure", loc="left", fontsize=11)
    axes[0].spines[["top", "right"]].set_visible(False)

    axes[1].plot(t_grid, survival, color=BLUE, linewidth=2)
    axes[1].scatter(tenors, result.curve.survival_probability(np.array(tenors)), color=BLUE,
                     zorder=3, s=28)
    axes[1].set_xlabel("Time (years)")
    axes[1].set_ylabel("Survival probability")
    axes[1].set_ylim(0.7, 1.01)
    axes[1].set_title("Implied survival probability curve", loc="left", fontsize=11)
    axes[1].spines[["top", "right"]].set_visible(False)

    fig.suptitle("Bootstrapped credit curve (illustrative single-name CDS quotes)", x=0.01,
                 ha="left", fontsize=12)
    fig.tight_layout()
    fig.savefig(OUT / "hazard_and_survival.png", dpi=160)
    plt.close(fig)


def plot_bootstrap_vs_naive_flat():
    calib = pd.read_csv(OUT / "calibration_check.csv")
    naive = pd.read_csv(OUT / "naive_flat_comparison.csv")

    fig, ax = plt.subplots(figsize=(8.5, 4.8))
    ax.plot(calib["tenor_years"], calib["quoted_spread_bp"], color=TEXT_PRIMARY, linewidth=1.6,
            marker="o", markersize=6, label="Quoted market spread")
    ax.plot(calib["tenor_years"], calib["model_spread_bp"], color=BLUE, linewidth=2.2,
            linestyle="--", marker="o", markersize=4, markerfacecolor=SURFACE,
            label="Bootstrapped curve (matches exactly)")
    ax.plot(naive["tenor_years"], naive["flat_implied_bp"], color=ORANGE, linewidth=1.8,
            marker="s", markersize=4, markerfacecolor=SURFACE,
            label="Single flat hazard (calibrated to 5Y only)")
    ax.set_xlabel("Tenor (years)")
    ax.set_ylabel("Par CDS spread (bp)")
    ax.set_title("Bootstrapped term structure vs. a single flat hazard rate", loc="left", fontsize=12)
    ax.legend(frameon=False, loc="upper left")
    ax.spines[["top", "right"]].set_visible(False)
    fig.tight_layout()
    fig.savefig(OUT / "bootstrap_vs_naive_flat.png", dpi=160)
    plt.close(fig)


def plot_cs01_buckets():
    cs01 = pd.read_csv(OUT / "cs01.csv")
    fig, ax = plt.subplots(figsize=(7, 4.5))
    xs = np.arange(len(cs01))
    colors = [AQUA if abs(v) < 1.0 else BLUE for v in cs01["cs01_usd_per_bp"]]
    ax.bar(xs, cs01["cs01_usd_per_bp"], color=colors, width=0.55)
    ymax = max(cs01["cs01_usd_per_bp"].max(), 1.0)
    for x, v in zip(xs, cs01["cs01_usd_per_bp"]):
        if abs(v) > 1:
            ax.text(x, v + ymax * 0.02, f"${v:,.0f}", ha="center", va="bottom", fontsize=9)
    ax.set_xticks(xs)
    ax.set_xticklabels([f"{t:g}Y" for t in cs01["tenor_years"]])
    ax.set_ylim(-ymax * 0.08, ymax * 1.15)
    ax.axhline(0, color=TEXT_SECONDARY, linewidth=0.8)
    ax.set_ylabel("CS01 ($ per 1bp bump)")
    ax.set_title("Bucketed CS01: $10mm 5Y protection buyer, struck at par", loc="left", fontsize=12)
    ax.annotate("all other buckets ≈ $0\n(outside this position's own tenor)",
                xy=(0, 0), xytext=(0.02, 0.90), textcoords="axes fraction",
                fontsize=9, color=TEXT_SECONDARY)
    ax.spines[["top", "right"]].set_visible(False)
    fig.tight_layout()
    fig.savefig(OUT / "cs01_buckets.png", dpi=160)
    plt.close(fig)


if __name__ == "__main__":
    plot_hazard_and_survival()
    plot_bootstrap_vs_naive_flat()
    plot_cs01_buckets()
    for name in ["hazard_and_survival", "bootstrap_vs_naive_flat", "cs01_buckets"]:
        print(f"wrote {OUT / (name + '.png')}")
