"""Shared value types for the credit-curve bootstrapper."""
from dataclasses import dataclass


@dataclass(frozen=True)
class CDSQuote:
    """A single market-quoted CDS spread at a given tenor (in years).

    Spread is in decimal form, e.g. 0.0125 = 125 basis points.
    """

    tenor_years: float
    spread: float


@dataclass(frozen=True)
class CDSConventions:
    """Standard single-name CDS contract conventions.

    Defaults follow common ISDA "Big Bang" / SNAC conventions in simplified
    form: quarterly premium payments, act/360-style accrual approximated as a
    flat 1/4 year, and 40% recovery for senior unsecured debt.
    """

    recovery_rate: float = 0.40
    payments_per_year: int = 4
    accrual_fraction: float = 0.5  # linear approximation for accrued premium on default
