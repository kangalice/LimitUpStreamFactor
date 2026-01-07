
#ifndef EVENTDRIVENSTRATEGY_SSS_TK_SPCTDIFF_ALL_HALF_H
#define EVENTDRIVENSTRATEGY_SSS_TK_SPCTDIFF_ALL_HALF_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Sss_tk_spctdiff_all_half : public BaseFactor
{
private:
    bool zcz;

public:
    Sss_tk_spctdiff_all_half(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"sss_tk_spctdiff_all_half"};
        zcz = marketDataManager->zcz;
    }

    void Calculate() override
    {
        double factorVal = 0;

        LocalTime endTime = TimeUtil::AddMilliseconds(marketDataManager->ZT_local_time, -marketDataManager->params->tickTimeLag);
        std::vector<Tick *> &tickList = this->marketDataManager->quote_list;
        int64_t endIndex = tickList.size() - 1;
        int64_t startIndex = 0;
        for (; endIndex >= 0; --endIndex)
        {
            LocalTime tickTime = tickList[endIndex]->md_time;
            if (tickTime <= endTime)
            {
                endTime = tickTime;
                break;
            }
        }
        for (; startIndex < tickList.size(); ++startIndex)
        {
            if (tickList[startIndex]->md_time >= 93000000)
            {
                break;
            }
        }

        // endIndex is the last in-range tick
        if (startIndex <= endIndex)
        {
            double mean = 0, halfMean = 0;
            int64_t totalSize = endIndex - startIndex + 1;
            int64_t halfSize = totalSize / 2;
            if (halfSize < 1)
            {
                halfSize = 1;
            }
            int64_t halfIndex = endIndex - halfSize;
            for (; startIndex <= endIndex; ++startIndex)
            {
                double res = (tickList[startIndex]->ask_avg_px - tickList[startIndex]->last_px) / this->marketDataManager->pre_close;
                if (zcz)
                {
                    res /= 2;
                }
                if (startIndex > halfIndex)
                {
                    halfMean += res;
                }
                mean += res;
            }
            mean = mean / totalSize;
            halfMean = halfMean / halfSize;
            if (halfMean != 0)
            {
                factorVal = std::log(halfMean / mean);
            }
        }
        this->UpdateValue(0, factorVal);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_SSS_TK_SPCTDIFF_ALL_HALF_H
