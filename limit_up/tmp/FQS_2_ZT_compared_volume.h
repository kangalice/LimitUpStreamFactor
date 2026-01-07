#ifndef EVENTDRIVENSTRATEGY_FQS_2_ZT_COMPARED_VOLUME_H
#define EVENTDRIVENSTRATEGY_FQS_2_ZT_COMPARED_VOLUME_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class FQS_2_ZT_compared_volume : public BaseFactor
{
private:
    uint64_t FQS_Time = 0;
    uint64_t FQSSum = 0;
    int64_t index = 0;

public:
    FQS_2_ZT_compared_volume(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"FQS_2_ZT_compared_volume"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        if (FQS_Time != 0)
        {
            FQSSum += fill->qty;
        }
        else
        {
            double FQS_price;
            if (this->marketDataManager->zcz)
            {
                FQS_price = floor(this->marketDataManager->pre_close * 100 * 1.14 + 0.5) / 100;
            }
            else
            {
                FQS_price = floor(this->marketDataManager->pre_close * 100 * 1.07 + 0.5) / 100;
            }
            if (fill->price >= FQS_price)
            {
                FQS_Time = fill->timestamp;
                FQSSum += fill->qty;
                index = ((int64_t)this->marketDataManager->lxjj_fill_list.size()) - 2;
            }
        }
    }

    void Calculate() override
    {
        double factorValue = 0;
        std::vector<Fill *> &lxjj_fill_list = this->marketDataManager->lxjj_fill_list;
        if (!lxjj_fill_list.empty())
        {
            uint64_t sum = FQSSum;
            for (int64_t i = index; i >= 0; --i)
            {
                auto &fill = lxjj_fill_list[i];
                if (fill->timestamp == FQS_Time)
                {
                    sum += fill->qty;
                }
                else
                {
                    break;
                }
            }
            if (sum != 0)
            {
                factorValue = sum / this->marketDataManager->free_float_capital;
            }
        }

        this->UpdateValue(0, factorValue);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_FQS_2_ZT_COMPARED_VOLUME_H
