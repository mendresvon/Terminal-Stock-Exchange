#include "AdminAccount.hpp"
#include "engine/MarketEngine.hpp"
#include <iostream>
#include <filesystem>

AdminAccount::AdminAccount(const std::string& username, const std::string& password)
    : Account(username, password, 0.0) // Admins hold no trading cash
{}

bool AdminAccount::authenticate(const std::string& password) const {
    return passwordHash == hashPassword(password);
}

std::string AdminAccount::getAccountType() const {
    return "ADMIN";
}

void AdminAccount::addAsset(MarketEngine& engine, std::shared_ptr<FinancialAsset> asset) {
    engine.addAsset(std::move(asset));
}

void AdminAccount::removeAsset(MarketEngine& engine, const std::string& symbol) {
    engine.removeAsset(symbol);
}

void AdminAccount::resetSimulation(MarketEngine& engine) {
    engine.clearAssets();
    engine.seedDefaultAssets();
    engine.clearAccounts();
    engine.setCurrentDay(1);

    // Wipe physical persistent files
    std::error_code ec;
    std::filesystem::remove("data/trade_logs.txt", ec);
    std::filesystem::remove("data/market_data.txt", ec);
    std::filesystem::remove("data/accounts.txt", ec);

    // Save the newly reset default state
    engine.save();
}
