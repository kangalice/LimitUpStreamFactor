#ifndef EVENTDRIVENSTRATEGY_BA_EMOTION_DIFF_SQRT_JUP_H
#define EVENTDRIVENSTRATEGY_BA_EMOTION_DIFF_SQRT_JUP_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class BA_emotion_diff_sqrt_jup : public BaseFactor
{
private:
    double halfPrice = 0;
    double aboveSum = 0;
    double belowSum = 0;

public:
    BA_emotion_diff_sqrt_jup(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"BA_emotion_diff_sqrt"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        if (halfPrice == 0)
        {
            halfPrice = (this->marketDataManager->high_limit_close - this->marketDataManager->jhjj_price) * 0.5 +
                        this->marketDataManager->jhjj_price;
        }
        if (fill->side == 1)
        {
            if (fill->price >= halfPrice)
            {
                aboveSum += fill->qty;
            }
            else
            {
                belowSum += fill->qty;
            }
        }
    }

    void Calculate() override
    {
        double diffRaw = (aboveSum - belowSum) / this->marketDataManager->free_float_capital;
        double tmp;
        if (diffRaw > 0)
        {
            tmp = 1;
        }
        else if (diffRaw < 0)
        {
            tmp = -1;
        }
        else if (diffRaw == 0)
        {
            tmp = 0;
        }
        else if (isnan(diffRaw))
        {
            tmp = NAN;
        }
        double diffSqrt = tmp * std::sqrt(std::abs(diffRaw));

        this->UpdateValue(0, isnan(diffSqrt) ? 0 : diffSqrt);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_BA_EMOTION_DIFF_SQRT_JUP_H
