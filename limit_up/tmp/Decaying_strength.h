#ifndef EVENTDRIVENSTRATEGY_DECAYING_STRENGTH_H
#define EVENTDRIVENSTRATEGY_DECAYING_STRENGTH_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Decaying_strength : public BaseFactor
{
private:
    int64_t delta = 0;
    double sum1 = 0;
    bool isCyb;
    int64_t timestamp930 = 93000000;

public:
    Decaying_strength(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"Decaying_strength"};
        this->isCyb = this->marketDataManager->zcz;
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        delta = common_utils::TimeUtil::CalTimeDelta(timestamp930, fill->timestamp);
        double ret = fill->price / this->marketDataManager->pre_close - 1.0;
        if (this->isCyb)
        {
            ret = ret / 2.0;
        }
        sum1 += delta * ret * fill->qty;
    }

    void Calculate() override
    {
        double strength = sum1 / delta / this->marketDataManager->lxjj_total_qty;
        this->UpdateValue(0, isnan(strength) ? 0.3 : strength);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_DECAYING_STRENGTH_H
