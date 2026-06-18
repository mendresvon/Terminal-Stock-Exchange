#pragma once

#include <deque>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────
//  Terminal  —  ANSI color helpers and ASCII box-drawing utilities
// ─────────────────────────────────────────────────────────────────────

namespace Terminal {

// ── ANSI escape codes ─────────────────────────────────────────────
constexpr const char* RESET        = "\033[0m";
constexpr const char* BOLD         = "\033[1m";
constexpr const char* DIM          = "\033[2m";
constexpr const char* ITALIC       = "\033[3m";
constexpr const char* GREEN        = "\033[32m";
constexpr const char* RED          = "\033[31m";
constexpr const char* YELLOW       = "\033[33m";
constexpr const char* CYAN         = "\033[36m";
constexpr const char* WHITE        = "\033[37m";
constexpr const char* MAGENTA      = "\033[35m";
constexpr const char* BRIGHT_GREEN  = "\033[92m";
constexpr const char* BRIGHT_RED    = "\033[91m";
constexpr const char* BRIGHT_CYAN   = "\033[96m";
constexpr const char* BRIGHT_YELLOW = "\033[93m";   ///< Epic 4: net worth & highlights
constexpr const char* BRIGHT_MAGENTA = "\033[95m";  ///< Epic 4: admin badge

// ── Layout constants ──────────────────────────────────────────────
constexpr int BOX_WIDTH = 62;   ///< Outer width of all header boxes (chars)

// ── Utility functions ─────────────────────────────────────────────

/// Clears the terminal screen using ANSI escape codes.
void clearScreen();

/// Prints the one-time ASCII art splash screen on first launch.
void printSplash();

/// Prints a double-line box (╔═══╗) with a centred, bolded title.
void printHeader(const std::string& title);

/// Prints a thin horizontal divider (─────) of the given width.
void printDivider(int width = BOX_WIDTH);

/**
 * Renders a data table with double-line ASCII borders (╔╦╗ ╠╬╣ ╚╩╝).
 *
 * @param headers   Column header strings.
 * @param rows      2-D grid of cell strings; short rows are right-padded.
 * @param colWidth  Character width allocated to each column cell.
 */
void printTable(const std::vector<std::string>&              headers,
                const std::vector<std::vector<std::string>>& rows,
                int                                          colWidth = 14);

/// Writes @p text wrapped in @p color, then resets.
void printColored(const std::string& text, const char* color);

/**
 * Renders a scrolling ASCII line chart of price history.
 * Dots mark price points; vertical bars connect adjacent levels.
 * Green = price rose vs previous day; Red = price fell.
 *
 * @param history  The asset's price history deque (up to 30 entries).
 */
void printChart(const std::deque<double>& history);

/**
 * Returns a compact UTF-8 sparkline string built from block characters
 * (▁▂▃▄▅▆▇█).  The result is exactly @p width visible characters wide.
 * Coloured green if the last price >= first price in the visible window,
 * red otherwise.  Returns the raw string WITHOUT embedded ANSI — caller
 * wraps with the colour from sparklineColor().
 *
 * @param history  Price deque (any length ≥ 1).
 * @param width    Number of block chars to emit (default 8).
 */
std::string printSparkline(const std::deque<double>& history, int width = 8);

/**
 * Returns the ANSI colour code appropriate for the sparkline:
 * BRIGHT_GREEN if last >= first, BRIGHT_RED otherwise.
 */
const char* sparklineColor(const std::deque<double>& history);

/// Prints a green success line:  ✓ <msg>
void printSuccess(const std::string& msg);

/// Prints a red error line:      ✗ <msg>
void printError(const std::string& msg);

/// Prints a yellow warning line: ⚠ <msg>
void printWarning(const std::string& msg);

} // namespace Terminal
