//
// Created by zhangtian on 2023/5/15.
//

#pragma once

#include "include/factor/base_factor.h"

using namespace vendor::strategy::common_utils;

namespace vendor {
namespace strategy {
namespace eventdriven {

class V_vtpcBact23_4 : public BaseFactor
{
public:
    V_vtpcBact23_4(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"V_vtpcBact23_4"};
    }

    void Calculate() override
    {
        double totalAmt = 0;
        double totalQty = 0;

        auto &tradeList = marketDataManager->last_1min_trade_list;
        for (auto itr = tradeList.begin(); itr != tradeList.end(); itr++)
        {
            if ((*itr)->trade_price > 0 && (*itr)->trade_buy_no > (*itr)->trade_sell_no)
            {
                totalAmt += (*itr)->trade_qty * (*itr)->trade_price;
                totalQty += (*itr)->trade_qty;
            }
        }

        double factorValue = 0;
        if (totalQty != 0)
        {
            double vwap = totalAmt / totalQty;
            factorValue = (vwap / marketDataManager->pre_close - 1) * 100;
        }

        factorValue = marketDataManager->zcz ? factorValue * 0.5 : factorValue;
        this->UpdateValue(0, factorValue);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
