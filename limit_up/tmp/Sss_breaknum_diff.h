#ifndef EVENTDRIVENSTRATEGY_SSS_BREAKNUM_DIFF_H
#define EVENTDRIVENSTRATEGY_SSS_BREAKNUM_DIFF_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Sss_breaknum_diff : public BaseFactor
{
private:
    std::unordered_map<uint64_t, double> actBuyMap;
    std::unordered_map<uint64_t, double> actSellMap;
    std::unordered_set<uint64_t> actBuySetWithNuniquePrice;
    std::unordered_set<uint64_t> actSellSetWithNuniquePrice;

public:
    Sss_breaknum_diff(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"sss_breaknum_diff"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        uint64_t buyNo = fill->buy_no;
        uint64_t sellNo = fill->sell_no;
        double price = fill->price;

        if (buyNo > sellNo)
        {
            if (this->actBuySetWithNuniquePrice.count(buyNo) == 0)
            {
                double existPrice = -1;
                auto it = this->actBuyMap.find(buyNo);
                if (it != this->actBuyMap.end())
                {
                    existPrice = it->second;
                }
                if (existPrice == -1)
                {
                    this->actBuyMap.emplace(buyNo, price);
                }
                else
                {
                    if (price != existPrice)
                    {
                        this->actBuySetWithNuniquePrice.emplace(buyNo);
                    }
                }
            }
        }
        else
        {
            if (this->actSellSetWithNuniquePrice.count(sellNo) == 0)
            {
                double existPrice = -1;
                auto it = this->actSellMap.find(sellNo);
                if (it != this->actSellMap.end())
                {
                    existPrice = it->second;
                }
                if (existPrice == -1)
                {
                    this->actSellMap.emplace(sellNo, price);
                }
                else
                {
                    if (price != existPrice)
                    {
                        this->actSellSetWithNuniquePrice.emplace(sellNo);
                    }
                }
            }
        }
    }

    void Calculate() override
    {
        this->UpdateValue(
            0, static_cast<double>(this->actBuySetWithNuniquePrice.size()) - static_cast<double>(this->actSellSetWithNuniquePrice.size()));
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_SSS_BREAKNUM_DIFF_H
