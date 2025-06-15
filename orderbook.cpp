#include <cstdint>
#include <vector>
#include <fmt/core.h>
#include <unordered_map>
#include <map>
#include <list>
#include <numeric>
#include <iostream>



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

/**
 * @brief  Struct for Levels - per price quantity
 * @note   
 * @retval None
 */
struct Level
{
    Price price_;
    Quantity quantity_;
};
// Array of these for the orderbook, basically for buy/sell the orders that are in per price
using Levels = std::vector<Level>;

/**
 * @brief  Levels class of both bids and asks
 * @note   
 * @retval None
 */
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

/**
 * @brief  Order class for basic blocks
 * @note   API made up of getters and isFilled / Fill method.
 * @retval None
 */
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

    /**
     * @brief  Getter for order type
     * @note   
     * @retval OrderType of order
     */
    OrderType getOrderType() const {
        return orderType_;
    }
    /**
     * @brief  Getter for order ID
     * @note   
     * @retval OrderId of order
     */
    OrderId getOrderId() const {
        return orderId_;
    }
    /**
     * @brief  Getter for buy/sell side
     * @note   
     * @retval Side enum of if it's buy or sell side order
     */
    Side getSide() const {
        return side_;
    }
    /**
     * @brief  Getter for price for order
     * @note   
     * @retval Price uint32_t for order
     */
    Price getPrice() const {
        return price_;
    }
    /**
     * @brief  Getter for order quantity initially passed in
     * @note   
     * @retval Returns Quantity of order initially passed in
     */
    Quantity getInitQuantity() const {
        return initQuantity_;
    }
    /**
     * @brief  Getter for remaining shares in order to fill
     * @note   
     * @retval Remaining shares to fill in order
     */
    Quantity getRemainingQuantity() const {
        return remainingQuantity_;
    }
    /**
     * @brief  Return how many shares have been filled in the order
     * @note   
     * @retval How many shares filled in order
     */
    Quantity getFilledQuantity() const {
        return initQuantity_ - remainingQuantity_;
    }
    /**
     * @brief  Getter for if the order is filled
     * @note   
     * @retval Returns true bool if order filled, else false
     */
    bool isFilled() {
        return remainingQuantity_ == 0;
    }
    /**
     * @brief  Attempts to fill order
     * @note   
     * @param  quantity: How many shares to fill in the order
     * @retval None
     */
    void Fill(Quantity quantity) {
        if (quantity > remainingQuantity_) 
        throw std::logic_error(fmt::format("Order {} can't be filled for more than its remaining quantity.", orderId_));

        remainingQuantity_ -= quantity;
    }
};
// Shared pointer object for order, we're putting it in many dictionaries and automates deallocation safely
using OrderPointer = std::shared_ptr<Order>;
// Vector of order pointers for orders at each price, make a list for O(1) removal within a order queue
using OrderPointers = std::list<OrderPointer>;

/**
 * @brief  Modified order class for ease of changing orders
 * @note   API includes mostly getters, makeOrderPointer makes a shared order pointer of the object if a change is requested
 * @retval None
 */
class ModOrder {
    private:
        OrderId orderId_;
        Price price_;
        Side side_;
        Quantity quantity_;
    
    public:
        /**
         * @brief  Constructor for modified order object
         * @note   
         * @param  orderId: Order ID of the existing order that should be modified
         * @param  price: Price modification of order
         * @param  side: Side modification of order (unnecessary)
         * @param  quantity: Quantity modification of order
         * @retval a ModOrder object
         */
        ModOrder(OrderId orderId, Price price, Side side, Quantity quantity)
            :   orderId_(orderId), price_(price), side_(side), quantity_(quantity) {}
    /**
     * @brief  Getter for modified order ID
     * @note   
     * @retval OrderID of object
     */
    OrderId getOrderId() const {
        return orderId_;
    }
    /**
     * @brief  Getter for price of modOrder
     * @note   
     * @retval Price uint32 of modOrder
     */
    Price getPrice() const {
        return price_;
    }
    /**
     * @brief  Getter for side of modOrder
     * @note   
     * @retval Side enum: Buy/Sell side of modorder
     */
    Side getSide() const {
        return side_;
    }
    /**
     * @brief  Getter for quantity of modorder
     * @note   
     * @retval Quantity uint32 of order
     */
    Quantity getQuantity() const {
        return quantity_;
    }
    /**
     * @brief  Makes an orderpointer from a modorder object for ease of replacement in orderbook
     * @note   
     * @param  orderType: What type the order should be
     * @retval OrderPointer to new order
     */
    OrderPointer makeOrderPointer(OrderType orderType) const {
        return std::make_shared<Order>(orderType, orderId_, side_, price_, quantity_);
    }
};
/**
 * @brief  Struct for information on trade per order (bidder/asker)
 * @note   
 * @retval None
 */
struct TradeInfo {
    OrderId orderId_;
    Price price_;
    Quantity quantity_;

    TradeInfo(OrderId orderId, Price price, Quantity quantity)
        :   orderId_(orderId), price_(price), quantity_(quantity) {}
};
/**
 * @brief  Overall trade, includes TradeInfo per bidder and asker order
 * @note   
 * @retval None
 */
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
/**
 * @brief  Primary orderbook class
 * @note   
 * @retval None
 */
class OrderBook {
    private:
        // Includes order itself and a iterator obj to the order's location within a OrderPointers List
        struct OrderEntry {
            OrderPointer order { nullptr };
            OrderPointers::iterator orderLoc_;
        };
        // Ordered map of asks, least ask first
        std::map<Price, OrderPointers> asks_;
        // Ordered map of bids, greatest bid first
        std::map<Price, OrderPointers, std::greater<Price>> bids_;
        // Unordered map of overall orders
        std::unordered_map<OrderId, OrderEntry> orders_;
        /**
         * @brief  CanMatch for FillOrKill orders
         * @note   
         * @param  side: Side of the order
         * @param  price: Price at which to evaluate
         * @retval If the order can be matched or not
         */
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

        /**
         * @brief  Main matchorders method, matches and executes trades
         * @note   
         * @retval Trades vector of the trades executed during matchorders
         */
        Trades MatchOrders() {
            // Reserve space, pessimistic approach
            Trades trades;
            trades.reserve(orders_.size());

            while (true) {
                // If no orders to execute, break
                if(bids_.empty() || asks_.empty()){
                    break;
                }

                auto& [bidPrice, bids] = *bids_.begin();
                auto& [askPrice, asks] = *asks_.begin();
                // If greatest Bid < least ask, no trades can be done so break
                if (bidPrice < askPrice) {
                    break;
                }

                // While each side has orders that haven't been executed
                while (bids.size() && asks.size()) {
                    // First bid and ask at price
                    auto& bid = bids.front();
                    auto& ask = asks.front();
                    // Min of bid and ask quantity is what gets traded
                    Quantity tradeQuantity = std::min(bid->getRemainingQuantity(), ask->getRemainingQuantity());
                    // Fill orders
                    bid->Fill(tradeQuantity);
                    ask->Fill(tradeQuantity);
                    // If either is fully filled, remove from bids OrderPointers array and orders map
                    if(bid->isFilled()) {
                        bids.pop_front();
                        orders_.erase(bid->getOrderId());
                    }
                    if(ask->isFilled()) {
                        asks.pop_front();
                        orders_.erase(ask->getOrderId());
                    }
                    // If no more orders for the price, remove completely from ordered map
                    if(bids.empty()) {
                        bids_.erase(bidPrice);
                    }
                    if(asks.empty()) {
                        asks_.erase(askPrice);
                    }
                    // Add executed trade to the trades vector
                    trades.push_back(Trade(TradeInfo(bid->getOrderId(), bid->getPrice(), tradeQuantity), 
                    TradeInfo(ask->getOrderId(), ask->getPrice(), tradeQuantity)));
                }
            }
            // If no more trades can happen, check if bids and asks are empty or not
            // If not, FillAndKill orders should be canceled
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
        /**
         * @brief  Add a order to the engine
         * @note   
         * @param  order: The pointer to the order object you want to put in
         * @retval The trades array of trades after you added the order
         */
        Trades addTrade(OrderPointer order) {
            if(orders_.contains(order->getOrderId())) return {};
            if(order->getOrderType() == OrderType::FillAndKill && !canMatch(order->getSide(), order->getPrice())) return {};

            OrderPointers::iterator iterator;
            // Add to OrderPointers list at price, set iterator
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
        /**
         * @brief  Cancel the order specified
         * @note   
         * @param  orderId: The order's ID that you want to cancel
         * @retval None
         */
        void cancelOrder(OrderId orderId) {
            if(!orders_.contains(orderId)) {
                return;
            }


            auto& [order, orderIterator] = orders_.at(orderId);
            orders_.erase(orderId);
            Price orderPrice = order->getPrice();
            // Get the list object for the price, and remove the order from the list
            // If the list is empty after, remove the price from the map
            if(order->getSide() == Side::Buy) {
                auto& orders = bids_.at(orderPrice);
                orders.erase(orderIterator);
                if(orders.empty()) {
                    bids_.erase(orderPrice);
                }
            } else {
                auto& orders = asks_.at(orderPrice);
                orders.erase(orderIterator);
                if(orders.empty()) {
                    asks_.erase(orderPrice);
                }
            }
        }
        /**
         * @brief  Modifies added order with ModOrder obj
         * @note   
         * @param  order: ModOrder obj with new values
         * @retval Trades array of trades after modifying order
         */
        Trades ModifyOrder(ModOrder order) {
            if (!orders_.contains(order.getOrderId())) return {};
            // we can just cancel and replace, as that's what it is
            // we need orderId for cancel order, orderPointer for add order
            const auto& [orderptr, iterator] = orders_.at(order.getOrderId());

            cancelOrder(order.getOrderId());
            return addTrade(order.makeOrderPointer(orderptr->getOrderType()));
        }
        /**
         * @brief  Getter for how many orders at any time
         * @note   
         * @retval Size_t obj of # orders
         */
        std::size_t getSize() const {
            return orders_.size();
        }
        /**
         * @brief  Getter for the levels of the OrderBook
         * @note   
         * @retval Returns OrderbookLevels of orderbook
         */
        OrderbookLevels getLevels() const {
            // Initialize vector of levels and reserve space
            Levels bids, asks;
            bids.reserve(orders_.size());
            asks.reserve(orders_.size());
            /**Lambda function, for a price and OrderPointers list, 
             return Level obj with given price and the total amount of remaining quantity for all orders of that list */
            auto makeLevel = [](Price price, const OrderPointers& orders) {
                return Level{ price, std::accumulate(orders.begin(), orders.end(), (Quantity)(0), [](std::size_t runningSum, const OrderPointer& order) {
                    return runningSum + order->getRemainingQuantity();
                })};
            };
        
            for (const auto& [price, orders] : bids_) {
                bids.push_back(makeLevel(price, orders));
            }

            for (const auto& [price, orders] : asks_) {
                asks.push_back(makeLevel(price, orders));
            }

            return OrderbookLevels(bids, asks);
        }

};

int main() {

    OrderBook orderbook;
    orderbook.addTrade(std::make_shared<Order>(OrderType::GoodTillCanceled, 1, Side::Buy, 15, 4));
    std::cout << "Size: " << orderbook.getSize() << std::endl;
    orderbook.cancelOrder(1);
    std::cout << "Size: " << orderbook.getSize() << std::endl;
    orderbook.addTrade(std::make_shared<Order>(OrderType::GoodTillCanceled, 1, Side::Buy, 15, 4));
    orderbook.addTrade(std::make_shared<Order>(OrderType::GoodTillCanceled, 2, Side::Sell, 15, 2));
    std::cout << "Size: " << orderbook.getSize() << std::endl;
    orderbook.addTrade(std::make_shared<Order>(OrderType::GoodTillCanceled, 3, Side::Sell, 15, 2));
    std::cout << "Size: " << orderbook.getSize() << std::endl;
    return 0;
}

