#pragma once

#include <deque>
#include <iostream>
#include <string>

#include "types/Types.hpp"

// ─────────────────────────────────────────────────────────────────
//  FinancialAsset  —  Abstract Base Class for all tradeable assets
// ─────────────────────────────────────────────────────────────────
//
//  Derived classes: Stock, Crypto, ETF
//
//  Key design decisions
//  ────────────────────
//  • priceHistory is a std::deque<double> capped at MAX_HISTORY (30)
//    so Epic 4 can render the ASCII price chart directly.
//  • calculateVolatility() is pure-virtual: each asset type returns
//    its own daily volatility factor used by the Epic 2 random-walk
//    price engine.
//  • getTradingFee() is pure-virtual: crypto charges more than stocks.

class FinancialAsset {
protected:
    std::string        symbol;
    std::string        name;
    double             currentPrice;
    std::deque<double> priceHistory;   ///< Rolling 30-day price history
    AssetType          assetType;

    static constexpr std::size_t MAX_HISTORY = 30;

public:
    FinancialAsset(const std::string& sym,
                   const std::string& nm,
                   double             initPrice,
                   AssetType          type)
        : symbol(sym)
        , name(nm)
        , currentPrice(initPrice)
        , assetType(type)
    {
        priceHistory.push_back(initPrice);
    }

    virtual ~FinancialAsset() = default;

    // ── Pure virtuals ────────────────────────────────────────────
    /// Returns the daily fractional volatility (e.g. 0.02 = ±2 %)
    virtual double calculateVolatility() const = 0;

    /// Returns the fee rate applied on each trade (e.g. 0.001 = 0.1 %)
    virtual double getTradingFee() const = 0;

    // ── Virtual with default implementations ─────────────────────
    /// Appends newPrice to history and evicts entries beyond MAX_HISTORY.
    virtual void updatePrice(double newPrice) {
        currentPrice = newPrice;
        priceHistory.push_back(newPrice);
        if (priceHistory.size() > MAX_HISTORY) {
            priceHistory.pop_front();
        }
    }

    /// Stub display — Epic 4 will replace this with a full ANSI panel.
    virtual void display() const {
        std::cout << "  [" << symbol << "] "
                  << name
                  << "  $" << currentPrice
                  << "  vol=" << calculateVolatility()
                  << "\n";
    }

    // ── Getters ──────────────────────────────────────────────────
    const std::string&        getSymbol()       const { return symbol;       }
    const std::string&        getName()         const { return name;         }
    double                    getCurrentPrice() const { return currentPrice; }
    const std::deque<double>& getPriceHistory() const { return priceHistory; }
    AssetType                 getAssetType()    const { return assetType;    }
};
