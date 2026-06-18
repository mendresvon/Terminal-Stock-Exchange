#include "PlayerTrader.hpp"
#include <iostream>

PlayerTrader::PlayerTrader(const std::string& username,
                           const std::string& password,
                           double             initialCash)
    : Account(username, password, initialCash)
{}

bool PlayerTrader::authenticate(const std::string& password) const {
    return passwordHash == hashPassword(password);
}

std::string PlayerTrader::getAccountType() const {
    return "PLAYER";
}

bool PlayerTrader::buy(const std::string& symbol,
                       int                quantity,
                       double             price,
                       double             feeRate)
{
    // TODO: Epic 2 — deduct cash + fee, add to portfolio, log transaction
    (void)feeRate;
    std::cout << "  [STUB] BUY " << quantity << " x " << symbol
              << " @ $" << price << "\n";
    return false;
}

bool PlayerTrader::sell(const std::string& symbol,
                        int                quantity,
                        double             price,
                        double             feeRate)
{
    // TODO: Epic 2 — credit cash − fee, remove from portfolio, log transaction
    (void)feeRate;
    std::cout << "  [STUB] SELL " << quantity << " x " << symbol
              << " @ $" << price << "\n";
    return false;
}
