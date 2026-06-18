#pragma once

#include <memory>
#include <string>
#include <vector>

#include "types/Types.hpp"

// Forward declarations to avoid circular includes
class FinancialAsset;
class Account;
class MarketEngine;

// ─────────────────────────────────────────────────────────────────────────
//  FileManager  —  Stateless I/O service for the Terminal Stock Exchange
//
//  All methods are static.  The three data files live in data/:
//
//    data/market_data.txt  — asset registry + 30-day price deques + day
//    data/accounts.txt     — all user accounts (cash, portfolio, cost basis)
//    data/trade_logs.txt   — append-only transaction log
//
//  File format: pipe-delimited plain text.  See FileManager.cpp for the
//  exact grammar of each section.
// ─────────────────────────────────────────────────────────────────────────

class FileManager {
public:
    // ── Directory bootstrap ───────────────────────────────────────────────
    /// Creates the data/ directory if it does not already exist.
    static void ensureDataDir();

    // ── Market data ───────────────────────────────────────────────────────
    /// Writes the current asset registry and day counter to
    /// data/market_data.txt.  Returns true on success.
    static bool saveMarket(const MarketEngine& engine);

    /// Reads data/market_data.txt and restores assets + day counter into
    /// the engine.  Returns true if the file existed and was parsed OK.
    static bool loadMarket(MarketEngine& engine);

    // ── Account data ──────────────────────────────────────────────────────
    /// Serialises every registered account to data/accounts.txt.
    static bool saveAccounts(const std::vector<std::shared_ptr<Account>>& accounts);

    /// Reads data/accounts.txt and reconstructs PlayerTrader / AdminAccount
    /// objects.  Returns the list; empty if file not found.
    static std::vector<std::shared_ptr<Account>> loadAccounts();

    // ── Trade log (append-only) ───────────────────────────────────────────
    /// Appends one line to data/trade_logs.txt for every completed trade.
    static bool appendTradeLog(const TransactionRecord& record,
                               const std::string&       username);

private:
    static constexpr const char* DATA_DIR        = "data";
    static constexpr const char* MARKET_FILE     = "data/market_data.txt";
    static constexpr const char* ACCOUNTS_FILE   = "data/accounts.txt";
    static constexpr const char* TRADE_LOG_FILE  = "data/trade_logs.txt";

    FileManager() = delete;  ///< Static-only class — no instances
};
