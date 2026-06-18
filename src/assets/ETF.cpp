#include "ETF.hpp"

ETF::ETF(const std::string&       symbol,
         const std::string&       name,
         double                   initialPrice,
         std::vector<std::string> basket,
         double                   volatilityFactor)
    : FinancialAsset(symbol, name, initialPrice, AssetType::ETF)
    , volatilityFactor(volatilityFactor)
    , basketSymbols(std::move(basket))
{}

double ETF::calculateVolatility() const {
    return volatilityFactor;
}

double ETF::getTradingFee() const {
    return 0.0003; // 0.03 % — very low expense ratio for ETFs
}
