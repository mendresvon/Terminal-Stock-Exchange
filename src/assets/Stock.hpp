#pragma once

#include "FinancialAsset.hpp"

// ─────────────────────────────────────────────────────
//  Stock  —  Equities with moderate daily volatility
// ─────────────────────────────────────────────────────

class Stock : public FinancialAsset {
private:
    double volatilityFactor;  ///< Daily fractional sigma, e.g. 0.02 = ±2 %
    double dividendYield;     ///< Annual yield fraction, e.g. 0.02 = 2 %

public:
    /**
     * @param symbol          Ticker symbol, e.g. "AAPL"
     * @param name            Human-readable name
     * @param initialPrice    Starting price in USD
     * @param volatilityFactor  Default ±2 % / day
     * @param dividendYield   Default 0 (non-dividend stock)
     */
    Stock(const std::string& symbol,
          const std::string& name,
          double             initialPrice,
          double             volatilityFactor = 0.02,
          double             dividendYield    = 0.0);

    double calculateVolatility() const override;
    double getTradingFee()       const override;

    // ── Stock-specific getters ────────────────────────────────────
    double getDividendYield()    const { return dividendYield;    }
    double getVolatilityFactor() const { return volatilityFactor; }
};
