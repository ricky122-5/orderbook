# Order Book Matching Engine

A simple yet functional order book matching engine in C++, supporting Good-Till-Canceled and Fill-And-Kill order types, with basic trading and modification functionality.

## Features

- Bid/Ask level tracking
- Order addition, cancellation, and modification
- FIFO order matching
- Support for `GoodTillCanceled` and `FillAndKill` order types
- Trade execution with bid/ask breakdown

## Build & Run

### Requirements

- C++17 or later
- [fmt](https://github.com/fmtlib/fmt) library

### Compile

```bash
g++ -std=c++17 -lfmt orderbook.cpp -o orderbook
```

> Ensure `fmt` is installed and linked properly.

### Run

```bash
./orderbook
```

## Code Overview

### Core Concepts

- **Order**: Represents an individual buy/sell order.
- **ModOrder**: Used to modify an existing order.
- **Trade & TradeInfo**: Records executed trades between matched orders.
- **OrderBook**: Main engine that handles matching, storage, and order state transitions.
- **OrderbookLevels**: Utility to inspect current book state (bids and asks).

### Order Types

```cpp
enum class OrderType {
    GoodTillCanceled,
    FillAndKill
};
```

### Sides

```cpp
enum class Side {
    Buy,
    Sell
};
```

## Example Usage

```cpp
OrderBook orderbook;
orderbook.addTrade(std::make_shared<Order>(OrderType::GoodTillCanceled, 1, Side::Buy, 15, 4));
orderbook.cancelOrder(1);
orderbook.addTrade(std::make_shared<Order>(OrderType::GoodTillCanceled, 1, Side::Buy, 15, 4));
orderbook.addTrade(std::make_shared<Order>(OrderType::GoodTillCanceled, 2, Side::Sell, 15, 2));
orderbook.addTrade(std::make_shared<Order>(OrderType::GoodTillCanceled, 3, Side::Sell, 15, 2));
```

## Levels API

```cpp
OrderbookLevels levels = orderbook.getLevels();
const Levels& bids = levels.getBids();
const Levels& asks = levels.getAsks();
```

Each `Level` contains:
- `price_`: Price level
- `quantity_`: Total remaining quantity at that level

## Future Improvements

- Order time priority for more accurate matching
- Market/Limit order support
- Persistence (serialization)
- WebSocket/API server

## License

MIT License - Use freely at your own discretion.
