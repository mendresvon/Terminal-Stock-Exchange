#pragma once

#include "engine/MarketEngine.hpp"

// ─────────────────────────────────────────────────────────────────
//  Menu  —  Main interactive loop and sub-menu dispatcher
// ─────────────────────────────────────────────────────────────────

class Menu {
private:
    MarketEngine& engine;
    bool          running;

    // ── Sub-menu actions ──────────────────────────────────────────
    void showMarket();
    void showPortfolio();
    void showTradeMenu();
    void showAdminPanel();
    void showTradeHistory();   ///< Epic 3: timestamped transaction log
    void nextTradingDay();
    void handleLogin();
    void handleRegister();     ///< Epic 3: new account registration
    void handleLogout();

    // ── Trade workflow ────────────────────────────────────────────
    /// Shows asset detail panel + chart, then offers Buy/Sell.
    void viewAsset(const std::string& symbol);
    void promptBuy (const std::string& symbol);
    void promptSell(const std::string& symbol);

    // ── Input helpers ─────────────────────────────────────────────
    /// Reads an integer in [min, max], re-prompting on bad input.
    int  getMenuChoice(int min, int max);

    /// Flushes stdin and waits for the user to press Enter.
    void waitForEnter();

public:
    explicit Menu(MarketEngine& engine);

    /// Starts the main menu loop (blocking until the user quits).
    void run();
};
