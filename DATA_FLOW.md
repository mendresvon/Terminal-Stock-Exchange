# 資料流程圖 / Data Flow Diagram

> **Terminal Stock Exchange (TSE)** — Sequence diagrams showing how data moves through the system for the three core runtime flows.

---

## Flow 1: Startup & State Restoration

```mermaid
sequenceDiagram
    actor User as 👤 User
    participant main as main.cpp
    participant ME as MarketEngine
    participant FM as FileManager
    participant Disk as 💾 data/

    User->>main: ./build/tse
    main->>ME: MarketEngine engine
    main->>ME: engine.load()
    ME->>FM: FileManager::loadMarket(engine)
    FM->>Disk: open data/market_data.txt
    alt File exists
        Disk-->>FM: pipe-delimited asset records + day counter
        FM-->>ME: restore assets, setCurrentDay(n)
        ME->>FM: FileManager::loadAccounts()
        FM->>Disk: open data/accounts.txt
        Disk-->>FM: pipe-delimited account records
        FM-->>ME: registerAccount(PlayerTrader / AdminAccount)
        ME-->>main: load() → true
    else First run (no data/)
        Disk-->>FM: file not found
        FM-->>ME: load() → false
        ME-->>main: load() → false
        main->>ME: engine.seedDefaultAssets()
        Note over ME: Seeds 10 default assets<br/>+ admin account (admin/admin)
    end
    main->>Menu: Menu menu(engine); menu.run()
    Menu-->>User: Splash screen → Login/Register prompt
```

---

## Flow 2: Trade Execution (Buy)

```mermaid
sequenceDiagram
    actor User as 👤 User
    participant MN as Menu
    participant ME as MarketEngine
    participant Asset as FinancialAsset (Stock/Crypto/ETF)
    participant PT as PlayerTrader
    participant FM as FileManager
    participant Disk as 💾 data/trade_logs.txt

    User->>MN: Select "Trade" → enter ticker "AAPL"
    MN->>ME: engine.getAsset("AAPL")
    ME-->>MN: shared_ptr~Stock~

    MN->>Asset: getCurrentPrice()
    Asset-->>MN: 182.50
    MN->>Asset: getTradingFee()
    Asset-->>MN: 0.001  (0.1%)

    MN-->>User: Asset detail panel + price chart
    User->>MN: Choose Buy, qty = 10

    MN->>ME: getCurrentUser()
    ME-->>MN: shared_ptr~PlayerTrader~

    MN->>PT: player.buy("AAPL", 10, 182.50, 0.001)
    Note over PT: cost = 10 × 182.50 = 1,825.00<br/>fee  = 1,825.00 × 0.001 = 1.83<br/>total deducted = 1,826.83<br/>cashBalance -= 1,826.83<br/>portfolio["AAPL"] += 10<br/>avgCostBasis["AAPL"] recalculated
    PT-->>MN: true (success)

    MN->>FM: FileManager::appendTradeLog(record, "alice")
    FM->>Disk: append "alice|BUY|AAPL|182.50|10|2024-01-15T10:32:00\n"
    Disk-->>FM: OK

    MN-->>User: ✓ Bought 10 shares of AAPL at $182.50
```

---

## Flow 3: Advance Trading Day

```mermaid
sequenceDiagram
    actor User as 👤 User
    participant MN as Menu
    participant ME as MarketEngine
    participant Asset as FinancialAsset (all assets)

    User->>MN: Select "Next Trading Day"
    MN->>ME: engine.stepDay()

    loop For each registered asset
        ME->>ME: draw return from N(0, volatility)
        ME->>Asset: updatePrice(currentPrice × (1 + return))
        Asset->>Asset: priceHistory.push_back(newPrice)<br/>if size > 30: pop_front()
    end

    ME->>ME: roll NEWS_PROBABILITY (10%)
    alt News event fires
        ME->>ME: applyNewsEvent()
        ME->>ME: pick random asset
        ME->>ME: roll spike magnitude [15%, 40%]
        ME->>ME: coin-flip: positive or negative event
        ME->>Asset: updatePrice(spiked price)
        ME-->>MN: print dramatic headline to terminal
    end

    ME->>ME: currentDay++
    ME-->>MN: return (gainers/losers printed inside stepDay)
    MN-->>User: Daily summary: top gainers, top losers, current day
```

---

## Flow 4: Save & Quit

```mermaid
sequenceDiagram
    actor User as 👤 User
    participant MN as Menu
    participant ME as MarketEngine
    participant FM as FileManager
    participant Disk as 💾 data/

    User->>MN: Select "Save & Quit"
    MN->>ME: engine.save()

    ME->>FM: FileManager::saveMarket(engine)
    FM->>Disk: create data/ if not exists
    FM->>Disk: write data/market_data.txt<br/>(day counter + all assets + 30-day price deques)
    Disk-->>FM: OK

    ME->>FM: FileManager::saveAccounts(engine.getAccounts())
    FM->>Disk: write data/accounts.txt<br/>(username|hash|cash|portfolio|avgCost for each account)
    Disk-->>FM: OK

    ME-->>MN: save() → true
    MN-->>User: 👋 Goodbye session summary (P&L, net worth)
    MN->>MN: running = false → exit menu loop
```

---

## Data File Formats

### `data/market_data.txt`
```
DAY|5
STOCK|AAPL|Apple Inc.|182.50|0.02|0.0|180.10|181.30|182.50|...
CRYPTO|BTC|Bitcoin|43250.00|0.07|0.005|41000.00|42100.00|43250.00|...
ETF|SPY|S&P 500 ETF|452.10|0.008|SPY:AAPL:MSFT:GOOGL|450.20|451.60|452.10|...
```

### `data/accounts.txt`
```
PLAYER|alice|12345678901234567890|9823.17|AAPL:10:182.50,MSFT:5:370.00|
ADMIN|admin|98765432109876543210|0.00||
```

### `data/trade_logs.txt`
```
alice|BUY|AAPL|182.50|10|2024-01-15T10:32:00
alice|SELL|BTC|43100.00|1|2024-01-15T14:05:22
```

---

*See also: [ARCHITECTURE.md](ARCHITECTURE.md) · [CLASS_DIAGRAM.md](CLASS_DIAGRAM.md) · [README.md](README.md)*
