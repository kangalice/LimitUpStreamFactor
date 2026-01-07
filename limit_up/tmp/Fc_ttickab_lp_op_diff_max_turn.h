
#ifndef EVENTDRIVENSTRATEGY_FC_TTICKAB_LP_OP_DIFF_MAX_TURN_H
#define EVENTDRIVENSTRATEGY_FC_TTICKAB_LP_OP_DIFF_MAX_TURN_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Fc_ttickab_lp_op_diff_max_turn : public BaseFactor
{
public:
    Fc_ttickab_lp_op_diff_max_turn(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"fc_ttickab_lp_op_diff_max_turn"};
    }

    void Calculate() override
    {
        double factorVal = 0;
        double pre_close = this->marketDataManager->pre_close;

        std::vector<double> lastPx_avgOfferPx_diff;
        auto &tick_list = this->marketDataManager->quote_list;
        LocalTime endTime = TimeUtil::AddMilliseconds(marketDataManager->ZT_local_time, -marketDataManager->params->tickTimeLag);
        for (auto &i : tick_list)
        {
            if (i->md_time <= endTime && i->md_time > 93000000)
            {
                lastPx_avgOfferPx_diff.emplace_back(i->ask_avg_px == 0 ? 0 : i->ask_avg_px - i->last_px);
            }
        }
        if (lastPx_avgOfferPx_diff.size() > 50)
        {
            double max = 0;
            double size = lastPx_avgOfferPx_diff.size();
            for (int i = 1; i <= 50; i++)
            {
                max = std::max(max, lastPx_avgOfferPx_diff[size - i]);
            }
            factorVal = max / pre_close;
        }
        else
        {
            factorVal = 0.025;
        }
        this->UpdateValue(0, factorVal);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_FC_TTICKAB_LP_OP_DIFF_MAX_TURN_H
