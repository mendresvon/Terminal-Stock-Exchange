#include "engine/MarketEngine.hpp"
#include "ui/Menu.hpp"

// ─────────────────────────────────────────────────────────────────
//  main — Entry point for the Terminal Stock Exchange
//
//  Startup sequence (Epic 3):
//    1. Attempt to load persisted state from data/.
//    2. If no data files exist (first run), seed default assets
//       and create the default admin account.
//    3. Hand control to the interactive menu loop.
//    4. On clean exit (Save & Quit), the Menu calls engine.save().
//
//  All simulation logic is in MarketEngine (engine/).
//  All UI rendering is in Terminal + Menu (ui/).
//  All file I/O is in FileManager (io/).
// ─────────────────────────────────────────────────────────────────

int main() {
    MarketEngine engine;

    bool loaded = engine.load();   // restore from data/ if available

    if (!loaded) {
        // First run — seed default market + default admin account
        engine.seedDefaultAssets();
    }

    Menu menu(engine);
    menu.run();

    return 0;
}
