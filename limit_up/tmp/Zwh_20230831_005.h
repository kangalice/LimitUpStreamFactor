//
// Created by zhangtian on 2024/1/23.
//

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {

class Zwh_20230831_005 : public BaseFactor
{
public:
    Zwh_20230831_005(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"zwh_20230831_005"};
    }

    void Calculate() override
    {
        double f = 0.0;

        auto &tick_list = this->marketDataManager->quote_list;

        bool zcz = this->marketDataManager->zcz;
        LocalTime endTime = TimeUtil::AddMilliseconds(marketDataManager->ZT_local_time, -marketDataManager->params->tickTimeLag);
        // 从后向前，找到第一个符合时间要求的Tick
        for (auto itr = tick_list.rbegin(); itr != tick_list.rend(); itr++)
        {
            auto tick = (*itr);
            if (tick->md_time >= 93000000 && tick->md_time <= endTime)
            {
                double res = tick->total_amount / (tick->bid_order_qty + tick->total_volume);
                f = res / this->marketDataManager->pre_close - 1;
                if (zcz)
                {
                    f = f / 2;
                }
                break;
            }
        }

        this->UpdateValue(0, f);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
