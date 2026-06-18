#pragma once

#include <string>
#include <vector>

#include "FinancialAsset.hpp"

// ──────────────────────────────────────────────────────────────────
//  ETF  —  Exchange-Traded Fund with low volatility and a basket
// ──────────────────────────────────────────────────────────────────

class ETF : public FinancialAsset {
private:
    double                   volatilityFactor; ///< Default ±0.8 % / day
    std::vector<std::string> basketSymbols;    ///< Constituent ticker symbols

public:
    /**
     * @param symbol          Ticker symbol, e.g. "SPY"
     * @param name            Human-readable name
     * @param initialPrice    Starting price in USD
     * @param basket          List of constituent tickers (stub for Epic 2)
     * @param volatilityFactor  Default ±0.8 % / day
     */
    ETF(const std::string&        symbol,
        const std::string&        name,
        double                    initialPrice,
        std::vector<std::string>  basket           = {},
        double                    volatilityFactor = 0.008);

    double calculateVolatility() const override;
    double getTradingFee()       const override;

    // ── ETF-specific getters ──────────────────────────────────────
    const std::vector<std::string>& getBasketSymbols()   const { return basketSymbols;    }
    double                          getVolatilityFactor() const { return volatilityFactor; }
};
