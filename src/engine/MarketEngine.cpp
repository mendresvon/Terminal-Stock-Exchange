#include "MarketEngine.hpp"
#include "assets/Stock.hpp"
#include "assets/Crypto.hpp"
#include "assets/ETF.hpp"
#include "accounts/AdminAccount.hpp"
#include "io/FileManager.hpp"
#include "ui/Terminal.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>

// ─────────────────────────────────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────────────────────────────────

MarketEngine::MarketEngine()
    : currentDay(1)
    , rng(std::random_device{}())  // hardware-entropy seed
{}

// ─────────────────────────────────────────────────────────────────────────
//  Asset registry
// ─────────────────────────────────────────────────────────────────────────

void MarketEngine::addAsset(std::shared_ptr<FinancialAsset> asset) {
    assets[asset->getSymbol()] = std::move(asset);
}

void MarketEngine::removeAsset(const std::string& symbol) {
    assets.erase(symbol);
}

void MarketEngine::clearAssets() {
    assets.clear();
}

std::shared_ptr<FinancialAsset>
MarketEngine::getAsset(const std::string& symbol) const {
    auto it = assets.find(symbol);
    return (it != assets.end()) ? it->second : nullptr;
}

const std::unordered_map<std::string, std::shared_ptr<FinancialAsset>>&
MarketEngine::getAssets() const {
    return assets;
}

// ─────────────────────────────────────────────────────────────────────────
//  listAssets()  —  Sorted, formatted market table
// ─────────────────────────────────────────────────────────────────────────

void MarketEngine::listAssets() const {
    using AP = std::pair<std::string, std::shared_ptr<FinancialAsset>>;
    std::vector<AP> sorted(assets.begin(), assets.end());

    // Order: Stocks → ETFs → Crypto; then alphabetically within each group
    auto typeOrder = [](AssetType t) -> int {
        switch (t) {
            case AssetType::STOCK:  return 0;
            case AssetType::ETF:    return 1;
            case AssetType::CRYPTO: return 2;
        }
        return 3;
    };

    std::sort(sorted.begin(), sorted.end(), [&](const AP& a, const AP& b) {
        int oa = typeOrder(a.second->getAssetType());
        int ob = typeOrder(b.second->getAssetType());
        if (oa != ob) return oa < ob;
        return a.first < b.first;
    });

    // Header row
    std::cout << "\n  "
              << Terminal::BOLD
              << std::left  << std::setw(7)  << "SYMBOL"
              << std::setw(22) << "NAME"
              << std::right << std::setw(12) << "PRICE"
              << std::setw(9)  << "CHANGE"
              << "  "
              << std::left  << std::setw(7)  << "TYPE"
              << std::setw(6)  << "VOL"
              << "  TREND"
              << Terminal::RESET << "\n";

    // Divider
    std::cout << "  " << Terminal::CYAN << Terminal::DIM;
    for (int i = 0; i < 73; ++i) std::cout << "\u2500";
    std::cout << Terminal::RESET << "\n";

    for (const auto& [sym, asset] : sorted) {
        const auto& hist = asset->getPriceHistory();
        double cur  = asset->getCurrentPrice();
        double prev = (hist.size() >= 2) ? *(hist.end() - 2) : cur;
        double chg  = cur - prev;
        double pct  = (prev > 0.0) ? chg / prev * 100.0 : 0.0;

        const char* typeTag  = "STOCK";
        if (asset->getAssetType() == AssetType::CRYPTO) typeTag = "CRYPTO";
        if (asset->getAssetType() == AssetType::ETF)    typeTag = "ETF";

        const char* priceColor = (chg >= 0.0) ? Terminal::BRIGHT_GREEN : Terminal::BRIGHT_RED;
        const char* arrow      = (chg >= 0.0) ? "\u25b2" : "\u25bc";

        // Sparkline
        std::string spark      = Terminal::printSparkline(hist, 8);
        const char* sparkColor = Terminal::sparklineColor(hist);

        std::cout << "  "
                  << std::left  << std::setw(7)  << sym
                  << std::left  << std::setw(22) << asset->getName()
                  << priceColor
                  << std::right << std::setw(10) << std::fixed << std::setprecision(2) << cur
                  << "  " << arrow << " "
                  << std::right << std::setw(5)  << std::fixed << std::setprecision(1)
                  << std::abs(pct) << "%"
                  << Terminal::RESET
                  << "  "
                  << std::left << std::setw(7) << typeTag
                  << std::right << std::setw(4) << std::fixed << std::setprecision(2)
                  << asset->calculateVolatility() * 100.0 << "%"
                  << "  "
                  << sparkColor << spark << Terminal::RESET
                  << "\n";
    }
    std::cout << "\n";
}

// ─────────────────────────────────────────────────────────────────────────
//  Account registry
// ─────────────────────────────────────────────────────────────────────────

void MarketEngine::registerAccount(std::shared_ptr<Account> account) {
    // Replace any existing account with the same username
    for (auto& a : accounts) {
        if (a->getUsername() == account->getUsername()) {
            a = account;
            return;
        }
    }
    accounts.push_back(std::move(account));
}

std::shared_ptr<Account> MarketEngine::findAccount(const std::string& username) const {
    for (const auto& a : accounts) {
        if (a->getUsername() == username) return a;
    }
    return nullptr;
}

const std::vector<std::shared_ptr<Account>>& MarketEngine::getAccounts() const {
    return accounts;
}

bool MarketEngine::accountExists(const std::string& username) const {
    return findAccount(username) != nullptr;
}

// ─────────────────────────────────────────────────────────────────────────
//  Session management
// ─────────────────────────────────────────────────────────────────────────

void MarketEngine::setCurrentUser(std::shared_ptr<Account> user) {
    currentUser = std::move(user);
}

std::shared_ptr<Account> MarketEngine::getCurrentUser() const {
    return currentUser;
}

// ─────────────────────────────────────────────────────────────────────────
//  stepDay()  —  Random Walk Price Engine
//
//  Algorithm (per asset)
//  ─────────────────────
//  dailyReturn ~ N(0, volatility)
//  newPrice    = oldPrice × (1 + dailyReturn)
//  newPrice    = max(0.01, newPrice)   ← floor prevents negatives
//
//  After all prices update, a Bernoulli(0.10) trial determines whether
//  a news event fires.  If so, applyNewsEvent() selects one random asset
//  and applies an additional ±15–40 % shock with a printed headline.
// ─────────────────────────────────────────────────────────────────────────

void MarketEngine::stepDay() {
    ++currentDay;

    for (auto& [sym, asset] : assets) {
        double vol = asset->calculateVolatility();
        std::normal_distribution<double> dist(0.0, vol);
        double dailyReturn = dist(rng);
        double newPrice    = asset->getCurrentPrice() * (1.0 + dailyReturn);
        newPrice = std::max(0.01, newPrice);
        asset->updatePrice(newPrice);
    }

    // 10 % news event
    std::bernoulli_distribution newsRoll(NEWS_PROBABILITY);
    if (newsRoll(rng) && !assets.empty()) {
        applyNewsEvent();
    }

    // ── Daily summary: top-3 gainers and top-3 losers ─────────────────────
    struct Move { std::string sym; double pct; };
    std::vector<Move> movers;
    movers.reserve(assets.size());

    for (const auto& [sym, asset] : assets) {
        const auto& hist = asset->getPriceHistory();
        if (hist.size() < 2) continue;
        double prev = *(hist.end() - 2);
        double cur  = asset->getCurrentPrice();
        double pct  = (prev > 0.0) ? (cur - prev) / prev * 100.0 : 0.0;
        movers.push_back({sym, pct});
    }

    std::sort(movers.begin(), movers.end(),
              [](const Move& a, const Move& b){ return a.pct > b.pct; });

    // Header
    std::cout << "\n  " << Terminal::BOLD << Terminal::BRIGHT_CYAN
              << "\u25b8 Day " << currentDay << " trading session complete."
              << Terminal::RESET << "  "
              << Terminal::DIM << assets.size() << " assets updated."
              << Terminal::RESET << "\n";

    if (movers.size() >= 2) {
        // Gainers (top 3)
        std::cout << "\n  " << Terminal::BOLD << Terminal::BRIGHT_GREEN
                  << "Top Gainers" << Terminal::RESET << "\n";
        int shown = 0;
        for (const auto& m : movers) {
            if (m.pct < 0.0 || shown >= 3) break;
            std::cout << "    " << Terminal::BRIGHT_GREEN
                      << std::left  << std::setw(6) << m.sym
                      << std::right << std::setw(7) << std::fixed << std::setprecision(2)
                      << m.pct << "% \u25b2"
                      << Terminal::RESET << "\n";
            ++shown;
        }
        if (shown == 0) {
            std::cout << "    " << Terminal::DIM << "No gainers today." << Terminal::RESET << "\n";
        }

        // Losers (bottom 3)
        std::cout << "\n  " << Terminal::BOLD << Terminal::BRIGHT_RED
                  << "Top Losers" << Terminal::RESET << "\n";
        shown = 0;
        for (int i = static_cast<int>(movers.size()) - 1; i >= 0 && shown < 3; --i) {
            if (movers[static_cast<std::size_t>(i)].pct >= 0.0) break;
            const auto& m = movers[static_cast<std::size_t>(i)];
            std::cout << "    " << Terminal::BRIGHT_RED
                      << std::left  << std::setw(6) << m.sym
                      << std::right << std::setw(7) << std::fixed << std::setprecision(2)
                      << m.pct << "% \u25bc"
                      << Terminal::RESET << "\n";
            ++shown;
        }
        if (shown == 0) {
            std::cout << "    " << Terminal::DIM << "No losers today." << Terminal::RESET << "\n";
        }
    }
    std::cout << "\n";
}

// ─────────────────────────────────────────────────────────────────────────
//  applyNewsEvent()
// ─────────────────────────────────────────────────────────────────────────

void MarketEngine::applyNewsEvent() {
    // Pick a random asset
    std::uniform_int_distribution<std::size_t> assetDist(0, assets.size() - 1);
    auto it = assets.begin();
    std::advance(it, assetDist(rng));
    auto& asset = it->second;

    // Spike (+) or crash (−)?
    bool isPositive = std::bernoulli_distribution(0.50)(rng);
    std::uniform_real_distribution<double> magDist(NEWS_SPIKE_MIN, NEWS_SPIKE_MAX);
    double magnitude = magDist(rng);
    if (!isPositive) magnitude = -magnitude;

    double newPrice = asset->getCurrentPrice() * (1.0 + magnitude);
    newPrice = std::max(0.01, newPrice);
    asset->updatePrice(newPrice);

    // Select and format headline
    const auto& pool = isPositive ? POSITIVE_HEADLINES : NEGATIVE_HEADLINES;
    std::uniform_int_distribution<std::size_t> hDist(0, pool.size() - 1);
    std::string headline = "[ " + asset->getSymbol() + " ]" + pool[hDist(rng)];

    // Print with dramatic color
    const char* color = isPositive ? "\033[92m" : "\033[91m";
    const char* reset = "\033[0m";
    const char* bold  = "\033[1m";
    const char* arrow = isPositive ? "  \u25b2 +" : "  \u25bc ";

    std::cout << "\n"
              << "  " << bold << color
              << "\u25c6\u25c6\u25c6  BREAKING MARKET NEWS  \u25c6\u25c6\u25c6"
              << reset << "\n"
              << "  " << color << headline << reset << "\n"
              << "  " << color << arrow
              << std::fixed << std::setprecision(1)
              << std::abs(magnitude) * 100.0 << "% shock applied to "
              << asset->getSymbol() << reset << "\n\n";
}

// ─────────────────────────────────────────────────────────────────────────
//  seedDefaultAssets()
// ─────────────────────────────────────────────────────────────────────────

void MarketEngine::seedDefaultAssets() {
    // Stocks
    addAsset(std::make_shared<Stock>("AAPL", "Apple Inc.",       178.50, 0.020, 0.005));
    addAsset(std::make_shared<Stock>("TSLA", "Tesla Inc.",       245.00, 0.035, 0.000));
    addAsset(std::make_shared<Stock>("MSFT", "Microsoft Corp.",  415.20, 0.018, 0.009));
    addAsset(std::make_shared<Stock>("NVDA", "NVIDIA Corp.",     875.00, 0.040, 0.001));
    addAsset(std::make_shared<Stock>("AMZN", "Amazon.com Inc.",  185.60, 0.022, 0.000));

    // Crypto
    addAsset(std::make_shared<Crypto>("BTC", "Bitcoin",          43'500.00, 0.060, 0.005));
    addAsset(std::make_shared<Crypto>("ETH", "Ethereum",          2'280.00, 0.075, 0.005));
    addAsset(std::make_shared<Crypto>("SOL", "Solana",              105.00, 0.090, 0.006));

    // ETFs
    addAsset(std::make_shared<ETF>("SPY", "S&P 500 ETF",
        475.50, std::vector<std::string>{"AAPL","MSFT","AMZN","NVDA"}, 0.008));
    addAsset(std::make_shared<ETF>("QQQ", "NASDAQ-100 ETF",
        390.80, std::vector<std::string>{"AAPL","MSFT","NVDA"},        0.010));
}

// ─────────────────────────────────────────────────────────────────────────
//  Persistence  (Epic 3)
// ─────────────────────────────────────────────────────────────────────────

bool MarketEngine::save() const {
    bool ok = true;
    ok &= FileManager::saveMarket(*this);
    ok &= FileManager::saveAccounts(accounts);
    return ok;
}

bool MarketEngine::load() {
    bool marketOk   = FileManager::loadMarket(*this);
    auto loadedAccs = FileManager::loadAccounts();

    for (auto& a : loadedAccs) {
        registerAccount(a);
    }

    // Always ensure a default admin account exists
    if (!accountExists("admin")) {
        registerAccount(std::make_shared<AdminAccount>("admin", "admin"));
    }

    return marketOk || !loadedAccs.empty();
}
