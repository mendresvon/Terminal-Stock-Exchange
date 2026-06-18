#pragma once

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

// ─────────────────────────────────────────────
//  Shared types used across the entire TSE app
// ─────────────────────────────────────────────

/// Discriminates the three tradeable asset categories.
enum class AssetType { STOCK, CRYPTO, ETF };

/// Immutable record written for every buy / sell execution.
/// Epic 3 serialises these to data/trade_logs.txt.
struct TransactionRecord {
    std::string symbol;     ///< Ticker, e.g. "AAPL"
    std::string type;       ///< "BUY" or "SELL"
    double      price;      ///< Execution price per share/unit
    int         quantity;   ///< Number of units traded
    std::string timestamp;  ///< ISO-8601 wall-clock string
};

// ─────────────────────────────────────────────
//  Utility
// ─────────────────────────────────────────────

/// Returns the current local time as an ISO-8601 string,
/// e.g. "2024-01-15T10:32:00".  Used by FileManager to
/// timestamp every trade log entry.
inline std::string nowIso8601() {
    std::time_t  t  = std::time(nullptr);
    std::tm*     tm = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}
