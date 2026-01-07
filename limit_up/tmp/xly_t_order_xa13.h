
#ifndef EVENTDRIVENSTRATEGY_XLY_T_ORDER_XA13_H
#define EVENTDRIVENSTRATEGY_XLY_T_ORDER_XA13_H

#pragma once

#include "include/factor/base_factor.h"
using namespace vendor::strategy::common_utils;

namespace vendor {
namespace strategy {
namespace eventdriven {
class xly_t_order_xa13 : public BaseFactor
{
    struct OrderInfo {
        double amt;
        int64_t qty;
    };
private:
    std::map<int64_t, OrderInfo> sellMap;
    std::set<int64_t> buySet;

public:
    xly_t_order_xa13(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"xly_t_order_xa13"};
        this->update_mode = {4};
    }

    void Update(const TOrder *order) override
    {
        if (order->md_time >= 93000000) {
            int64_t min = order->md_time / 100000;
            if (order->side == '2') {
                auto it = sellMap.find(min);
                if (it == sellMap.end()) {
                    OrderInfo tmp(order->order_qty * order->order_price, order->order_qty);
                    sellMap.insert(std::make_pair(min, tmp));
                } else {
                    it->second.amt += order->order_qty * order->order_price;
                    it->second.qty += order->order_qty;
                }            
            } else {
                buySet.insert(min);
            }
        }
    }

    void Calculate() override
    {
        int counter = 0;
        std::vector<double> orderQty, vwap;
        orderQty.reserve(30);
        vwap.reserve(30);
        for (auto it = buySet.rbegin(); it != buySet.rend(); ++it) {
            if (counter == 30)
                break;
            if (buySet.find(*it) != buySet.end()) {
                auto it_sell = sellMap.find(*it);
                if (it_sell != sellMap.end()) {
                    orderQty.emplace_back(it_sell->second.qty);
                    vwap.emplace_back(it_sell->second.amt / it_sell->second.qty);
                }
                ++counter;
            }
        }
        double value = MathUtil::PearsonCorrelation(orderQty, vwap);
        this->UpdateValue(0, std::isnan(value) ? 0 : value);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_XLY_T_ORDER_XA13_H
