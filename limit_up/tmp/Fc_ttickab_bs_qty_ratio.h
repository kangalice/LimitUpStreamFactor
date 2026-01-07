
#ifndef EVENTDRIVENSTRATEGY_FC_TTICKAB_BS_QTY_RATIO_H
#define EVENTDRIVENSTRATEGY_FC_TTICKAB_BS_QTY_RATIO_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Fc_ttickab_bs_qty_ratio : public BaseFactor
{
public:
    Fc_ttickab_bs_qty_ratio(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"fc_ttickab_bs_qty_ratio"};
    }

    void Calculate() override
    {
        double factorVal = 0;

        LocalTime endTime = TimeUtil::AddMilliseconds(marketDataManager->ZT_local_time, -marketDataManager->params->tickTimeLag);
        std::vector<Tick *> &tickList = this->marketDataManager->quote_list;
        std::vector<double> ret;
        ret.reserve(10);
        for (int i = tickList.size() - 1; i >= 0; --i)
        {
            auto tick = tickList.at(i);
            if (tick->md_time <= endTime && tick->md_time >= 93000000)
            {
                ret.emplace_back((tick->ask_order_qty + 0.01) / (tick->bid_order_qty + 0.01));
            }
            if (ret.size() == 10)
            {
                break;
            }
        }
        factorVal = common_utils::MathUtil::calcMedian(ret);
        if (isnan(factorVal))
            factorVal = 0;
        this->UpdateValue(0, factorVal);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_FC_TTICKAB_BS_QTY_RATIO_H
