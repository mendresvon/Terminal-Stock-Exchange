#include "AdminAccount.hpp"
#include <iostream>

AdminAccount::AdminAccount(const std::string& username, const std::string& password)
    : Account(username, password, 0.0) // Admins hold no trading cash
{}

bool AdminAccount::authenticate(const std::string& password) const {
    return passwordHash == hashPassword(password);
}

std::string AdminAccount::getAccountType() const {
    return "ADMIN";
}

void AdminAccount::addAsset(std::shared_ptr<FinancialAsset> asset) {
    // TODO: Epic 2 — delegate to MarketEngine reference
    std::cout << "  [STUB] Admin: add asset " << asset->getSymbol() << "\n";
}

void AdminAccount::removeAsset(const std::string& symbol) {
    // TODO: Epic 2 — delegate to MarketEngine reference
    std::cout << "  [STUB] Admin: remove asset " << symbol << "\n";
}

void AdminAccount::resetSimulation() {
    // TODO: Epic 3 — wipe data/ files, re-seed default assets
    std::cout << "  [STUB] Admin: reset simulation\n";
}
