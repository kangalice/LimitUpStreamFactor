
#ifndef EVENTDRIVENSTRATEGY_SSS_TK_SLOPR_SELL_T20_MAX
#define EVENTDRIVENSTRATEGY_SSS_TK_SLOPR_SELL_T20_MAX

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Sss_tk_slope_sell_t20_max : public BaseFactor
{
public:
    Sss_tk_slope_sell_t20_max(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"sss_tk_slope_sell_t20_max"};
    }

    void Calculate() override
    {
        LocalTime endTime = TimeUtil::AddMilliseconds(marketDataManager->ZT_local_time, -marketDataManager->params->tickTimeLag);
        auto tick_list = this->marketDataManager->quote_list;
        std::vector<double> value_list;
        int counter = 20;
        double slope_sell = 0;
        for (auto it = tick_list.rbegin(); it != tick_list.rend(); it++)
        {
            if (counter <= 0)
                break;
            auto cur_tick = (*it);
            if (counter > 0 && cur_tick->md_time <= endTime && cur_tick->last_px > 0)
            {
                int64_t sell_order_sum = 0;
                double fprice = 0;
                for(int i = 0; i < 10; i++)
                {
                    sell_order_sum += cur_tick->ask_qty[i];
                    if (cur_tick->ask_price[i] != 0)
                    {
                        fprice = cur_tick->ask_price[i];
                    }
                }
                double tmp = (1-cur_tick->ask_qty[0] / (double)sell_order_sum) / (fprice - cur_tick->ask_price[0]);
                if (! isnan(tmp))
                {
                    slope_sell = std::max(slope_sell, tmp);
                }
                counter -= 1;
            }
        }
        this->UpdateValue(0, slope_sell);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_SSS_TK_SLOPR_SELL_T20_MAX
