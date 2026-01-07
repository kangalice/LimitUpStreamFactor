//
// Created by appadmin on 2023/10/17.
//

#ifndef METIS_STRATEGY_CPP_BACKTEST_XBC_20230525_2_H
#define METIS_STRATEGY_CPP_BACKTEST_XBC_20230525_2_H

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Xbc_20230525_2 : public BaseFactor
{
public:
    Xbc_20230525_2(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"xbc_20230525_2"};
    }

    void Calculate() override
    {
        auto &tick_list = this->marketDataManager->quote_list;
        LocalTime end_time = TimeUtil::AddMilliseconds(marketDataManager->ZT_local_time, -marketDataManager->params->tickTimeLag);
        std::unordered_map<int64_t, std::vector<Tick *>> tick_time_map;
        tick_time_map.reserve(4000);
        for (auto tick : tick_list)
        {
            if (tick->md_time <= end_time && tick->md_time >= 93000000)
            {
                tick_time_map[tick->md_time / 3000].push_back(tick);
            }
        }
        double sum = 0;
        double cnt = 0;
        bool need_break = false;
        auto &fill_list = marketDataManager->lxjj_fill_list;
        for (auto it = fill_list.rbegin(); it != fill_list.rend(); ++it)
        {
            int64_t md = (*it)->timestamp / 3000;
            auto tick_it = tick_time_map.find(md);
            if (tick_it != tick_time_map.end())
            {
                for (auto *tick: tick_it->second) {
                    sum += ((*it)->price - tick->bid_avg_px) * (*it)->qty /
                           marketDataManager->free_float_capital /
                           marketDataManager->pre_close;
                    ++cnt;
                    if (cnt >= 100)
                    {
                        need_break = true;
                        break;
                    }
                }
                if (need_break) {
                    break;
                }
            }
        }
        double factorVal = sum / cnt;
        if (cnt < 5)
        {
            factorVal = 0;
        }
        this->UpdateValue(0, factorVal);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // METIS_STRATEGY_CPP_BACKTEST_XBC_20230525_2_H
