#include "MarketEngine.hpp"
#include "assets/Stock.hpp"
#include "assets/Crypto.hpp"
#include "assets/ETF.hpp"
#include "accounts/AdminAccount.hpp"
#include "io/FileManager.hpp"

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
              << std::left
              << std::setw(7)  << "SYMBOL"
              << std::setw(22) << "NAME"
              << std::right
              << std::setw(12) << "PRICE"
              << std::setw(8)  << "CHANGE"
              << "  "
              << std::left
              << std::setw(7)  << "TYPE"
              << std::setw(7)  << "VOL"
              << "\n";
    // Print divider using repeated UTF-8 '─' (U+2500)
    std::cout << "  ";
    for (int i = 0; i < 63; ++i) std::cout << "\u2500";
    std::cout << "\n";

    for (const auto& [sym, asset] : sorted) {
        // Price change vs previous day
        const auto& hist = asset->getPriceHistory();
        double cur  = asset->getCurrentPrice();
        double prev = (hist.size() >= 2) ? *(hist.end() - 2) : cur;
        double chg  = cur - prev;
        double pct  = (prev > 0.0) ? chg / prev * 100.0 : 0.0;

        const char* typeTag  = "STOCK";
        if (asset->getAssetType() == AssetType::CRYPTO) typeTag = "CRYPTO";
        if (asset->getAssetType() == AssetType::ETF)    typeTag = "ETF";

        // Choose color based on daily change
        const char* priceColor = (chg >= 0.0) ? "\033[92m" : "\033[91m";
        const char* reset      = "\033[0m";
        const char* arrow      = (chg >= 0.0) ? "\u25b2" : "\u25bc";

        std::cout << "  "
                  << std::left  << std::setw(7)  << sym
                  << std::left  << std::setw(22) << asset->getName()
                  << priceColor
                  << std::right << std::setw(10) << std::fixed << std::setprecision(2) << cur
                  << "  " << arrow << " "
                  << std::right << std::setw(5)  << std::fixed << std::setprecision(1)
                  << std::abs(pct) << "%"
                  << reset
                  << "  "
                  << std::left  << std::setw(7)  << typeTag
                  << std::right << std::setw(5)  << std::fixed << std::setprecision(3)
                  << asset->calculateVolatility()
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

    std::cout << "\n  \033[1m\033[36m\u25b8 Day " << currentDay
              << " trading session complete.\033[0m  "
              << assets.size() << " assets updated.\n";
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
