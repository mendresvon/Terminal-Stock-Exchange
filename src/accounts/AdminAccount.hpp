#pragma once

#include <memory>

#include "Account.hpp"
#include "assets/FinancialAsset.hpp"

// ────────────────────────────────────────────────────────────────
//  AdminAccount  —  Elevated account with market management powers
// ────────────────────────────────────────────────────────────────

class AdminAccount : public Account {
public:
    /**
     * @param username  Admin login name
     * @param password  Plain-text password (hashed on construction)
     */
    AdminAccount(const std::string& username, const std::string& password);

    bool        authenticate(const std::string& password) const override;
    std::string getAccountType()                          const override;

    // ── Admin operations (Epic 2 / 3 implementation) ──────────────
    /// Registers a new asset on the market engine.
    void addAsset(std::shared_ptr<FinancialAsset> asset);

    /// Delists an asset by ticker symbol.
    void removeAsset(const std::string& symbol);

    /// Wipes all player accounts and resets price history to defaults.
    void resetSimulation();
};
