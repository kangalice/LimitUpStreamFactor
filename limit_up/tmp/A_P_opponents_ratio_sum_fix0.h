#ifndef EVENTDRIVENSTRATEGY_A_P_OPPONENTS_RATIO_SUM_FIX0_H
#define EVENTDRIVENSTRATEGY_A_P_OPPONENTS_RATIO_SUM_FIX0_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class A_P_opponents_ratio_sum_fix0 : public BaseFactor
{
private:
    double activeSum1 = 0;
    double activeSum2 = 0;
    double passiveSum1 = 0;
    double passiveSum2 = 0;

public:
    A_P_opponents_ratio_sum_fix0(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"A_P_opponents_ratio_sum_fix0"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        if (fill->side == 1)
        {
            if (fill->amt <= 50000)
            {
                activeSum1 += fill->qty;
            }
            activeSum2 += fill->qty;
        }
        else
        {
            if (fill->amt <= 50000)
            {
                passiveSum1 += fill->qty;
            }
            passiveSum2 += fill->qty;
        }
    }

    void Calculate() override
    {
        double sum = 1.15;
        if (activeSum2 != 0 && passiveSum2 != 0)
        {
            sum = activeSum1 / activeSum2 + passiveSum1 / passiveSum2;
        }
        this->UpdateValue(0, sum);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_A_P_OPPONENTS_RATIO_SUM_FIX0_H
