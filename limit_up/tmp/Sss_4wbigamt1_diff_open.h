#ifndef EVENTDRIVENSTRATEGY_SSS_4WBIGAMT1_DIFF_OPEN_H
#define EVENTDRIVENSTRATEGY_SSS_4WBIGAMT1_DIFF_OPEN_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Sss_4wbigamt1_diff_open : public BaseFactor
{
private:
    double totalBuySum = 0;
    double totalSellSum = 0;
    std::unordered_map<uint64_t, double> buyNoAmtMap;
    std::unordered_map<uint64_t, double> sellNoAmtMap;
    std::unordered_set<uint64_t> filterBuyNoSet;
    std::unordered_set<uint64_t> filterSellNoSet;
    std::unordered_set<uint64_t> buyNoSet;
    std::unordered_set<uint64_t> sellNoSet;

public:
    Sss_4wbigamt1_diff_open(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"sss_4wbigamt1_diff_open"};
        this->update_mode = {2};
    }

    void Update(const Fill *fill) override
    {
        uint64_t buyNo = fill->buy_no;
        uint64_t sellNo = fill->sell_no;

        if (fill->timestamp > 93000000 && fill->timestamp < 100000000)
        {
            filterBuyNoSet.emplace(buyNo);
            filterSellNoSet.emplace(sellNo);
        }

        double amt = fill->amt;

        if (buyNoSet.count(buyNo) != 0)
        {
            totalBuySum += amt;
        }
        else
        {
            double buySum;
            auto it = buyNoAmtMap.find(buyNo);
            if (it != buyNoAmtMap.end())
            {
                buySum = it->second + amt;
                buyNoAmtMap.insert_or_assign(it->first, buySum);
            }
            else
            {
                buySum = amt;
                buyNoAmtMap.emplace(buyNo, buySum);
            }

            if (filterBuyNoSet.count(buyNo) != 0 && buySum > 40000)
            {
                totalBuySum += buySum;
                buyNoSet.emplace(buyNo);
            }
        }

        if (sellNoSet.count(sellNo) != 0)
        {
            totalSellSum += amt;
        }
        else
        {

            double sellSum;
            auto it = sellNoAmtMap.find(sellNo);
            if (it != sellNoAmtMap.end())
            {
                sellSum = it->second + amt;
                sellNoAmtMap.insert_or_assign(it->first, sellSum);
            }
            else
            {
                sellSum = amt;
                sellNoAmtMap.emplace(sellNo, sellSum);
            }

            if (filterSellNoSet.count(sellNo) != 0 && sellSum > 40000)
            {
                totalSellSum += sellSum;
                sellNoSet.emplace(sellNo);
            }
        }
    }

    void Calculate() override
    {

        this->UpdateValue(0, totalBuySum - totalSellSum);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_SSS_4WBIGAMT1_DIFF_OPEN_H
