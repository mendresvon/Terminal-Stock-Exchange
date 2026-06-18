#include "Stock.hpp"

Stock::Stock(const std::string& symbol,
             const std::string& name,
             double             initialPrice,
             double             volatilityFactor,
             double             dividendYield)
    : FinancialAsset(symbol, name, initialPrice, AssetType::STOCK)
    , volatilityFactor(volatilityFactor)
    , dividendYield(dividendYield)
{}

double Stock::calculateVolatility() const {
    return volatilityFactor;
}

double Stock::getTradingFee() const {
    return 0.001; // 0.1 % flat brokerage fee
}
