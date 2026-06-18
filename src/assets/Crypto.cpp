#include "Crypto.hpp"

Crypto::Crypto(const std::string& symbol,
               const std::string& name,
               double             initialPrice,
               double             volatilityFactor,
               double             tradingFeeRate)
    : FinancialAsset(symbol, name, initialPrice, AssetType::CRYPTO)
    , volatilityFactor(volatilityFactor)
    , tradingFeeRate(tradingFeeRate)
    , tradesAroundClock(true)
{}

double Crypto::calculateVolatility() const {
    return volatilityFactor;
}

double Crypto::getTradingFee() const {
    return tradingFeeRate;
}
