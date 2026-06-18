#pragma once

#include <string>

// ─────────────────────────────────────────────
//  Shared types used across the entire TSE app
// ─────────────────────────────────────────────

/// Discriminates the three tradeable asset categories.
enum class AssetType { STOCK, CRYPTO, ETF };

/// Immutable record written for every buy / sell execution.
/// Epic 3 will serialise these to data/trade_logs.txt.
struct TransactionRecord {
    std::string symbol;     ///< Ticker, e.g. "AAPL"
    std::string type;       ///< "BUY" or "SELL"
    double      price;      ///< Execution price per share/unit
    int         quantity;   ///< Number of units traded
    std::string timestamp;  ///< ISO-8601 wall-clock string (Epic 3)
};
