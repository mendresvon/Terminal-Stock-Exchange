#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "types/Types.hpp"

// ─────────────────────────────────────────────────────────────────
//  Account  —  Abstract Base Class for all user account types
// ─────────────────────────────────────────────────────────────────
//
//  Derived classes: PlayerTrader, AdminAccount
//
//  Key design decisions
//  ────────────────────
//  • passwordHash uses std::hash<std::string>.
//    TODO: upgrade to FNV-1a or bcrypt before any real deployment.
//  • portfolio is an unordered_map<symbol, qty> for O(1) lookup.
//  • tradeHistory is a std::vector<TransactionRecord> — Epic 3 will
//    flush this to data/trade_logs.txt on save.

class Account {
protected:
    std::string username;
    std::string passwordHash;
    double      cashBalance;

    /// symbol → quantity held
    std::unordered_map<std::string, int>    portfolio;
    std::vector<TransactionRecord>          tradeHistory;
    /// symbol → weighted-average purchase cost per unit
    std::unordered_map<std::string, double> avgCostBasis;

    // TODO: upgrade hash — std::hash is NOT cryptographically secure
    static std::string hashPassword(const std::string& password) {
        std::hash<std::string> hasher;
        return std::to_string(hasher(password));
    }

    /// Updates or erases the cost-basis entry for a position.
    void updateCostBasis(const std::string& symbol, int qty, double avgCost) {
        if (qty <= 0) { avgCostBasis.erase(symbol); }
        else          { avgCostBasis[symbol] = avgCost; }
    }

public:
    Account(const std::string& user,
            const std::string& password,
            double             initialCash)
        : username(user)
        , passwordHash(hashPassword(password))
        , cashBalance(initialCash)
    {}

    virtual ~Account() = default;

    // ── Pure virtuals ────────────────────────────────────────────
    virtual bool        authenticate(const std::string& password) const = 0;
    virtual std::string getAccountType()                          const = 0;

    // ── Getters ──────────────────────────────────────────────────
    const std::string& getUsername()     const { return username;     }
    double             getCashBalance()  const { return cashBalance;  }
    const std::string& getPasswordHash() const { return passwordHash; }

    const std::unordered_map<std::string, int>&    getPortfolio()    const { return portfolio;    }
    const std::vector<TransactionRecord>&           getTradeHistory() const { return tradeHistory; }
    const std::unordered_map<std::string, double>&  getAvgCostBasis() const { return avgCostBasis; }

    // ── Setters / mutators (used by derived classes and Epic 3 I/O)
    void setCashBalance(double balance)                                       { cashBalance = balance;  }
    void setPortfolio(const std::unordered_map<std::string, int>& p)         { portfolio   = p;        }
    void addTradeRecord(const TransactionRecord& record)                      { tradeHistory.push_back(record); }
    void modifyCash(double delta)                                             { cashBalance += delta;   }

    /// Restores the weighted-average cost map from persisted data (FileManager).
    void setAvgCostBasis(const std::unordered_map<std::string, double>& basis) {
        avgCostBasis = basis;
    }

    /// Directly sets the stored password hash (FileManager use only — bypasses
    /// hashing so a hash loaded from disk is not double-hashed).
    void setPasswordHashDirect(const std::string& hash) {
        passwordHash = hash;
    }
};
