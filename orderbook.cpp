#include <cstdint>
#include <vector>
#include <fmt/core.h>
#include <unordered_map>
#include <map>
#include <list>



enum class OrderType {
    GoodTillCanceled,
    FillAndKill
};

enum class Side {
    Buy,
    Sell
};

using Price = std::uint32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;


struct Level
{
    Price price_;
    Quantity quantity_;
};

using Levels = std::vector<Level>;


class OrderbookLevels {

    private:
        Levels bids_;
        Levels asks_;

    public:
        OrderbookLevels(const Levels& bids, const Levels& asks)
            :   bids_(bids), asks_(asks) {}
        
    const Levels& getBids() const {
        return bids_;
    }

    const Levels& getAsks() const {
        return asks_;
    }
};


class Order {
    private:
        OrderType orderType_;
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity initQuantity_;
        Quantity remainingQuantity_;

    public:
        Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
            :   orderType_(orderType), 
                orderId_(orderId), 
                side_(side), 
                price_(price), 
                initQuantity_(quantity), 
                remainingQuantity_(quantity) {}


    OrderType getOrderType() const {
        return orderType_;
    }

    OrderId getOrderId() const {
        return orderId_;
    }

    Side getSide() const {
        return side_;
    }

    Price getPrice() const {
        return price_;
    }

    Quantity getInitQuantity() const {
        return initQuantity_;
    }

    Quantity getRemainingQuantity() const {
        return remainingQuantity_;
    }

    Quantity getFilledQuantity() const {
        return initQuantity_ - remainingQuantity_;
    }

    bool isFilled() {
        return remainingQuantity_ == 0;
    }

    void Fill(Quantity quantity) {
        if (quantity > remainingQuantity_) 
        throw std::logic_error(fmt::format("Order {} can't be filled for more than its remaining quantity.", orderId_));

        remainingQuantity_ -= quantity;
    }
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

class ModOrder {
    private:
        OrderId orderId_;
        Price price_;
        Side side_;
        Quantity quantity_;
    
    public:
        ModOrder(OrderId orderId, Price price, Side side, Quantity quantity)
            :   orderId_(orderId), price_(price), side_(side), quantity_(quantity) {}

    OrderId getOrderId() const {
        return orderId_;
    }

    Price getPrice() const {
        return price_;
    }

    Side getSide() const {
        return side_;
    }

    Quantity getQuantity() const {
        return quantity_;
    }

    OrderPointer makeOrderPointer(OrderType orderType) const {
        return std::make_shared<Order>(orderType, orderId_, side_, price_, quantity_);
    }
};

struct TradeInfo {
    OrderId orderId_;
    Price price_;
    Quantity quantity_;

    TradeInfo(OrderId orderId, Price price, Quantity quantity)
        :   orderId_(orderId), price_(price), quantity_(quantity) {}
};

class Trade {
    private:
        TradeInfo bid_;
        TradeInfo ask_;
    public:
        Trade(const TradeInfo& bid, const TradeInfo& ask)
            :   bid_(bid), ask_(ask) {}

    const TradeInfo& getBid() const {
        return bid_;
    }

    const TradeInfo& getAsk() const {
        return ask_;
    }
};

using Trades = std::vector<Trade>;

class OrderBook {
    private:
        struct OrderEntry {
            OrderPointer order { nullptr };
            OrderPointers::iterator orderLoc_;
        };
        std::map<Price, OrderPointers> asks_;
        std::map<Price, OrderPointers, std::greater<Price>> bids_;
        std::unordered_map<OrderId, OrderEntry> orders_;

        bool canMatch(Side side, Price price) const {
            if (side == Side::Buy) {
                if (asks_.empty())
                    return false;
                
                const auto& [bestAsk, _] = *asks_.begin();
                return price >= bestAsk;
            } else {
                if (bids_.empty())
                    return false;
                
                const auto& [bestBid, _] = *bids_.begin();
                return price <= bestBid;
            }
        }


        Trades MatchOrders() {
            Trades trades;
            trades.reserve(orders_.size());

            while (true) {
                if(bids_.empty() || asks_.empty()){
                    break;
                }

                auto& [bidPrice, bids] = *bids_.begin();
                auto& [askPrice, asks] = *asks_.begin();

                if (bidPrice < askPrice) {
                    break;
                }


                while (bids.size() && asks.size()) {
                    auto& bid = bids.front();
                    auto& ask = asks.front();

                    Quantity tradeQuantity = std::min(bid->getRemainingQuantity(), ask->getRemainingQuantity());

                    bid->Fill(tradeQuantity);
                    ask->Fill(tradeQuantity);

                    if(bid->isFilled()) {
                        bids.pop_front();
                        orders_.erase(bid->getOrderId());
                    }
                    if(ask->isFilled()) {
                        asks.pop_front();
                        orders_.erase(ask->getOrderId());
                    }

                    if(bids.empty()) {
                        bids_.erase(bidPrice);
                    }
                    if(asks.empty()) {
                        asks_.erase(askPrice);
                    }

                    trades.push_back(Trade(TradeInfo(bid->getOrderId(), bid->getPrice(), tradeQuantity), 
                    TradeInfo(ask->getOrderId(), ask->getPrice(), tradeQuantity)));
                }
            }

            if(!bids_.empty()) {
                auto& [_, bids] = *bids_.begin();
                auto& order = bids.front();
                if(order->getOrderType() == OrderType::FillAndKill) {
                    // Cancel order
                } 
            }

            if(!asks_.empty()) {
                auto& [_, asks] = *asks_.begin();
                auto& order = asks.front();
                if(order->getOrderType() == OrderType::FillAndKill) {
                    // Cancel order
                }
            }

            return trades;
        }
    public:

        Trades addTrade(OrderPointer order) {
            if(orders_.contains(order->getOrderId())) return {};
            if(order->getOrderType() == OrderType::FillAndKill && !canMatch(order->getSide(), order->getPrice())) return {};

            OrderPointers::iterator iterator;
            if(order->getSide() == Side::Buy) {
                auto& orders = bids_[order->getPrice()];
                orders.push_back(order);
                iterator = std::next(orders.begin(), orders.size() - 1);
            } else {
                auto& orders = asks_[order->getPrice()];
                orders.push_back(order);
                iterator = std::next(orders.begin(), orders.size() - 1);
            }

            orders_.insert({order->getOrderId(), OrderEntry{order, iterator}});
            return MatchOrders();
        }

};

int main() {
    return 0;
}

