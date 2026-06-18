#include "Terminal.hpp"

#include <iomanip>
#include <iostream>

namespace Terminal {

// ── Screen control ────────────────────────────────────────────────────────────

void clearScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
}

// ── Header box ────────────────────────────────────────────────────────────────

void printHeader(const std::string& title) {
    const int inner = BOX_WIDTH - 2; // space between the two vertical bars

    // Top border: ╔══════...══════╗
    std::cout << CYAN << "╔";
    for (int i = 0; i < inner; ++i) std::cout << "═";
    std::cout << "╗" << RESET << "\n";

    // Title row (centred, no ANSI in the padding math)
    int titleLen  = static_cast<int>(title.size());
    int leftPad   = (inner - titleLen) / 2;
    int rightPad  = inner - titleLen - leftPad;

    std::cout << CYAN << "║" << RESET;
    for (int i = 0; i < leftPad;  ++i) std::cout << " ";
    std::cout << BOLD << BRIGHT_CYAN << title << RESET;
    for (int i = 0; i < rightPad; ++i) std::cout << " ";
    std::cout << CYAN << "║" << RESET << "\n";

    // Bottom border: ╚══════...══════╝
    std::cout << CYAN << "╚";
    for (int i = 0; i < inner; ++i) std::cout << "═";
    std::cout << "╝" << RESET << "\n";
}

// ── Divider ───────────────────────────────────────────────────────────────────

void printDivider(int width) {
    std::cout << CYAN << DIM;
    for (int i = 0; i < width; ++i) std::cout << "─";
    std::cout << RESET << "\n";
}

// ── Data table ────────────────────────────────────────────────────────────────
//
//  Layout:
//    ╔══════════════╦══════════════╦ ... ╦══════════════╗
//    ║ Header       ║ Header       ║ ... ║ Header       ║
//    ╠══════════════╬══════════════╬ ... ╬══════════════╣
//    ║ cell         ║ cell         ║ ... ║ cell         ║
//    ...
//    ╚══════════════╩══════════════╩ ... ╩══════════════╝

void printTable(const std::vector<std::string>&              headers,
                const std::vector<std::vector<std::string>>& rows,
                int                                          colWidth)
{
    const std::size_t cols = headers.size();
    // Helper: print a full horizontal rule row
    auto hRule = [&](const char* left, const char* mid, const char* right) {
        std::cout << CYAN << left;
        for (std::size_t c = 0; c < cols; ++c) {
            for (int j = 0; j < colWidth + 2; ++j) std::cout << "═";
            std::cout << (c + 1 < cols ? mid : right);
        }
        std::cout << RESET << "\n";
    };

    // Top border
    hRule("╔", "╦", "╗");

    // Header row
    std::cout << CYAN << "║" << RESET;
    for (const auto& h : headers) {
        std::cout << " " << BOLD << std::left << std::setw(colWidth)
                  << h << RESET
                  << CYAN << " ║" << RESET;
    }
    std::cout << "\n";

    // Separator
    hRule("╠", "╬", "╣");

    // Data rows
    for (const auto& row : rows) {
        std::cout << CYAN << "║" << RESET;
        for (std::size_t c = 0; c < cols; ++c) {
            const std::string cell = (c < row.size()) ? row[c] : "";
            std::cout << " " << std::left << std::setw(colWidth)
                      << cell
                      << CYAN << " ║" << RESET;
        }
        std::cout << "\n";
    }

    // Bottom border
    hRule("╚", "╩", "╝");
}

// ── Coloured print ────────────────────────────────────────────────────────────

void printColored(const std::string& text, const char* color) {
    std::cout << color << text << RESET;
}

} // namespace Terminal
