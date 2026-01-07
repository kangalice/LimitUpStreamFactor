//
// Created by appadmin on 2023/2/13.
//

#ifndef EVENTDRIVENSTRATEGY_ZT_TIME_T_O2PRE_H
#define EVENTDRIVENSTRATEGY_ZT_TIME_T_O2PRE_H

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class ZT_Time_T_o2pre : public BaseFactor
{

public:
    ZT_Time_T_o2pre(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"ZT_Time", "sss_ZT_Time_ms", "T_o2pre", "sss_t_o2pre"};
    }

    void Calculate() override
    {
        this->UpdateValue(0, static_cast<double>(marketDataManager->reached_ZT_time));
        this->UpdateValue(1, common_utils::TimeUtil::CalTimeDelta(common_utils::Lxjj_AM_Start, marketDataManager->ZT_local_time));
        double val = marketDataManager->open_price / marketDataManager->pre_close - 1;
        this->UpdateValue(2, val);
        this->UpdateValue(3, marketDataManager->zcz ? val / 2 : val);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_ZT_TIME_T_O2PRE_H
