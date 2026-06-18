#include "MarketEngine.hpp"
#include "assets/Stock.hpp"
#include "assets/Crypto.hpp"
#include "assets/ETF.hpp"

#include <iomanip>
#include <iostream>

MarketEngine::MarketEngine() : currentDay(1) {}

// ── Asset registry ────────────────────────────────────────────────────────────

void MarketEngine::addAsset(std::shared_ptr<FinancialAsset> asset) {
    assets[asset->getSymbol()] = std::move(asset);
}

void MarketEngine::removeAsset(const std::string& symbol) {
    assets.erase(symbol);
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

void MarketEngine::listAssets() const {
    // Header row
    std::cout << "\n"
              << "  " << std::left
              << std::setw(6)  << "SYMBOL"
              << std::setw(22) << "NAME"
              << std::setw(12) << "PRICE"
              << std::setw(8)  << "TYPE"
              << std::setw(10) << "VOL"
              << "\n";
    std::cout << "  " << std::string(58, '-') << "\n";

    for (const auto& [sym, asset] : assets) {
        const char* typeTag = "STOCK";
        if (asset->getAssetType() == AssetType::CRYPTO) typeTag = "CRYPTO";
        if (asset->getAssetType() == AssetType::ETF)    typeTag = "ETF";

        std::cout << "  "
                  << std::left  << std::setw(6)  << asset->getSymbol()
                  << std::left  << std::setw(22) << asset->getName()
                  << std::right << std::setw(8)  << std::fixed
                  << std::setprecision(2) << asset->getCurrentPrice()
                  << "    "
                  << std::left  << std::setw(8)  << typeTag
                  << std::right << std::setw(6)  << std::fixed
                  << std::setprecision(3) << asset->calculateVolatility()
                  << "\n";
    }
    std::cout << "\n";
}

// ── Session management ────────────────────────────────────────────────────────

void MarketEngine::setCurrentUser(std::shared_ptr<Account> user) {
    currentUser = std::move(user);
}

std::shared_ptr<Account> MarketEngine::getCurrentUser() const {
    return currentUser;
}

// ── Simulation control ────────────────────────────────────────────────────────

void MarketEngine::stepDay() {
    // TODO: Epic 2 — random-walk price updates + 10 % chance news event
    ++currentDay;
    std::cout << "\n  [Day " << currentDay << "] Market is open.\n"
              << "  (Price simulation coming in Epic 2)\n";
}

// ── Seeding ───────────────────────────────────────────────────────────────────

void MarketEngine::seedDefaultAssets() {
    // Stocks
    addAsset(std::make_shared<Stock>("AAPL", "Apple Inc.",        178.50, 0.020, 0.005));
    addAsset(std::make_shared<Stock>("TSLA", "Tesla Inc.",        245.00, 0.035, 0.000));
    addAsset(std::make_shared<Stock>("MSFT", "Microsoft Corp.",   415.20, 0.018, 0.009));
    addAsset(std::make_shared<Stock>("NVDA", "NVIDIA Corp.",      875.00, 0.040, 0.001));
    addAsset(std::make_shared<Stock>("AMZN", "Amazon.com Inc.",   185.60, 0.022, 0.000));

    // Crypto
    addAsset(std::make_shared<Crypto>("BTC",  "Bitcoin",          43'500.00, 0.060, 0.005));
    addAsset(std::make_shared<Crypto>("ETH",  "Ethereum",          2'280.00, 0.075, 0.005));
    addAsset(std::make_shared<Crypto>("SOL",  "Solana",              105.00, 0.090, 0.006));

    // ETFs
    addAsset(std::make_shared<ETF>("SPY", "S&P 500 ETF",
        475.50, std::vector<std::string>{"AAPL","MSFT","AMZN","NVDA"}, 0.008));
    addAsset(std::make_shared<ETF>("QQQ", "NASDAQ-100 ETF",
        390.80, std::vector<std::string>{"AAPL","MSFT","NVDA"},        0.010));
}
