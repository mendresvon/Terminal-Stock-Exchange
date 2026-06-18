#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "assets/FinancialAsset.hpp"
#include "accounts/Account.hpp"

// ─────────────────────────────────────────────────────────────────
//  MarketEngine  —  Central registry for assets and simulation state
// ─────────────────────────────────────────────────────────────────
//
//  This is the single source of truth for:
//    • The market asset universe  (unordered_map for O(1) lookup)
//    • The currently logged-in user
//    • The simulated trading-day counter
//
//  Epic 2 will flesh out stepDay() with the random-walk algorithm
//  and the 10 % chance News Event system.
//  Epic 3 will add save() / load() delegating to FileManager.

class MarketEngine {
private:
    std::unordered_map<std::string, std::shared_ptr<FinancialAsset>> assets;
    std::shared_ptr<Account> currentUser;
    int currentDay;

public:
    MarketEngine();

    // ── Asset registry ────────────────────────────────────────────
    void addAsset(std::shared_ptr<FinancialAsset> asset);
    void removeAsset(const std::string& symbol);

    /// O(1) lookup; returns nullptr if symbol not found.
    std::shared_ptr<FinancialAsset> getAsset(const std::string& symbol) const;

    /// Read-only access to the full asset map (for portfolio rendering).
    const std::unordered_map<std::string, std::shared_ptr<FinancialAsset>>&
    getAssets() const;

    /// Prints a basic summary table of all listed assets.
    void listAssets() const;

    // ── Session management ────────────────────────────────────────
    void                     setCurrentUser(std::shared_ptr<Account> user);
    std::shared_ptr<Account> getCurrentUser() const;

    // ── Simulation control ────────────────────────────────────────
    int  getCurrentDay() const { return currentDay; }

    /// Advance simulation by one trading day.
    /// Epic 2: random-walk price updates + news events.
    void stepDay();

    // ── Seeding ───────────────────────────────────────────────────
    /// Populates the market with a default set of Stocks, Cryptos, ETFs.
    void seedDefaultAssets();
};
