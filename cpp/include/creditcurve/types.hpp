#pragma once

namespace creditcurve {

// A single market-quoted CDS spread at a given tenor (in years).
// Spread is in decimal form, e.g. 0.0125 = 125 basis points.
struct CDSQuote {
    double tenorYears;
    double spread;
};

// Standard single-name CDS contract conventions used throughout this library.
// Defaults follow common ISDA "Big Bang" / SNAC conventions in simplified form
// (quarterly premium payments, act/360-style accrual approximated as 1/4 year,
// 40% recovery for senior unsecured).
struct CDSConventions {
    double recoveryRate = 0.40;
    int paymentsPerYear = 4;
    double accrualFraction = 0.5;  // linear approximation for accrued premium on default
};

}  // namespace creditcurve
