#pragma once

#include "FinancialAsset.hpp"

// ──────────────────────────────────────────────────────────────
//  Crypto  —  Cryptocurrency with high volatility and 24/7 trading
// ──────────────────────────────────────────────────────────────

class Crypto : public FinancialAsset {
private:
    double volatilityFactor;  ///< Daily fractional sigma, e.g. 0.07 = ±7 %
    double tradingFeeRate;    ///< Fee per trade, e.g. 0.005 = 0.5 %
    bool   tradesAroundClock; ///< Always true for crypto (24/7)

public:
    /**
     * @param symbol          Ticker symbol, e.g. "BTC"
     * @param name            Human-readable name
     * @param initialPrice    Starting price in USD
     * @param volatilityFactor  Default ±7 % / day
     * @param tradingFeeRate  Default 0.5 % per trade
     */
    Crypto(const std::string& symbol,
           const std::string& name,
           double             initialPrice,
           double             volatilityFactor = 0.07,
           double             tradingFeeRate   = 0.005);

    double calculateVolatility() const override;
    double getTradingFee()       const override;

    // ── Crypto-specific getters ───────────────────────────────────
    bool   isAroundClock()       const { return tradesAroundClock; }
    double getVolatilityFactor() const { return volatilityFactor;  }
    double getTradingFeeRate()   const { return tradingFeeRate;    }
};
