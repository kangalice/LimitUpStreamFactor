//
// Created by zhangtian on 2023/2/20.
//

#pragma once

#include <climits>
#include <optional>
#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Vwap : public BaseFactor
{
public:
    Vwap(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"Vwap"};
    }

    void Calculate() override
    {
        double factorValue = 0;
        double lxjjTotalAmt = marketDataManager->lxjj_total_amt;
        double lxjjTotalQty = marketDataManager->lxjj_total_qty;

        if (marketDataManager->pre_close != 0 && lxjjTotalQty != 0)
        {
            factorValue = (lxjjTotalAmt / lxjjTotalQty / marketDataManager->pre_close - 1) * 100;
            factorValue = isnan(factorValue) ? 0 : factorValue;
        }

        factorValue = (marketDataManager->zcz ? factorValue * 0.53 : factorValue);
        UpdateValue(0, factorValue);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor