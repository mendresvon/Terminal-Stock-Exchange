#pragma once

#include "Account.hpp"

// ─────────────────────────────────────────────────────────────
//  PlayerTrader  —  Human-controlled trading account
// ─────────────────────────────────────────────────────────────

class PlayerTrader : public Account {
public:
    /**
     * @param username     Login name
     * @param password     Plain-text password (hashed on construction)
     * @param initialCash  Starting cash balance (default $10 000)
     */
    PlayerTrader(const std::string& username,
                 const std::string& password,
                 double             initialCash = 10'000.0);

    bool        authenticate(const std::string& password) const override;
    std::string getAccountType()                          const override;

    // ── Trading operations (Epic 2 implementation) ────────────────
    /// Deducts cash + fee and adds shares to portfolio.
    /// @param day  Simulation day for the transaction timestamp.
    /// @return true on success, false if insufficient funds.
    bool buy(const std::string& symbol,
             int                quantity,
             double             price,
             double             feeRate,
             int                day = 0);

    /// Removes shares from portfolio and credits cash − fee.
    /// @return true on success, false if insufficient holdings.
    bool sell(const std::string& symbol,
              int                quantity,
              double             price,
              double             feeRate,
              int                day = 0);
};
