# 系統架構圖 / System Architecture Diagram

> **Terminal Stock Exchange (TSE)** — C++17 互動式終端股票交易模擬系統

This diagram shows how the five architectural layers of the TSE interact with each other at runtime.

---

## 架構總覽 / Architecture Overview

```mermaid
graph TD
    subgraph Entry["🚀 Entry Point — main.cpp"]
        MAIN["main.cpp<br/>─────────────────────────<br/>1. engine.load()  → restore state<br/>2. seedDefaultAssets() on first run<br/>3. menu.run()  → hand off to UI"]
    end

    subgraph Engine["⚙️ Core Engine Layer"]
        ME["MarketEngine<br/>─────────────────────────<br/>• Asset registry  (unordered_map)<br/>• Account registry (vector)<br/>• Random-walk price engine (mt19937)<br/>• News event system (10% / day)<br/>• stepDay() / save() / load()"]
    end

    subgraph Assets["📈 Asset Module"]
        FA["&lt;&lt;abstract&gt;&gt; FinancialAsset<br/>─────────────────────────<br/>symbol / name / currentPrice<br/>priceHistory : deque&lt;double, 30&gt;<br/>calculateVolatility() = 0<br/>getTradingFee() = 0"]
        ST["Stock<br/>vol = ±2 % / day<br/>fee = 0.1 %"]
        CR["Crypto<br/>vol = ±7 % / day<br/>fee = 0.5 %"]
        ET["ETF<br/>vol = ±0.8 % / day<br/>fee = 0.03 %<br/>basketSymbols[]"]
        FA -->|inherits| ST
        FA -->|inherits| CR
        FA -->|inherits| ET
    end

    subgraph Accounts["👤 Account Module"]
        AC["&lt;&lt;abstract&gt;&gt; Account<br/>─────────────────────────<br/>username / passwordHash<br/>cashBalance<br/>portfolio : unordered_map&lt;symbol, qty&gt;<br/>avgCostBasis : unordered_map<br/>tradeHistory : vector&lt;TransactionRecord&gt;<br/>authenticate() = 0"]
        PT["PlayerTrader<br/>buy() / sell()<br/>Weighted-avg cost basis<br/>Starting cash $10,000"]
        AA["AdminAccount<br/>addAsset()<br/>removeAsset()<br/>resetSimulation()"]
        AC -->|inherits| PT
        AC -->|inherits| AA
    end

    subgraph IO["💾 Persistence Layer"]
        FM["FileManager  (static-only class)<br/>─────────────────────────<br/>saveMarket()   / loadMarket()<br/>saveAccounts() / loadAccounts()<br/>appendTradeLog()"]
        D1[("data/market_data.txt<br/>Asset registry<br/>+ 30-day price history")]
        D2[("data/accounts.txt<br/>All account records<br/>(pipe-delimited)")]
        D3[("data/trade_logs.txt<br/>Append-only<br/>ISO-8601 trade log")]
        FM --> D1
        FM --> D2
        FM --> D3
    end

    subgraph UI["🖥️ UI Layer"]
        TN["Terminal  (namespace)<br/>─────────────────────────<br/>ANSI escape-code constants<br/>printHeader / printTable<br/>printChart / printSparkline<br/>printSuccess / printError / printWarning"]
        MN["Menu<br/>─────────────────────────<br/>showMarket()      showPortfolio()<br/>showTradeMenu()   showAdminPanel()<br/>showTradeHistory()  nextTradingDay()<br/>handleLogin()     handleRegister()"]
    end

    subgraph Types["🔧 Shared Types (Types.hpp)"]
        TY["enum  AssetType  { STOCK, CRYPTO, ETF }<br/>struct TransactionRecord { symbol, type, price, qty, timestamp }<br/>inline nowIso8601() → string"]
    end

    %% Relationships
    MAIN --> ME
    MAIN --> MN
    ME  -->|"manages (shared_ptr)"| Assets
    ME  -->|"manages (shared_ptr)"| Accounts
    ME  -->|"delegates I/O"| FM
    MN  -->|"calls"| ME
    MN  -->|"renders via"| TN
    FM  -.->|"reconstructs"| Accounts
    FM  -.->|"reconstructs"| Assets
    TY  -.-|"used by"| Assets
    TY  -.-|"used by"| Accounts
    TY  -.-|"used by"| FM
```

---

## Layer Responsibilities

| Layer | Files | Responsibility |
|-------|-------|---------------|
| **Entry Point** | `src/main.cpp` | Bootstrap: load state → seed defaults → start menu loop |
| **Core Engine** | `src/engine/MarketEngine.hpp/.cpp` | Central registry, random-walk simulation, news events, persistence delegation |
| **Asset Module** | `src/assets/` | OOP hierarchy for all tradeable instruments; polymorphic volatility and fee logic |
| **Account Module** | `src/accounts/` | OOP hierarchy for all user types; buy/sell execution, cost-basis tracking |
| **Persistence Layer** | `src/io/FileManager.hpp/.cpp` | Static I/O service; pipe-delimited text serialisation/deserialisation |
| **UI Layer** | `src/ui/Terminal.hpp/.cpp`, `src/ui/Menu.hpp/.cpp` | ANSI rendering utilities + interactive menu loop |
| **Shared Types** | `src/types/Types.hpp` | Cross-cutting enum, struct, and utility function |

---

## Key Design Decisions

- **`shared_ptr` throughout** — assets and accounts are heap-allocated and owned by `MarketEngine` via `shared_ptr`; other components hold non-owning references.
- **Static-only `FileManager`** — no instance needed; all methods are `static`, preventing accidental state in the I/O layer.
- **`namespace Terminal`** — renders the UI layer as a collection of stateless free functions rather than a class, keeping it lightweight.
- **`mt19937` PRNG** — seeded from `std::random_device` on `MarketEngine` construction for reproducible-but-varied daily price moves.

---

*See also: [CLASS_DIAGRAM.md](CLASS_DIAGRAM.md) · [DATA_FLOW.md](DATA_FLOW.md) · [README.md](README.md)*
