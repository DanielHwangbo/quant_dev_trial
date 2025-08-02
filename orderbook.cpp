#include "orderbook.hpp"
#include <algorithm>
#include <iostream>

void OrderBook::process(const FixMessage& msg) {
    switch (msg.action) {
        case Action::Renew:
            // Instruction 3a: Clear the orderbook
            reset();
            break;
        case Action::Add:
            add_order(msg);
            break;
        case Action::Cancel:
            cancel_order(msg.order_id);
            break;
        case Action::Trade:
            if (msg.side == Side::None) break; // 3c
            if (msg.side == Side::BID) {
                trade_order(msg, Side::ASK);
            } else if (msg.side == Side::ASK) {
                trade_order(msg, Side::BID);
            }
            break;
        case Action::Fill:
            // No-op: per instruction
            break;
        default:
            break;
    }
}

void OrderBook::reset() {
    orders_.clear();
    trade_buffer_.clear();
    full_bids_.clear();
    full_asks_.clear();
    top_bids_.clear();
    top_asks_.clear();
}

const std::unordered_map<std::string, Order>& OrderBook::get_orders() const {
    return orders_;
}

std::unordered_map<std::string, TradeInfo>& OrderBook::get_trade_buffer() {
    return trade_buffer_;
}

int OrderBook::get_depth(Side side, int64_t price) const {
    int depth = 0;

    if (side == Side::BID) {
        for (auto it = full_bids_.rbegin(); it != full_bids_.rend(); ++it) {
            const auto& [p, level] = *it;
            if (level.size <= 0) continue;
            if (price >= p) return depth;
            ++depth;
        }
        return depth;
    } else if (side == Side::ASK) {
        for (const auto& [p, level] : full_asks_) {
            if (level.size <= 0) continue;
            if (price <= p) return depth;
            ++depth;
        }
        return depth;
    }

    return 0;
}

const std::vector<BookLevel>& OrderBook::get_bids() const {
    return top_bids_;
}

const std::vector<BookLevel>& OrderBook::get_asks() const {
    return top_asks_;
}

bool OrderBook::has_book_changed(const std::vector<BookLevel>& old_bids, const std::vector<BookLevel>& new_bids,
                                 const std::vector<BookLevel>& old_asks, const std::vector<BookLevel>& new_asks) const {
    return old_bids != new_bids || old_asks != new_asks;
}

void OrderBook::add_order(const FixMessage& msg) {
    if (msg.order_id.empty()) return;

    Order order{msg.order_id, msg.side, msg.price, msg.size};
    orders_[msg.order_id] = order;

    if (msg.side == Side::BID) {
        auto& level = full_bids_[msg.price];
        level.price = msg.price;
        level.size += msg.size;
        level.count += 1;
    } else if (msg.side == Side::ASK) {
        auto& level = full_asks_[msg.price];
        level.price = msg.price;
        level.size += msg.size;
        level.count += 1;
    }

    update_top_book(msg.side);
}

void OrderBook::cancel_order(const std::string& order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) return;

    Order order = it->second;
    orders_.erase(it);

    if (order.side == Side::BID) {
        auto& level = full_bids_[order.price];
        level.size -= order.size;
        level.count -= 1;
        if (level.count == 0 || level.size <= 0) {
            full_bids_.erase(order.price);
        }
    } else if (order.side == Side::ASK) {
        auto& level = full_asks_[order.price];
        level.size -= order.size;
        level.count -= 1;
        if (level.count == 0 || level.size <= 0) {
            full_asks_.erase(order.price);
        }
    }

    update_top_book(order.side);
}

void OrderBook::trade_order(const FixMessage& msg, Side real_side) {
    if (msg.order_id.empty()) return;

    auto it = orders_.find(msg.order_id);
    if (it == orders_.end()) return;

    Order& order = it->second;
    int trade_size = std::min(order.size, msg.size);
    order.size -= trade_size;

    if (real_side == Side::BID) {
        auto& level = full_bids_[order.price];
        level.size -= trade_size;
        if (order.size <= 0) {
            level.count -= 1;
            if (level.count == 0 || level.size <= 0) {
                full_bids_.erase(order.price);
            }
            orders_.erase(it);
        }
    } else if (real_side == Side::ASK) {
        auto& level = full_asks_[order.price];
        level.size -= trade_size;
        if (order.size <= 0) {
            level.count -= 1;
            if (level.count == 0 || level.size <= 0) {
                full_asks_.erase(order.price);
            }
            orders_.erase(it);
        }
    }

    update_top_book(real_side);
}

void OrderBook::update_top_book(Side side) {
    std::vector<BookLevel>& top_book = (side == Side::BID) ? top_bids_ : top_asks_;
    top_book.clear();

    if (side == Side::BID) {
        for (auto it = full_bids_.rbegin(); it != full_bids_.rend(); ++it) {
            if (top_book.size() >= 10) break;
            if (it->second.size > 0) {
                top_book.push_back(it->second);
            }
        }
    } else {
        for (const auto& [price, level] : full_asks_) {
            if (top_book.size() >= 10) break;
            if (level.size > 0) {
                top_book.push_back(level);
            }
        }
    }
}
