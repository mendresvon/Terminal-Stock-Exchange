#pragma once

#include <array>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>

#include "assets/FinancialAsset.hpp"
#include "accounts/Account.hpp"

// ─────────────────────────────────────────────────────────────────────────
//  MarketEngine  —  Central registry, simulation controller, and news desk
//
//  Epic 2 additions
//  ────────────────
//  • stepDay() now drives a random-walk price engine using <random>.
//    Each asset's daily return is drawn from N(0, volatility).
//  • With NEWS_PROBABILITY (10 %) per day, one random asset receives a
//    large spike or crash, and a dramatic headline is printed.
//  • clearAssets() allows the admin panel to wipe and re-seed the market.
// ─────────────────────────────────────────────────────────────────────────

class MarketEngine {
private:
    std::unordered_map<std::string, std::shared_ptr<FinancialAsset>> assets;
    std::shared_ptr<Account> currentUser;
    int currentDay;

    // ── Random number generation ──────────────────────────────────────────
    std::mt19937 rng;   ///< Seeded from std::random_device on construction

    // ── News event constants ──────────────────────────────────────────────
    static constexpr double NEWS_PROBABILITY = 0.10;  ///< 10 % chance/day
    static constexpr double NEWS_SPIKE_MIN   = 0.15;  ///< Min event magnitude
    static constexpr double NEWS_SPIKE_MAX   = 0.40;  ///< Max event magnitude

    static constexpr std::array<const char*, 8> POSITIVE_HEADLINES = {{
        " BEATS quarterly earnings by 20% — Analysts upgrade to BUY!",
        " announces massive $5B share buyback program!",
        " signs landmark partnership deal — investors cheer!",
        " CEO unveils revolutionary product at investor day!",
        ": Hedge funds disclose major new long positions!",
        " added to major index — passive demand explodes!",
        ": Short squeeze sends shares flying higher!",
        " raises full-year guidance — Wall Street celebrates!"
    }};

    static constexpr std::array<const char*, 8> NEGATIVE_HEADLINES = {{
        " MISSES earnings by 15% — stock collapses!",
        ": CEO resigns unexpectedly amid accounting scandal!",
        " faces surprise SEC investigation!",
        " issues profit warning — full-year guidance slashed!",
        ": Massive data breach exposes 50M customer records!",
        " faces sweeping regulatory crackdown!",
        ": Class action lawsuit filed — shares crater!",
        ": Analyst downgrades to SELL, target cut 40%!"
    }};

    /// Selects a random asset, applies a spike or crash, prints a headline.
    void applyNewsEvent();

public:
    MarketEngine();

    // ── Asset registry ────────────────────────────────────────────────────
    void addAsset(std::shared_ptr<FinancialAsset> asset);
    void removeAsset(const std::string& symbol);
    void clearAssets();   ///< Wipes all assets (admin reset)

    /// O(1) lookup; returns nullptr if symbol not found.
    std::shared_ptr<FinancialAsset> getAsset(const std::string& symbol) const;

    const std::unordered_map<std::string, std::shared_ptr<FinancialAsset>>&
    getAssets() const;

    /// Sorted market table: Stocks → ETFs → Crypto, then alphabetically.
    void listAssets() const;

    // ── Session management ────────────────────────────────────────────────
    void                     setCurrentUser(std::shared_ptr<Account> user);
    std::shared_ptr<Account> getCurrentUser() const;

    // ── Simulation control ────────────────────────────────────────────────
    int  getCurrentDay() const { return currentDay; }

    /// Advances one trading day:
    ///  1. Random-walk each asset price using N(0, volatility).
    ///  2. With 10 % probability, trigger a news event on one random asset.
    void stepDay();

    // ── Seeding ───────────────────────────────────────────────────────────
    void seedDefaultAssets();
};
