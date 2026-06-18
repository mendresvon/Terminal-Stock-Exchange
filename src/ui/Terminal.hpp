#pragma once

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
constexpr const char* GREEN        = "\033[32m";
constexpr const char* RED          = "\033[31m";
constexpr const char* YELLOW       = "\033[33m";
constexpr const char* CYAN         = "\033[36m";
constexpr const char* WHITE        = "\033[37m";
constexpr const char* MAGENTA      = "\033[35m";
constexpr const char* BRIGHT_GREEN = "\033[92m";
constexpr const char* BRIGHT_RED   = "\033[91m";
constexpr const char* BRIGHT_CYAN  = "\033[96m";

// ── Layout constants ──────────────────────────────────────────────
constexpr int BOX_WIDTH = 62;   ///< Outer width of all header boxes (chars)

// ── Utility functions ─────────────────────────────────────────────

/// Clears the terminal screen using ANSI escape codes.
void clearScreen();

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

} // namespace Terminal
