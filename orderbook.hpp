#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <map>

enum class Side : char {
    BID = 'B',
    ASK = 'A',
    None = 'N'
};

enum class Action : char {
    Add = 'A',
    Cancel = 'C',
    Trade = 'T',
    Fill = 'F',
    Unknown = 'U',
    Renew = 'R',
};

struct FixMessage {
    std::string ts_recv;
    std::string ts_event;
    std::string rtype;
    std::string publisher_id;
    std::string instrument_id;
    Action action = Action::Unknown;
    Side side = Side::None;
    int depth = 0;
    int64_t price = 0;
    int size = 0;
    std::string flags;
    std::string ts_in_delta;
    std::string sequence;
    std::string symbol;
    std::string order_id;
};

struct Order {
    std::string order_id;
    Side side;
    int64_t price;
    int size;
};

struct BookLevel {
    int64_t price = 0;
    int size = 0;
    int count = 0;

    bool operator==(const BookLevel& other) const {
        return price == other.price && size == other.size && count == other.count;
    }
    bool operator!=(const BookLevel& other) const {
        return !(*this == other);
    }
};


struct TradeInfo {
    int size = 0;
    // Extend as needed
};

class OrderBook {
public:
    void process(const FixMessage& msg);
    void reset();

    const std::unordered_map<std::string, Order>& get_orders() const;
    std::unordered_map<std::string, TradeInfo>& get_trade_buffer();

    const std::vector<BookLevel>& get_bids() const;
    const std::vector<BookLevel>& get_asks() const;
    int get_depth(Side side, int64_t price) const;

    bool has_book_changed(const std::vector<BookLevel>& old_bids, const std::vector<BookLevel>& new_bids,
                          const std::vector<BookLevel>& old_asks, const std::vector<BookLevel>& new_asks) const;

private:
    void add_order(const FixMessage& msg);
    void cancel_order(const std::string& order_id);
    void trade_order(const FixMessage& msg, Side real_side);
    void update_top_book(Side side);

private:
    std::unordered_map<std::string, Order> orders_;
    std::unordered_map<std::string, TradeInfo> trade_buffer_;
    std::vector<BookLevel> top_bids_;
    std::vector<BookLevel> top_asks_;

    // Use map sorted ascending by default, we'll reverse order in update_top_book for bids
    std::map<int64_t, BookLevel> full_bids_; // price -> level, bids map (price ascending)
    std::map<int64_t, BookLevel> full_asks_; // price -> level, asks map (price ascending)
};

#endif // ORDERBOOK_HPP
