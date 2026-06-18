#include "Menu.hpp"
#include "Terminal.hpp"
#include "accounts/PlayerTrader.hpp"

#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>

// ─────────────────────────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────────────────────────

Menu::Menu(MarketEngine& eng) : engine(eng), running(true) {}

// ─────────────────────────────────────────────────────────────────
//  Main loop
// ─────────────────────────────────────────────────────────────────

void Menu::run() {
    handleLogin();

    while (running) {
        Terminal::clearScreen();
        Terminal::printHeader("TERMINAL STOCK EXCHANGE");

        // Status bar
        auto user = engine.getCurrentUser();
        if (user) {
            const char* acctColor =
                (user->getAccountType() == "ADMIN") ? Terminal::MAGENTA
                                                    : Terminal::BRIGHT_CYAN;
            std::cout << "\n  "
                      << Terminal::DIM << "User: " << Terminal::RESET
                      << Terminal::BOLD << user->getUsername() << Terminal::RESET
                      << "  "
                      << acctColor << "[" << user->getAccountType() << "]" << Terminal::RESET
                      << "    "
                      << Terminal::DIM << "Cash: " << Terminal::RESET
                      << Terminal::BRIGHT_GREEN << "$"
                      << std::fixed << std::setprecision(2)
                      << user->getCashBalance() << Terminal::RESET
                      << "    "
                      << Terminal::DIM << "Day: " << Terminal::RESET
                      << Terminal::YELLOW << engine.getCurrentDay() << Terminal::RESET
                      << "\n";
        }

        std::cout << "\n";
        Terminal::printDivider();
        std::cout << "\n";

        bool isAdmin = user && (user->getAccountType() == "ADMIN");

        std::cout << "  " << Terminal::BOLD << Terminal::CYAN << " 1" << Terminal::RESET
                  << "  View Market\n";
        std::cout << "  " << Terminal::BOLD << Terminal::CYAN << " 2" << Terminal::RESET
                  << "  My Portfolio\n";
        std::cout << "  " << Terminal::BOLD << Terminal::CYAN << " 3" << Terminal::RESET
                  << "  Trade\n";
        std::cout << "  " << Terminal::BOLD << Terminal::CYAN << " 4" << Terminal::RESET
                  << "  Next Trading Day\n";

        if (isAdmin) {
            std::cout << "  " << Terminal::BOLD << Terminal::MAGENTA << " 5" << Terminal::RESET
                      << "  Admin Panel\n";
        }

        std::cout << "  " << Terminal::BOLD << Terminal::YELLOW << " 6" << Terminal::RESET
                  << "  Logout\n";
        std::cout << "  " << Terminal::BOLD << Terminal::RED    << " 0" << Terminal::RESET
                  << "  Quit\n";
        std::cout << "\n";
        Terminal::printDivider();
        std::cout << "\n  " << Terminal::DIM << "Enter choice: " << Terminal::RESET;

        int choice = getMenuChoice(0, 6);

        switch (choice) {
            case 1: showMarket();      break;
            case 2: showPortfolio();   break;
            case 3: showTradeMenu();   break;
            case 4: nextTradingDay();  break;
            case 5:
                if (isAdmin) showAdminPanel();
                break;
            case 6: handleLogout();    break;
            case 0: running = false;   break;
            default: break;
        }
    }

    // Exit screen
    Terminal::clearScreen();
    Terminal::printHeader("GOODBYE");
    std::cout << "\n"
              << "  " << Terminal::BRIGHT_CYAN
              << "Thank you for trading with Terminal Stock Exchange!"
              << Terminal::RESET
              << "\n\n";
}

// ─────────────────────────────────────────────────────────────────
//  Sub-menu implementations
// ─────────────────────────────────────────────────────────────────

void Menu::showMarket() {
    Terminal::clearScreen();
    Terminal::printHeader("MARKET OVERVIEW");
    engine.listAssets();
    waitForEnter();
}

void Menu::showPortfolio() {
    Terminal::clearScreen();
    Terminal::printHeader("MY PORTFOLIO");

    auto user = engine.getCurrentUser();
    if (!user) { waitForEnter(); return; }

    const auto& portfolio = user->getPortfolio();
    if (portfolio.empty()) {
        std::cout << "\n  " << Terminal::DIM
                  << "Your portfolio is empty. Use option 3 to make your first trade!"
                  << Terminal::RESET << "\n";
    } else {
        // TODO: Epic 2 — render full portfolio with P&L calculations
        std::cout << "\n  [Portfolio rendering — coming in Epic 2]\n";
    }

    waitForEnter();
}

void Menu::showTradeMenu() {
    Terminal::clearScreen();
    Terminal::printHeader("TRADE");
    std::cout << "\n  " << Terminal::DIM
              << "[Buy/Sell interface — coming in Epic 2]"
              << Terminal::RESET << "\n";
    waitForEnter();
}

void Menu::nextTradingDay() {
    Terminal::clearScreen();
    Terminal::printHeader("ADVANCE TRADING DAY");
    engine.stepDay();
    waitForEnter();
}

void Menu::showAdminPanel() {
    Terminal::clearScreen();
    Terminal::printHeader("ADMIN PANEL");
    std::cout << "\n"
              << "  " << Terminal::MAGENTA << "1." << Terminal::RESET << " Add Asset\n"
              << "  " << Terminal::MAGENTA << "2." << Terminal::RESET << " Remove Asset\n"
              << "  " << Terminal::MAGENTA << "3." << Terminal::RESET << " Reset Simulation\n"
              << "  " << Terminal::MAGENTA << "0." << Terminal::RESET << " Back\n"
              << "\n  " << Terminal::DIM << "Enter choice: " << Terminal::RESET;

    int choice = getMenuChoice(0, 3);
    (void)choice; // TODO: Epic 2 — wire up AdminAccount operations
    std::cout << "\n  " << Terminal::DIM
              << "[Admin operations — coming in Epic 2]"
              << Terminal::RESET << "\n";
    waitForEnter();
}

void Menu::handleLogin() {
    Terminal::clearScreen();
    Terminal::printHeader("WELCOME TO TERMINAL STOCK EXCHANGE");

    std::cout << "\n"
              << "  " << Terminal::DIM
              << "Full authentication coming in Epic 3.\n"
              << "  Auto-logging in as demo player...\n"
              << Terminal::RESET << "\n";

    // Epic 1 demo account — Epic 3 replaces this with file-backed auth
    auto player = std::make_shared<PlayerTrader>("demo_trader", "password", 10'000.0);
    engine.setCurrentUser(player);

    std::cout << "  " << Terminal::BRIGHT_GREEN
              << "✓ Logged in as demo_trader  ($10,000.00 starting balance)"
              << Terminal::RESET << "\n";

    waitForEnter();
}

void Menu::handleLogout() {
    engine.setCurrentUser(nullptr);
    running = false; // Logout exits the session; Epic 3 will return to login screen
}

// ─────────────────────────────────────────────────────────────────
//  Input helpers
// ─────────────────────────────────────────────────────────────────

int Menu::getMenuChoice(int min, int max) {
    int choice{};
    while (true) {
        std::cin >> choice;
        if (std::cin.fail() || choice < min || choice > max) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  " << Terminal::RED
                      << "Invalid input. Enter a number between "
                      << min << " and " << max << ": "
                      << Terminal::RESET;
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return choice;
        }
    }
}

void Menu::waitForEnter() {
    std::cout << "\n  " << Terminal::DIM
              << "Press Enter to return..." << Terminal::RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
