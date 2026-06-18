#include "Menu.hpp"
#include "Terminal.hpp"
#include "accounts/PlayerTrader.hpp"
#include "assets/Stock.hpp"
#include "assets/Crypto.hpp"
#include "assets/ETF.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────
//  Formatting helpers (file-local)
// ─────────────────────────────────────────────────────────────────────────

namespace {

/// Format a double as a dollar amount, e.g. "$12,345.67"
std::string fmtDollar(double v, int decimals = 2) {
    std::ostringstream oss;
    oss << "$" << std::fixed << std::setprecision(decimals) << v;
    return oss.str();
}

/// Format a double as a percentage, e.g. "+3.45%"
std::string fmtPct(double v) {
    std::ostringstream oss;
    oss << (v >= 0 ? "+" : "") << std::fixed << std::setprecision(2) << v << "%";
    return oss.str();
}

/// Left-pad a string to width (for manual colored-cell layout)
std::string padRight(const std::string& s, int width) {
    if (static_cast<int>(s.size()) >= width) return s;
    return s + std::string(width - s.size(), ' ');
}

/// Convert string to uppercase in-place
void toUpper(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::toupper(c); });
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────────────────────────────────

Menu::Menu(MarketEngine& eng) : engine(eng), running(true) {}

// ─────────────────────────────────────────────────────────────────────────
//  Main loop
// ─────────────────────────────────────────────────────────────────────────

void Menu::run() {
    handleLogin();

    while (running) {
        Terminal::clearScreen();
        Terminal::printHeader("TERMINAL STOCK EXCHANGE");

        auto user = engine.getCurrentUser();
        if (user) {
            const char* acctColor =
                (user->getAccountType() == "ADMIN") ? Terminal::MAGENTA
                                                    : Terminal::BRIGHT_CYAN;
            std::cout << "\n  "
                      << Terminal::DIM   << "User: " << Terminal::RESET
                      << Terminal::BOLD  << user->getUsername() << Terminal::RESET
                      << "  "
                      << acctColor << "[" << user->getAccountType() << "]" << Terminal::RESET
                      << "    "
                      << Terminal::DIM   << "Cash: " << Terminal::RESET
                      << Terminal::BRIGHT_GREEN
                      << std::fixed << std::setprecision(2)
                      << fmtDollar(user->getCashBalance()) << Terminal::RESET
                      << "    "
                      << Terminal::DIM   << "Day: " << Terminal::RESET
                      << Terminal::YELLOW << engine.getCurrentDay() << Terminal::RESET
                      << "\n";
        }

        std::cout << "\n";
        Terminal::printDivider();
        std::cout << "\n";

        bool isAdmin = user && (user->getAccountType() == "ADMIN");

        std::cout << "  " << Terminal::BOLD << Terminal::CYAN  << " 1" << Terminal::RESET << "  View Market\n";
        std::cout << "  " << Terminal::BOLD << Terminal::CYAN  << " 2" << Terminal::RESET << "  My Portfolio\n";
        std::cout << "  " << Terminal::BOLD << Terminal::CYAN  << " 3" << Terminal::RESET << "  Trade\n";
        std::cout << "  " << Terminal::BOLD << Terminal::CYAN  << " 4" << Terminal::RESET << "  Next Trading Day\n";
        if (isAdmin) {
            std::cout << "  " << Terminal::BOLD << Terminal::MAGENTA << " 5" << Terminal::RESET << "  Admin Panel\n";
        }
        std::cout << "  " << Terminal::BOLD << Terminal::YELLOW << " 6" << Terminal::RESET << "  Logout\n";
        std::cout << "  " << Terminal::BOLD << Terminal::RED    << " 0" << Terminal::RESET << "  Quit\n";

        std::cout << "\n";
        Terminal::printDivider();
        std::cout << "\n  " << Terminal::DIM << "Enter choice: " << Terminal::RESET;

        int choice = getMenuChoice(0, 6);

        switch (choice) {
            case 1: showMarket();     break;
            case 2: showPortfolio();  break;
            case 3: showTradeMenu();  break;
            case 4: nextTradingDay(); break;
            case 5: if (isAdmin) showAdminPanel(); break;
            case 6: handleLogout();   break;
            case 0: running = false;  break;
            default: break;
        }
    }

    Terminal::clearScreen();
    Terminal::printHeader("GOODBYE");
    std::cout << "\n  " << Terminal::BRIGHT_CYAN
              << "Thank you for trading with Terminal Stock Exchange!"
              << Terminal::RESET << "\n\n";
}

// ─────────────────────────────────────────────────────────────────────────
//  showMarket()
// ─────────────────────────────────────────────────────────────────────────

void Menu::showMarket() {
    Terminal::clearScreen();
    Terminal::printHeader("MARKET OVERVIEW");
    engine.listAssets();
    waitForEnter();
}

// ─────────────────────────────────────────────────────────────────────────
//  showPortfolio()  —  Full portfolio table with coloured P&L
// ─────────────────────────────────────────────────────────────────────────

void Menu::showPortfolio() {
    Terminal::clearScreen();
    Terminal::printHeader("MY PORTFOLIO");

    auto user = engine.getCurrentUser();
    if (!user) { waitForEnter(); return; }

    const auto& portfolio  = user->getPortfolio();
    const auto& costBasis  = user->getAvgCostBasis();

    if (portfolio.empty()) {
        std::cout << "\n  " << Terminal::DIM
                  << "Your portfolio is empty.  Use option  3  to make your first trade!"
                  << Terminal::RESET << "\n";
        waitForEnter();
        return;
    }

    // ── Column widths (manually managed so we can inject ANSI into P&L) ──
    const int W_SYM  = 7;
    const int W_QTY  = 7;
    const int W_AVG  = 12;
    const int W_PRC  = 12;
    const int W_VAL  = 13;
    const int W_PNL  = 13;
    const int W_PCT  = 9;

    // Top border
    auto hRule = [&](const char* l, const char* m, const char* r) {
        std::cout << Terminal::CYAN << l;
        for (int w : {W_SYM, W_QTY, W_AVG, W_PRC, W_VAL, W_PNL, W_PCT}) {
            for (int j = 0; j < w + 2; ++j) std::cout << "\u2550";
            std::cout << (w == W_PCT ? r : m);
        }
        std::cout << Terminal::RESET << "\n";
    };

    hRule("\u2554", "\u2566", "\u2557");

    // Header row
    auto hcell = [&](const std::string& s, int w) {
        std::cout << " " << Terminal::BOLD
                  << std::left << std::setw(w) << s
                  << Terminal::RESET << Terminal::CYAN << " \u2551" << Terminal::RESET;
    };
    std::cout << Terminal::CYAN << "\u2551" << Terminal::RESET;
    hcell("SYMBOL", W_SYM);
    hcell("QTY",    W_QTY);
    hcell("AVG COST", W_AVG);
    hcell("PRICE",  W_PRC);
    hcell("VALUE",  W_VAL);
    hcell("P&L",    W_PNL);
    hcell("P&L %",  W_PCT);
    std::cout << "\n";

    hRule("\u2560", "\u256c", "\u2563");

    // Data rows
    double totalValue = 0.0;
    double totalCost  = 0.0;

    for (const auto& [sym, qty] : portfolio) {
        auto   asset       = engine.getAsset(sym);
        double curPrice    = asset ? asset->getCurrentPrice() : 0.0;
        double avgCost     = costBasis.count(sym) ? costBasis.at(sym) : 0.0;
        double posValue    = curPrice  * qty;
        double posCost     = avgCost   * qty;
        double pnl         = posValue  - posCost;
        double pnlPct      = (posCost > 0.0) ? pnl / posCost * 100.0 : 0.0;

        totalValue += posValue;
        totalCost  += posCost;

        const char* pnlColor = (pnl >= 0.0) ? Terminal::BRIGHT_GREEN : Terminal::BRIGHT_RED;
        std::string pnlStr   = (pnl >= 0.0 ? "+" : "") + fmtDollar(pnl);
        std::string pctStr   = fmtPct(pnlPct);

        // Print row — manually pad colored cells to maintain alignment
        auto cell = [&](const std::string& s, int w) {
            std::cout << " " << std::left << std::setw(w) << s
                      << Terminal::CYAN << " \u2551" << Terminal::RESET;
        };
        auto colorCell = [&](const std::string& s, int w, const char* col) {
            std::string padded = padRight(s, w);
            std::cout << " " << col << padded << Terminal::RESET
                      << Terminal::CYAN << " \u2551" << Terminal::RESET;
        };

        std::cout << Terminal::CYAN << "\u2551" << Terminal::RESET;
        cell(sym,                           W_SYM);
        cell(std::to_string(qty),           W_QTY);
        cell(fmtDollar(avgCost),            W_AVG);
        cell(fmtDollar(curPrice),           W_PRC);
        cell(fmtDollar(posValue),           W_VAL);
        colorCell(pnlStr,    W_PNL, pnlColor);
        colorCell(pctStr,    W_PCT, pnlColor);
        std::cout << "\n";
    }

    hRule("\u255a", "\u2569", "\u255d");

    // ── Portfolio summary ─────────────────────────────────────────────────
    double totalPnL    = totalValue - totalCost;
    double netWorth    = totalValue + user->getCashBalance();
    const char* pnlCol = (totalPnL >= 0.0) ? Terminal::BRIGHT_GREEN : Terminal::BRIGHT_RED;

    std::cout << "\n"
              << "  " << Terminal::DIM    << "Cash Balance:   " << Terminal::RESET
              << Terminal::BRIGHT_GREEN   << fmtDollar(user->getCashBalance()) << Terminal::RESET << "\n"
              << "  " << Terminal::DIM    << "Holdings Value: " << Terminal::RESET
              << Terminal::BRIGHT_CYAN    << fmtDollar(totalValue) << Terminal::RESET << "\n"
              << "  " << Terminal::DIM    << "Total P&L:      " << Terminal::RESET
              << pnlCol                   << (totalPnL >= 0 ? "+" : "") << fmtDollar(totalPnL) << Terminal::RESET << "\n"
              << "  " << Terminal::BOLD   << "Net Worth:      "
              << Terminal::YELLOW         << fmtDollar(netWorth) << Terminal::RESET << "\n\n";

    waitForEnter();
}

// ─────────────────────────────────────────────────────────────────────────
//  showTradeMenu()
// ─────────────────────────────────────────────────────────────────────────

void Menu::showTradeMenu() {
    Terminal::clearScreen();
    Terminal::printHeader("TRADE");
    engine.listAssets();

    std::cout << "  Enter ticker symbol (or 0 to go back): ";
    std::string sym;
    std::cin >> sym;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    toUpper(sym);

    if (sym == "0" || sym == "BACK") return;

    auto asset = engine.getAsset(sym);
    if (!asset) {
        Terminal::printError("Unknown symbol: " + sym);
        waitForEnter();
        return;
    }

    viewAsset(sym);
}

// ─────────────────────────────────────────────────────────────────────────
//  viewAsset()  —  Asset detail panel + chart + trade options
// ─────────────────────────────────────────────────────────────────────────

void Menu::viewAsset(const std::string& symbol) {
    while (true) {
        auto asset = engine.getAsset(symbol);
        if (!asset) return;

        Terminal::clearScreen();

        const auto& hist    = asset->getPriceHistory();
        double       cur    = asset->getCurrentPrice();
        double       prev   = (hist.size() >= 2) ? *(hist.end() - 2) : cur;
        double       chg    = cur - prev;
        double       chgPct = (prev > 0.0) ? chg / prev * 100.0 : 0.0;
        const char*  chgCol = (chg >= 0.0) ? Terminal::BRIGHT_GREEN : Terminal::BRIGHT_RED;
        const char*  arrow  = (chg >= 0.0) ? "\u25b2" : "\u25bc";

        // Header
        std::string typeStr;
        switch (asset->getAssetType()) {
            case AssetType::STOCK:  typeStr = "STOCK";  break;
            case AssetType::CRYPTO: typeStr = "CRYPTO"; break;
            case AssetType::ETF:    typeStr = "ETF";    break;
        }
        Terminal::printHeader(asset->getName() + "  [" + symbol + "]");

        std::cout << "\n"
                  << "  " << Terminal::DIM    << "Type:    " << Terminal::RESET
                  << Terminal::BOLD << typeStr << Terminal::RESET << "\n"
                  << "  " << Terminal::DIM    << "Price:   " << Terminal::RESET
                  << Terminal::BOLD << chgCol
                  << fmtDollar(cur) << "   " << arrow << " "
                  << (chg >= 0 ? "+" : "") << std::fixed << std::setprecision(2) << chg
                  << "  (" << fmtPct(chgPct) << ")"
                  << Terminal::RESET << "\n"
                  << "  " << Terminal::DIM    << "Vol:     " << Terminal::RESET
                  << std::fixed << std::setprecision(3) << asset->calculateVolatility() * 100.0 << "% / day\n"
                  << "  " << Terminal::DIM    << "Fee:     " << Terminal::RESET
                  << std::fixed << std::setprecision(2) << asset->getTradingFee() * 100.0 << "%\n"
                  << "  " << Terminal::DIM    << "History: " << Terminal::RESET
                  << hist.size() << " days\n";

        // Check player holdings
        auto user   = engine.getCurrentUser();
        auto player = std::dynamic_pointer_cast<PlayerTrader>(user);
        if (player) {
            const auto& pf = player->getPortfolio();
            if (pf.count(symbol)) {
                int qty = pf.at(symbol);
                std::cout << "  " << Terminal::DIM << "Held:    " << Terminal::RESET
                          << Terminal::YELLOW << qty << " units" << Terminal::RESET << "\n";
            }
        }

        // Chart
        std::cout << "\n";
        Terminal::printDivider();
        Terminal::printChart(hist);
        Terminal::printDivider();

        // Trade options
        std::cout << "\n"
                  << "  " << Terminal::BOLD << Terminal::BRIGHT_GREEN << " 1" << Terminal::RESET << "  Buy\n"
                  << "  " << Terminal::BOLD << Terminal::BRIGHT_RED   << " 2" << Terminal::RESET << "  Sell\n"
                  << "  " << Terminal::BOLD << Terminal::DIM          << " 0" << Terminal::RESET << "  Back\n"
                  << "\n  Enter choice: ";

        int choice = getMenuChoice(0, 2);
        if      (choice == 1) promptBuy(symbol);
        else if (choice == 2) promptSell(symbol);
        else                  return;
    }
}

// ─────────────────────────────────────────────────────────────────────────
//  promptBuy()
// ─────────────────────────────────────────────────────────────────────────

void Menu::promptBuy(const std::string& symbol) {
    auto asset  = engine.getAsset(symbol);
    auto user   = engine.getCurrentUser();
    auto player = std::dynamic_pointer_cast<PlayerTrader>(user);

    if (!asset || !player) {
        Terminal::printError("Buy unavailable — not logged in as a player.");
        waitForEnter();
        return;
    }

    double price   = asset->getCurrentPrice();
    double feeRate = asset->getTradingFee();
    double cash    = player->getCashBalance();
    int    maxQty  = (feeRate < 1.0)
                     ? static_cast<int>(cash / (price * (1.0 + feeRate)))
                     : 0;

    std::cout << "\n";
    Terminal::printDivider();
    std::cout << "  " << Terminal::DIM    << "Price:      " << Terminal::RESET
              << Terminal::BOLD << fmtDollar(price) << Terminal::RESET << "\n"
              << "  " << Terminal::DIM    << "Fee rate:   " << Terminal::RESET
              << std::fixed << std::setprecision(2) << feeRate * 100.0 << "%\n"
              << "  " << Terminal::DIM    << "Cash avail: " << Terminal::RESET
              << Terminal::BRIGHT_GREEN   << fmtDollar(cash) << Terminal::RESET << "\n"
              << "  " << Terminal::DIM    << "Max qty:    " << Terminal::RESET
              << Terminal::YELLOW << maxQty << Terminal::RESET << "\n\n"
              << "  Enter quantity to buy (0 to cancel): ";

    int qty{};
    std::cin >> qty;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (qty <= 0) { std::cout << "  Cancelled.\n"; waitForEnter(); return; }

    double subTotal   = price * qty;
    double fee        = subTotal * feeRate;
    double totalCost  = subTotal + fee;

    std::cout << "\n";
    Terminal::printDivider();
    std::cout << "  " << Terminal::BOLD << "ORDER SUMMARY\n" << Terminal::RESET
              << "  Buying    : " << qty << " \u00d7 " << symbol << "\n"
              << "  Sub-total : " << fmtDollar(subTotal) << "\n"
              << "  Fee       : " << fmtDollar(fee) << "\n"
              << "  " << Terminal::BOLD << "Total cost: " << Terminal::BRIGHT_CYAN
              << fmtDollar(totalCost) << Terminal::RESET << "\n\n"
              << "  Confirm? (y/n): ";

    char confirm{};
    std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (confirm == 'y' || confirm == 'Y') {
        if (player->buy(symbol, qty, price, feeRate, engine.getCurrentDay())) {
            Terminal::printSuccess("Bought " + std::to_string(qty) + " " + symbol
                                   + " @ " + fmtDollar(price)
                                   + "  (fee: " + fmtDollar(fee) + ")");
        } else {
            Terminal::printError("Transaction failed — insufficient funds.");
        }
    } else {
        std::cout << "  " << Terminal::DIM << "Order cancelled." << Terminal::RESET << "\n";
    }

    waitForEnter();
}

// ─────────────────────────────────────────────────────────────────────────
//  promptSell()
// ─────────────────────────────────────────────────────────────────────────

void Menu::promptSell(const std::string& symbol) {
    auto asset  = engine.getAsset(symbol);
    auto user   = engine.getCurrentUser();
    auto player = std::dynamic_pointer_cast<PlayerTrader>(user);

    if (!asset || !player) {
        Terminal::printError("Sell unavailable — not logged in as a player.");
        waitForEnter();
        return;
    }

    const auto& portfolio = player->getPortfolio();
    if (!portfolio.count(symbol) || portfolio.at(symbol) == 0) {
        Terminal::printError("You hold no position in " + symbol + ".");
        waitForEnter();
        return;
    }

    int    heldQty  = portfolio.at(symbol);
    double price    = asset->getCurrentPrice();
    double feeRate  = asset->getTradingFee();

    // Show cost basis for P&L preview
    const auto& costBasis = player->getAvgCostBasis();
    double avgCost = costBasis.count(symbol) ? costBasis.at(symbol) : 0.0;

    std::cout << "\n";
    Terminal::printDivider();
    std::cout << "  " << Terminal::DIM    << "Held:       " << Terminal::RESET
              << Terminal::YELLOW << heldQty << " units" << Terminal::RESET << "\n"
              << "  " << Terminal::DIM    << "Avg cost:   " << Terminal::RESET
              << fmtDollar(avgCost) << "\n"
              << "  " << Terminal::DIM    << "Cur price:  " << Terminal::RESET
              << Terminal::BOLD << fmtDollar(price) << Terminal::RESET << "\n"
              << "  " << Terminal::DIM    << "Fee rate:   " << Terminal::RESET
              << std::fixed << std::setprecision(2) << feeRate * 100.0 << "%\n\n"
              << "  Enter quantity to sell (0 to cancel, max " << heldQty << "): ";

    int qty{};
    std::cin >> qty;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (qty <= 0) { std::cout << "  Cancelled.\n"; waitForEnter(); return; }
    if (qty > heldQty) {
        Terminal::printError("Cannot sell more than you hold (" + std::to_string(heldQty) + ").");
        waitForEnter();
        return;
    }

    double subTotal  = price * qty;
    double fee       = subTotal * feeRate;
    double proceeds  = subTotal - fee;
    double pnl       = (price - avgCost) * qty - fee;
    const char* pnlCol = (pnl >= 0.0) ? Terminal::BRIGHT_GREEN : Terminal::BRIGHT_RED;

    std::cout << "\n";
    Terminal::printDivider();
    std::cout << "  " << Terminal::BOLD << "ORDER SUMMARY\n" << Terminal::RESET
              << "  Selling   : " << qty << " \u00d7 " << symbol << "\n"
              << "  Sub-total : " << fmtDollar(subTotal) << "\n"
              << "  Fee       : " << fmtDollar(fee) << "\n"
              << "  " << Terminal::BOLD << "Proceeds:   " << Terminal::BRIGHT_CYAN
              << fmtDollar(proceeds) << Terminal::RESET << "\n"
              << "  Realised P&L: " << pnlCol
              << (pnl >= 0 ? "+" : "") << fmtDollar(pnl) << Terminal::RESET << "\n\n"
              << "  Confirm? (y/n): ";

    char confirm{};
    std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (confirm == 'y' || confirm == 'Y') {
        if (player->sell(symbol, qty, price, feeRate, engine.getCurrentDay())) {
            Terminal::printSuccess("Sold " + std::to_string(qty) + " " + symbol
                                   + " @ " + fmtDollar(price)
                                   + "  (P&L: " + (pnl >= 0 ? "+" : "") + fmtDollar(pnl) + ")");
        } else {
            Terminal::printError("Transaction failed.");
        }
    } else {
        std::cout << "  " << Terminal::DIM << "Order cancelled." << Terminal::RESET << "\n";
    }

    waitForEnter();
}

// ─────────────────────────────────────────────────────────────────────────
//  nextTradingDay()
// ─────────────────────────────────────────────────────────────────────────

void Menu::nextTradingDay() {
    Terminal::clearScreen();
    Terminal::printHeader("ADVANCE TRADING DAY");
    std::cout << "\n";
    engine.stepDay();
    waitForEnter();
}

// ─────────────────────────────────────────────────────────────────────────
//  showAdminPanel()
// ─────────────────────────────────────────────────────────────────────────

void Menu::showAdminPanel() {
    while (true) {
        Terminal::clearScreen();
        Terminal::printHeader("ADMIN PANEL");

        std::cout << "\n"
                  << "  " << Terminal::MAGENTA << Terminal::BOLD << " 1" << Terminal::RESET
                  << "  Add New Asset\n"
                  << "  " << Terminal::MAGENTA << Terminal::BOLD << " 2" << Terminal::RESET
                  << "  Remove Asset\n"
                  << "  " << Terminal::RED     << Terminal::BOLD << " 3" << Terminal::RESET
                  << "  Reset Simulation\n"
                  << "  " << Terminal::DIM     << " 0" << Terminal::RESET
                  << "  Back\n\n"
                  << "  Enter choice: ";

        int choice = getMenuChoice(0, 3);

        if (choice == 0) return;

        // ── Add Asset ──────────────────────────────────────────────────
        if (choice == 1) {
            Terminal::clearScreen();
            Terminal::printHeader("ADD NEW ASSET");

            std::cout << "\n  Asset type:\n"
                      << "  1. Stock   2. Crypto   3. ETF\n"
                      << "  Enter type: ";
            int typeChoice = getMenuChoice(1, 3);

            std::cout << "  Ticker symbol (e.g. GOOG): ";
            std::string sym; std::cin >> sym; toUpper(sym);
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::cout << "  Full name: ";
            std::string name; std::getline(std::cin, name);

            std::cout << "  Initial price ($): ";
            double price{}; std::cin >> price;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (engine.getAsset(sym)) {
                Terminal::printWarning(sym + " already exists — overwriting.");
            }

            if (typeChoice == 1) {
                std::cout << "  Volatility (e.g. 0.02): ";
                double vol{}; std::cin >> vol;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                engine.addAsset(std::make_shared<Stock>(sym, name, price, vol));
            } else if (typeChoice == 2) {
                std::cout << "  Volatility (e.g. 0.07): ";
                double vol{}; std::cin >> vol;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                engine.addAsset(std::make_shared<Crypto>(sym, name, price, vol));
            } else {
                engine.addAsset(std::make_shared<ETF>(sym, name, price));
            }

            Terminal::printSuccess("Asset " + sym + " added to market.");
            waitForEnter();
        }

        // ── Remove Asset ────────────────────────────────────────────────
        else if (choice == 2) {
            Terminal::clearScreen();
            Terminal::printHeader("REMOVE ASSET");
            engine.listAssets();

            std::cout << "  Enter ticker to remove (0 to cancel): ";
            std::string sym; std::cin >> sym; toUpper(sym);
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (sym == "0") continue;

            if (!engine.getAsset(sym)) {
                Terminal::printError("Symbol " + sym + " not found.");
            } else {
                engine.removeAsset(sym);
                Terminal::printSuccess(sym + " delisted from market.");
            }
            waitForEnter();
        }

        // ── Reset Simulation ───────────────────────────────────────────
        else if (choice == 3) {
            Terminal::clearScreen();
            Terminal::printHeader("RESET SIMULATION");
            Terminal::printWarning("This will wipe all asset price history and re-seed the market.");
            std::cout << "\n  Type  CONFIRM  to proceed (or anything else to cancel): ";

            std::string confirm; std::getline(std::cin, confirm);

            if (confirm == "CONFIRM") {
                engine.clearAssets();
                engine.seedDefaultAssets();
                Terminal::printSuccess("Market reset to defaults.");
            } else {
                std::cout << "  " << Terminal::DIM << "Reset cancelled." << Terminal::RESET << "\n";
            }
            waitForEnter();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────
//  handleLogin() / handleLogout()
// ─────────────────────────────────────────────────────────────────────────

void Menu::handleLogin() {
    Terminal::clearScreen();
    Terminal::printHeader("WELCOME TO TERMINAL STOCK EXCHANGE");

    std::cout << "\n  " << Terminal::DIM
              << "Full authentication system coming in Epic 3.\n"
              << "  Auto-logging in as demo player...\n"
              << Terminal::RESET << "\n";

    auto player = std::make_shared<PlayerTrader>("demo_trader", "password", 10'000.0);
    engine.setCurrentUser(player);

    std::cout << "  " << Terminal::BRIGHT_GREEN
              << "\u2713 Logged in as demo_trader  ($10,000.00 starting balance)"
              << Terminal::RESET << "\n";

    waitForEnter();
}

void Menu::handleLogout() {
    engine.setCurrentUser(nullptr);
    running = false;
}

// ─────────────────────────────────────────────────────────────────────────
//  Input helpers
// ─────────────────────────────────────────────────────────────────────────

int Menu::getMenuChoice(int min, int max) {
    int choice{};
    while (true) {
        std::cin >> choice;
        if (std::cin.eof()) {
            // stdin exhausted (piped/redirected input) — exit gracefully
            running = false;
            return min;
        }
        if (std::cin.fail() || choice < min || choice > max) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  " << Terminal::RED << "Enter a number between "
                      << min << " and " << max << ": " << Terminal::RESET;
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return choice;
        }
    }
}

void Menu::waitForEnter() {
    std::cout << "\n  " << Terminal::DIM << "Press Enter to continue..." << Terminal::RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
