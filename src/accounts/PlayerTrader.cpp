#include "PlayerTrader.hpp"

#include <string>

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

// ─────────────────────────────────────────────────────────────────────────
//  buy()
//
//  Cost model
//  ──────────
//  totalCost = price × qty + (price × qty × feeRate)
//
//  Cost-basis update (weighted average)
//  ─────────────────────────────────────
//  newAvgCost = (prevAvgCost × prevQty + price × qty) / (prevQty + qty)
// ─────────────────────────────────────────────────────────────────────────

bool PlayerTrader::buy(const std::string& symbol,
                       int                quantity,
                       double             price,
                       double             feeRate,
                       int                day)
{
    if (quantity <= 0 || price <= 0.0) return false;

    double tradeValue = price * static_cast<double>(quantity);
    double fee        = tradeValue * feeRate;
    double totalCost  = tradeValue + fee;

    if (cashBalance < totalCost) return false;  // insufficient funds

    // Update weighted-average cost basis
    int    prevQty  = portfolio.count(symbol)    ? portfolio.at(symbol)    : 0;
    double prevCost = avgCostBasis.count(symbol) ? avgCostBasis.at(symbol) : 0.0;

    int    newQty     = prevQty + quantity;
    double newAvgCost = (prevCost * static_cast<double>(prevQty)
                       + price    * static_cast<double>(quantity))
                       / static_cast<double>(newQty);

    portfolio[symbol] = newQty;
    updateCostBasis(symbol, newQty, newAvgCost);
    cashBalance -= totalCost;

    // Audit log
    addTradeRecord({ symbol, "BUY", price, quantity, "Day " + std::to_string(day) });

    return true;
}

// ─────────────────────────────────────────────────────────────────────────
//  sell()
//
//  Proceeds model
//  ──────────────
//  proceeds = price × qty − (price × qty × feeRate)
//
//  Cost-basis: average cost per unit stays unchanged on sell (avg-cost method).
//  Entry is erased when the position reaches zero.
// ─────────────────────────────────────────────────────────────────────────

bool PlayerTrader::sell(const std::string& symbol,
                        int                quantity,
                        double             price,
                        double             feeRate,
                        int                day)
{
    if (quantity <= 0 || price <= 0.0) return false;

    auto it = portfolio.find(symbol);
    if (it == portfolio.end() || it->second < quantity) return false; // not enough shares

    double tradeValue = price * static_cast<double>(quantity);
    double fee        = tradeValue * feeRate;
    double proceeds   = tradeValue - fee;

    it->second -= quantity;
    if (it->second == 0) {
        portfolio.erase(it);
        updateCostBasis(symbol, 0, 0.0);
    }

    cashBalance += proceeds;

    // Audit log
    addTradeRecord({ symbol, "SELL", price, quantity, "Day " + std::to_string(day) });

    return true;
}
