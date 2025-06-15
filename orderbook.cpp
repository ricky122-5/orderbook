#include <cstdint>
#include <vector>
#include <fmt/core.h>
#include <unordered_map>
#include <map>



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
            }
        }

};

int main() {
    return 0;
}

