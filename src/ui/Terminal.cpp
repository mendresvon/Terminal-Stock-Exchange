#include "Terminal.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace Terminal {

// ─────────────────────────────────────────────────────────────────────────
//  Existing helpers (unchanged)
// ─────────────────────────────────────────────────────────────────────────

void clearScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
}

void printHeader(const std::string& title) {
    const int inner = BOX_WIDTH - 2;

    std::cout << CYAN << "\u2554";
    for (int i = 0; i < inner; ++i) std::cout << "\u2550";
    std::cout << "\u2557" << RESET << "\n";

    int titleLen  = static_cast<int>(title.size());
    int leftPad   = (inner - titleLen) / 2;
    int rightPad  = inner - titleLen - leftPad;

    std::cout << CYAN << "\u2551" << RESET;
    for (int i = 0; i < leftPad;  ++i) std::cout << " ";
    std::cout << BOLD << BRIGHT_CYAN << title << RESET;
    for (int i = 0; i < rightPad; ++i) std::cout << " ";
    std::cout << CYAN << "\u2551" << RESET << "\n";

    std::cout << CYAN << "\u255a";
    for (int i = 0; i < inner; ++i) std::cout << "\u2550";
    std::cout << "\u255d" << RESET << "\n";
}

void printDivider(int width) {
    std::cout << CYAN << DIM;
    for (int i = 0; i < width; ++i) std::cout << "\u2500";
    std::cout << RESET << "\n";
}

void printTable(const std::vector<std::string>&              headers,
                const std::vector<std::vector<std::string>>& rows,
                int                                          colWidth)
{
    const std::size_t cols = headers.size();
    auto hRule = [&](const char* left, const char* mid, const char* right) {
        std::cout << CYAN << left;
        for (std::size_t c = 0; c < cols; ++c) {
            for (int j = 0; j < colWidth + 2; ++j) std::cout << "\u2550";
            std::cout << (c + 1 < cols ? mid : right);
        }
        std::cout << RESET << "\n";
    };

    hRule("\u2554", "\u2566", "\u2557");

    std::cout << CYAN << "\u2551" << RESET;
    for (const auto& h : headers) {
        std::cout << " " << BOLD << std::left << std::setw(colWidth)
                  << h << RESET
                  << CYAN << " \u2551" << RESET;
    }
    std::cout << "\n";

    hRule("\u2560", "\u256c", "\u2563");

    for (const auto& row : rows) {
        std::cout << CYAN << "\u2551" << RESET;
        for (std::size_t c = 0; c < cols; ++c) {
            const std::string cell = (c < row.size()) ? row[c] : "";
            std::cout << " " << std::left << std::setw(colWidth)
                      << cell
                      << CYAN << " \u2551" << RESET;
        }
        std::cout << "\n";
    }

    hRule("\u255a", "\u2569", "\u255d");
}

void printColored(const std::string& text, const char* color) {
    std::cout << color << text << RESET;
}

// ─────────────────────────────────────────────────────────────────────────
//  printChart()  —  ASCII Line Chart
//
//  Layout (ROWS=12 high, up to 50 columns wide)
//
//   $185.00 │          *
//   $180.00 │     * |  |
//   $175.00 │  *  | |  |  *
//   $170.00 │  |  | |  |  |  *
//            └───────────────────
//             Day 1 → Day N
//
//  • '*' marks the exact price for that day.
//  • '|' draws a vertical connector between adjacent price levels so the
//    eye can follow the trend as a continuous line.
//  • Green = price rose vs previous day; Red = fell.
// ─────────────────────────────────────────────────────────────────────────

void printChart(const std::deque<double>& history) {
    if (history.size() < 2) {
        std::cout << "\n  " << DIM << "(Not enough data yet — advance more trading days)"
                  << RESET << "\n\n";
        return;
    }

    const int ROWS     = 12;
    int       dataSize = static_cast<int>(history.size());
    int       COLS     = std::min(dataSize, 50);
    int       startIdx = dataSize - COLS;

    // Range of visible slice
    auto beg = history.cbegin() + startIdx;
    auto en  = history.cend();

    double minP  = *std::min_element(beg, en);
    double maxP  = *std::max_element(beg, en);
    double range = (maxP - minP < 0.001) ? 1.0 : (maxP - minP);

    auto priceToRow = [&](double p) -> int {
        int r = static_cast<int>(std::round((maxP - p) / range * (ROWS - 1)));
        return std::max(0, std::min(ROWS - 1, r));
    };

    // Compute the chart row for each data column
    std::vector<int> rows(COLS);
    for (int c = 0; c < COLS; ++c) {
        rows[c] = priceToRow(*(beg + c));
    }

    // Build character and color grids
    std::vector<std::vector<char>> grid(ROWS, std::vector<char>(COLS, ' '));
    std::vector<std::vector<bool>> isGreen(ROWS, std::vector<bool>(COLS, true));

    for (int c = 0; c < COLS; ++c) {
        int  r     = rows[c];
        bool green = (c == 0) || (*(beg + c) >= *(beg + c - 1));

        grid[r][c]    = '*';
        isGreen[r][c] = green;

        // Draw vertical connector toward the NEXT column's row
        if (c < COLS - 1) {
            int  nextR     = rows[c + 1];
            bool nextGreen = *(beg + c + 1) >= *(beg + c);

            int lo = std::min(r, nextR) + 1;
            int hi = std::max(r, nextR);
            for (int ir = lo; ir < hi; ++ir) {
                if (grid[ir][c] == ' ') {
                    grid[ir][c]    = '|';
                    isGreen[ir][c] = nextGreen;
                }
            }
        }
    }

    // Print Y-axis + chart body
    std::cout << "\n";
    for (int r = 0; r < ROWS; ++r) {
        double yPrice = maxP - static_cast<double>(r) / (ROWS - 1) * range;
        std::cout << "  " << CYAN
                  << std::right << std::setw(10) << std::fixed
                  << std::setprecision(2) << yPrice
                  << " \u2502" << RESET;

        for (int c = 0; c < COLS; ++c) {
            char ch = grid[r][c];
            if (ch != ' ') {
                std::cout << (isGreen[r][c] ? BRIGHT_GREEN : BRIGHT_RED) << ch << RESET;
            } else {
                std::cout << ' ';
            }
        }
        std::cout << '\n';
    }

    // X-axis
    std::cout << "             " << CYAN << "\u2514";
    for (int c = 0; c < COLS; ++c) std::cout << "\u2500";
    std::cout << RESET << "\n"
              << "              " << DIM
              << "Day " << (startIdx + 1) << "  \u2192  Day " << dataSize
              << RESET << "\n\n";
}

// ─────────────────────────────────────────────────────────────────────────
//  Status line helpers
// ─────────────────────────────────────────────────────────────────────────

void printSuccess(const std::string& msg) {
    std::cout << "\n  " << BRIGHT_GREEN << "\u2713 " << msg << RESET << "\n";
}

void printError(const std::string& msg) {
    std::cout << "\n  " << BRIGHT_RED << "\u2717 " << msg << RESET << "\n";
}

void printWarning(const std::string& msg) {
    std::cout << "\n  " << YELLOW << "\u26a0  " << msg << RESET << "\n";
}

} // namespace Terminal
