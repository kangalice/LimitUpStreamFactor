
#ifndef EVENTDRIVENSTRATEGY_STDBVOL_H
#define EVENTDRIVENSTRATEGY_STDBVOL_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Stdbvol : public BaseFactor
{
public:
    Stdbvol(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"stdBvol"};
    }

    void Calculate() override
    {
        if (this->marketDataManager->last_1min_trade_buy_map.empty())
        {
            this->UpdateValue(0, 0);
            return;
        }

        common_utils::StandardDeviation qtyStd(true);
        for (auto itr = this->marketDataManager->last_1min_trade_buy_map.begin();
             itr != this->marketDataManager->last_1min_trade_buy_map.end(); itr++)
        {
            qtyStd.increment(itr->second.qty);
        }

        double val = qtyStd.getResult();
        this->UpdateValue(0, isnan(val) ? 0 : val);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_STDBVOL_H
