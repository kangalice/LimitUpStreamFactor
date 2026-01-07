#ifndef EVENTDRIVENSTRATEGY_SHEEP_OR_GOAT_H
#define EVENTDRIVENSTRATEGY_SHEEP_OR_GOAT_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Sheep_or_goat : public BaseFactor
{
private:
    double activeSell = 0;
    double activeBuy = 0;

public:
    Sheep_or_goat(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"Sheep_or_goat"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        if (fill->side == 2)
        {
            activeSell += 1;
        }
        else
        {
            activeBuy += 1;
        }
    }

    void Calculate() override
    {
        double factorValue = 0;
        if (activeSell <= 5)
        {
            factorValue = 0.5;
        }
        else if (activeSell <= 25)
        {
            factorValue = 1.5;
        }
        else if (activeSell <= 40)
        {
            factorValue = 2.0;
        }
        else
        {
            double lxjjBuyCount = this->marketDataManager->lxjj_buy_no_set.size();
            double lxjjSellCount = this->marketDataManager->lxjj_sell_no_set.size();
            if (lxjjBuyCount > 0 && lxjjSellCount > 0)
            {
                factorValue = (activeBuy / lxjjBuyCount) / (activeSell / lxjjSellCount);
            }
        }

        this->UpdateValue(0, isnan(factorValue) ? 0 : factorValue);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_SHEEP_OR_GOAT_H
