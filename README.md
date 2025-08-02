# MBP-10 Order Book Reconstruction

This project reconstructs a Market By Price (MBP-10) order book from a CSV-formatted Market By Order (MBO) data stream. The system handles real-time order events such as additions, trades, cancellations, and renewals, maintaining an accurate top-10 price level book for both bid and ask sides.

---

## Core Functionality

### 1. `OrderBook::process()`

* Routes incoming `FixMessage` objects based on their action type.
* Handles:

  * `Renew`: Resets the order book at the start of the session.
  * `Add`: Adds an order and updates the appropriate side.
  * `Cancel`: Cancels an order and updates levels.
  * `Trade`: Reduces size on the **opposite** book side (as per matching rule).
  * `Fill`: Ignored, as per business rule.

### 2. `get_depth(Side, price)`

* Computes the depth level of a price on either bid or ask side.
* Traverses full book, skipping zero-size levels.
* Uses:

  * `reverse_iterator` for bids (descending order).
  * `forward` iteration for asks (ascending order).

### 3. `update_top_book()`

* Keeps a separate vector of the top 10 levels on each side.
* Ensures fast access and clean CSV generation.

---

## Optimizations

### Efficient Data Structures

* `std::map` for `full_bids_` and `full_asks_`: maintains sorted order.
* `std::unordered_map` for `orders_`: O(1) lookup for cancel/trade.

### Lazy Top-N Maintenance

* Top 10 book levels are rebuilt only when necessary.
* Avoids unnecessary computation.

### Fast Trade Handling

* Supports both known and unknown order IDs during trades.
* Skips computation if already invalidated.
* Opposite-side trade matching logic implemented inline.

---

## Design Decisions

* **Action Filtering**:

  * `Trade` affects **opposite** side of the book.
  * `Fill` and `None` side trades are ignored (per spec).

* **Price Depth**:

  * Price depths are 0-based, and only non-zero-size levels are considered.

* **Trimming & Formatting**:

  * Prices are stored as integers (scaled by 10,000) to avoid floating-point errors.
  * Printed using a helper that removes trailing zeroes and decimal points.

---

## Usage

```bash
./reconstruction_ghwangbo input.csv output.csv
```

* Takes an input file of MBO events.
* Produces an output file of MBP-10 state transitions.

---

## Files

* `orderbook.hpp / orderbook.cpp` — Main order book logic.
* `main.cpp` — Input/output handling, parsing, MBP CSV generation.

---
