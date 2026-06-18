#include "io/FileManager.hpp"
#include "engine/MarketEngine.hpp"
#include "accounts/PlayerTrader.hpp"
#include "accounts/AdminAccount.hpp"
#include "assets/Stock.hpp"
#include "assets/Crypto.hpp"
#include "assets/ETF.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────
//  Internal parsing helpers (file-local)
// ─────────────────────────────────────────────────────────────────────────

namespace {

/// Split a string on every occurrence of `delim`.
std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::istringstream       ss(s);
    std::string              tok;
    while (std::getline(ss, tok, delim)) {
        tokens.push_back(tok);
    }
    return tokens;
}

/// Safely parse a double; returns 0.0 on failure.
double toDouble(const std::string& s) {
    try    { return std::stod(s); }
    catch  (...) { return 0.0; }
}

/// Safely parse an int; returns 0 on failure.
int toInt(const std::string& s) {
    try    { return std::stoi(s); }
    catch  (...) { return 0; }
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────
//  ensureDataDir
// ─────────────────────────────────────────────────────────────────────────

void FileManager::ensureDataDir() {
    if (!fs::exists(DATA_DIR)) {
        fs::create_directory(DATA_DIR);
    }
}

// ─────────────────────────────────────────────────────────────────────────
//  saveMarket
//
//  Format:
//    DAY|<n>
//    STOCK|<sym>|<name>|<price0>|<price1>|...
//    CRYPTO|<sym>|<name>|<price0>|...
//    ETF|<sym>|<name>|<price0>|...
// ─────────────────────────────────────────────────────────────────────────

bool FileManager::saveMarket(const MarketEngine& engine) {
    ensureDataDir();
    std::ofstream ofs(MARKET_FILE, std::ios::trunc);
    if (!ofs) {
        std::cerr << "[FileManager] Cannot open " << MARKET_FILE << " for writing.\n";
        return false;
    }

    ofs << "DAY|" << engine.getCurrentDay() << "\n";

    for (const auto& [sym, asset] : engine.getAssets()) {
        switch (asset->getAssetType()) {
            case AssetType::STOCK:  ofs << "STOCK";  break;
            case AssetType::CRYPTO: ofs << "CRYPTO"; break;
            case AssetType::ETF:    ofs << "ETF";    break;
        }
        ofs << "|" << asset->getSymbol()
            << "|" << asset->getName();

        for (double p : asset->getPriceHistory()) {
            ofs << "|" << p;
        }
        ofs << "\n";
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────
//  loadMarket
// ─────────────────────────────────────────────────────────────────────────

bool FileManager::loadMarket(MarketEngine& engine) {
    std::ifstream ifs(MARKET_FILE);
    if (!ifs) return false;   // file doesn't exist yet — first run

    engine.clearAssets();

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;

        auto parts = split(line, '|');
        if (parts.empty()) continue;

        // ── Day counter ──────────────────────────────────────────────────
        if (parts[0] == "DAY") {
            if (parts.size() >= 2) engine.setCurrentDay(toInt(parts[1]));
            continue;
        }

        // ── Asset rows: need at least type + sym + name + 1 price ────────
        if (parts.size() < 4) continue;

        const std::string& type = parts[0];
        const std::string& sym  = parts[1];
        const std::string& name = parts[2];

        // Reconstruct price history (index 3 … end)
        std::vector<double> prices;
        prices.reserve(parts.size() - 3);
        for (std::size_t i = 3; i < parts.size(); ++i) {
            prices.push_back(toDouble(parts[i]));
        }

        double initPrice = prices.empty() ? 0.0 : prices.back();

        std::shared_ptr<FinancialAsset> asset;

        if (type == "STOCK") {
            // We serialise volatility only via the asset type default.
            // Use a neutral volatility here; real volatility is baked into
            // the Stock class default (0.02).  For custom-added stocks we
            // store the type string only — acceptable for a simulation.
            asset = std::make_shared<Stock>(sym, name, initPrice);
        } else if (type == "CRYPTO") {
            asset = std::make_shared<Crypto>(sym, name, initPrice);
        } else if (type == "ETF") {
            asset = std::make_shared<ETF>(sym, name, initPrice);
        } else {
            continue;   // Unknown type — skip
        }

        // Replay historical prices into the asset's deque (except the last,
        // which is already the initial price set in the constructor).
        for (std::size_t i = 0; i + 1 < prices.size(); ++i) {
            asset->updatePrice(prices[i]);
        }
        // Make sure the final (current) price is set correctly.
        asset->updatePrice(initPrice);

        engine.addAsset(asset);
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────
//  saveAccounts
//
//  Format:
//    PLAYER|<username>|<passwordHash>|<cash>|<sym:qty:avgCost,...>
//    ADMIN|<username>|<passwordHash>|0.00|
// ─────────────────────────────────────────────────────────────────────────

bool FileManager::saveAccounts(const std::vector<std::shared_ptr<Account>>& accounts) {
    ensureDataDir();
    std::ofstream ofs(ACCOUNTS_FILE, std::ios::trunc);
    if (!ofs) {
        std::cerr << "[FileManager] Cannot open " << ACCOUNTS_FILE << " for writing.\n";
        return false;
    }

    for (const auto& acct : accounts) {
        ofs << acct->getAccountType()
            << "|" << acct->getUsername()
            << "|" << acct->getPasswordHash()
            << "|" << acct->getCashBalance()
            << "|";

        // Serialise portfolio + cost basis as: SYM:QTY:AVGCOST,...
        const auto& portfolio  = acct->getPortfolio();
        const auto& costBasis  = acct->getAvgCostBasis();
        bool        first      = true;
        for (const auto& [sym, qty] : portfolio) {
            if (!first) ofs << ",";
            first = false;
            double avg = costBasis.count(sym) ? costBasis.at(sym) : 0.0;
            ofs << sym << ":" << qty << ":" << avg;
        }
        ofs << "\n";
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────
//  loadAccounts
// ─────────────────────────────────────────────────────────────────────────

std::vector<std::shared_ptr<Account>> FileManager::loadAccounts() {
    std::vector<std::shared_ptr<Account>> accounts;
    std::ifstream ifs(ACCOUNTS_FILE);
    if (!ifs) return accounts;   // first run — no file yet

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;

        auto parts = split(line, '|');
        if (parts.size() < 5) continue;

        const std::string& type         = parts[0];
        const std::string& username     = parts[1];
        const std::string& passwordHash = parts[2];
        double             cash         = toDouble(parts[3]);
        const std::string& portfolioStr = parts[4];

        std::shared_ptr<Account> acct;

        if (type == "PLAYER") {
            // Construct with empty password — we will manually set the hash
            auto player = std::make_shared<PlayerTrader>(username, "", cash);
            // Restore the stored hash directly (bypass re-hashing)
            player->setPasswordHashDirect(passwordHash);
            acct = player;
        } else if (type == "ADMIN") {
            auto admin = std::make_shared<AdminAccount>(username, "");
            admin->setPasswordHashDirect(passwordHash);
            acct = admin;
        } else {
            continue;
        }

        acct->setCashBalance(cash);

        // Restore portfolio + cost basis
        if (!portfolioStr.empty()) {
            std::unordered_map<std::string, int>    portfolio;
            std::unordered_map<std::string, double> costBasis;

            for (const auto& entry : split(portfolioStr, ',')) {
                auto fields = split(entry, ':');
                if (fields.size() < 3) continue;
                const std::string& sym = fields[0];
                int    qty  = toInt(fields[1]);
                double avg  = toDouble(fields[2]);
                if (qty > 0) {
                    portfolio[sym] = qty;
                    costBasis[sym] = avg;
                }
            }

            acct->setPortfolio(costBasis.empty()
                                    ? std::unordered_map<std::string,int>{}
                                    : portfolio);
            acct->setAvgCostBasis(costBasis);
        }

        accounts.push_back(acct);
    }

    return accounts;
}

// ─────────────────────────────────────────────────────────────────────────
//  appendTradeLog
//
//  Format (one line per trade):
//    <timestamp>|<username>|<BUY|SELL>|<sym>|<qty>|<price>
// ─────────────────────────────────────────────────────────────────────────

bool FileManager::appendTradeLog(const TransactionRecord& record,
                                 const std::string&       username) {
    ensureDataDir();
    std::ofstream ofs(TRADE_LOG_FILE, std::ios::app);
    if (!ofs) {
        std::cerr << "[FileManager] Cannot open " << TRADE_LOG_FILE << " for appending.\n";
        return false;
    }

    ofs << record.timestamp
        << "|" << username
        << "|" << record.type
        << "|" << record.symbol
        << "|" << record.quantity
        << "|" << record.price
        << "\n";

    return true;
}
