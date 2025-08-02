#include "orderbook.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

// Helper: trim trailing zeros for price display
std::string trim_trailing_zeros(double num) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(10) << num;
    std::string str = oss.str();
    str.erase(str.find_last_not_of('0') + 1);
    if (!str.empty() && str.back() == '.') {
        str.pop_back();
    }
    return str;
}

FixMessage parse_csv_row(const std::string& line) {
    FixMessage msg;
    std::istringstream ss(line);
    std::string field;

    std::getline(ss, msg.ts_recv, ',');
    std::getline(ss, msg.ts_event, ',');
    std::getline(ss, msg.rtype, ',');
    std::getline(ss, msg.publisher_id, ',');
    std::getline(ss, msg.instrument_id, ',');
    std::getline(ss, field, ',');
    msg.action = field.empty() ? Action::Unknown : static_cast<Action>(field[0]);
    std::getline(ss, field, ',');
    msg.side = field.empty() ? Side::None : static_cast<Side>(field[0]);
    std::getline(ss, field, ',');
    msg.price = field.empty() ? 0 : static_cast<int64_t>(std::stod(field) * 10000);
    std::getline(ss, field, ',');
    msg.size = field.empty() ? 0 : std::stoi(field);
    std::getline(ss, field, ',');
    std::getline(ss, msg.order_id, ',');
    std::getline(ss, msg.flags, ',');
    std::getline(ss, msg.ts_in_delta, ',');
    std::getline(ss, msg.sequence, ',');
    std::getline(ss, msg.symbol, ',');

    return msg;
}

#include <string>
#include <sstream>
#include <iomanip>

std::string format_price_trimmed(double price) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << price;
    std::string s = oss.str();
    s.erase(s.find_last_not_of('0') + 1);
    if (s.back() == '.') s.pop_back();
    return s;
}

void write_row_csv(int count, std::ofstream& out, const FixMessage& row, const OrderBook& book) {

    out << count << "," << row.ts_event << "," << row.ts_event << ","  << "10," << row.publisher_id << ","
        << row.instrument_id << "," << static_cast<char>(row.action) << "," << static_cast<char>(row.side) << ","
        << row.depth << "," << format_price_trimmed(row.price / 10000.0) << "," << row.size << ","
        << row.flags << "," << row.ts_in_delta << "," << row.sequence;

    const auto& bids = book.get_bids();
    const auto& asks = book.get_asks();

    //std::cout << bids.size() << " " << asks.size() << std::endl;
    for (int i = 0; i < 10; ++i) {
        
        if (i < bids.size()) {
            
            out << "," << format_price_trimmed(bids[i].price / 10000.0) << "," << bids[i].size << "," << bids[i].count;
        } else {
            out << ",,0,0";
        }
        if (i < asks.size()) {
            out << "," << format_price_trimmed(asks[i].price / 10000.0) << "," << asks[i].size << "," << asks[i].count;
        } else {
            out << ",,0,0";
        }
    }
    out << "," << row.symbol << "," << row.order_id << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_csv> <output_csv>\n";
        return 1;
    }

    std::ifstream in(argv[1]);
    if (!in.is_open()) {
        std::cerr << "Failed to open input file\n";
        return 1;
    }

    std::ofstream out(argv[2]);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file\n";
        return 1;
    }

    // Write header
    out << ",ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,depth,price,size,flags,ts_in_delta,sequence";
    for (int i = 0; i < 10; ++i) {
        out << ",bid_px_" << std::setw(2) << std::setfill('0') << i
            << ",bid_sz_" << std::setw(2) << std::setfill('0') << i
            << ",bid_ct_" << std::setw(2) << std::setfill('0') << i
            << ",ask_px_" << std::setw(2) << std::setfill('0') << i
            << ",ask_sz_" << std::setw(2) << std::setfill('0') << i
            << ",ask_ct_" << std::setw(2) << std::setfill('0') << i;
    }
    out << ",symbol,order_id\n";

    OrderBook book;
    std::string line;

    // Skip header
    std::getline(in, line);
    int count = 0;
    while (std::getline(in, line)) {
        FixMessage msg = parse_csv_row(line);

        auto old_bids = book.get_bids();
        auto old_asks = book.get_asks();

        book.process(msg);
        msg.depth = book.get_depth(msg.side, msg.price);
        

        if (msg.action == Action::Renew || book.has_book_changed(old_bids, book.get_bids(), old_asks, book.get_asks())) {
            write_row_csv(count, out, msg, book);
            ++count;
        }
    }

    return 0;
}
