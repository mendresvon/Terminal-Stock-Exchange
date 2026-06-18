#include "engine/MarketEngine.hpp"
#include "ui/Menu.hpp"

// ─────────────────────────────────────────────────────────────────
//  main — Entry point for the Terminal Stock Exchange
//
//  Responsibilities:
//    1. Construct the market engine and seed default assets.
//    2. Hand control to the interactive menu loop.
//
//  All simulation logic is in MarketEngine (engine/).
//  All UI rendering is in Terminal + Menu (ui/).
// ─────────────────────────────────────────────────────────────────

int main() {
    MarketEngine engine;
    engine.seedDefaultAssets();

    Menu menu(engine);
    menu.run();

    return 0;
}
