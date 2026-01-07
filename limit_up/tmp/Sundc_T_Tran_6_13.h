//
// Created by zhangtian on 2023/2/15.
//

#pragma once

#include <optional>

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Sundc_T_Tran_6_13 : public BaseFactor
{
public:
    Sundc_T_Tran_6_13(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
        , timeSet()
    {
        this->factor_names = std::vector<std::string>{"sundc_t_tran_6", "sundc_t_tran_12", "sundc_t_tran_13"};
        this->update_mode = {3};
    }

    void Update(const Trade *trade) override
    {
        uint64_t millTime = trade->md_time;
        timeSet.insert(millTime);
        if (timeSet.size() > 5)
        {
            timeSet.erase(timeSet.begin());
        }
    }

    void Update(const Cancel *cancelOrder) override
    {
        uint64_t millTime = cancelOrder->md_time;
        timeSet.insert(millTime);
        if (timeSet.size() > 5)
        {
            timeSet.erase(timeSet.begin());
        }
    }

    static double merge(std::unordered_map<uint64_t, double> &mp, uint64_t key, double value)
    {
        auto itr = mp.find(key);
        if (itr != mp.end())
        {
            (itr->second) += value;
            return itr->second;
        }
        mp.emplace(key, value);
        return value;
    }

    // sundc_t_tran_13
    // 1.在（41）中计算各个TradeBuyNo出现的次数，得到列表
    // 2.在（41）中计算各个TradeSellNo出现的次数，得到列表
    // 3.1.的标准差除以2.标准差
    // 4.如果3.为nan或inf，设置因子值为0
    void Calculate() override
    {
        double buyAmt = 0;
        double sellAmt = 0;

        std::set<uint64_t> buySet;
        std::set<uint64_t> sellSet;

        std::unordered_map<uint64_t, double> tradeBuyMap(200);
        std::unordered_map<uint64_t, double> tradeSellMap(200);
        std::unordered_map<uint64_t, double> tradeBuyCountMap(200);
        std::unordered_map<uint64_t, double> tradeSellCountMap(200);

        if (!timeSet.empty())
        {
            uint64_t startDate = *timeSet.begin();
            const std::vector<Fill *> &fillList = marketDataManager->fill_list;
            for (int i = fillList.size() - 1; i >= 0; --i)
            {
                const auto &itr = fillList[i];
                if (itr->timestamp < startDate)
                {
                    break;
                }

                uint64_t buyNo = itr->buy_no;
                uint64_t sellNo = itr->sell_no;

                if (itr->side == 1) // bid
                {
                    buyAmt += itr->amt;
                    buySet.insert(buyNo);
                }
                else if (itr->side == 2) // offer
                {
                    sellAmt += itr->amt;
                    sellSet.insert(sellNo);
                }
                merge(tradeBuyMap, buyNo, itr->qty);
                merge(tradeSellMap, sellNo, itr->qty);
                merge(tradeBuyCountMap, buyNo, 1);
                merge(tradeSellCountMap, sellNo, 1);
            }
        }

        // sundc_t_tran_6
        double res2 = (buyAmt + sellAmt) / (marketDataManager->high_price * marketDataManager->free_float_capital);

        // sundc_t_tran_12
        std::vector<double> lVol;
        for (uint64_t itr : buySet)
        {
            lVol.emplace_back(tradeBuyMap.find(itr)->second);
        }
        for (uint64_t itr : sellSet)
        {
            lVol.emplace_back(tradeSellMap.find(itr)->second);
        }
        double lvolSum = common_utils::MathUtil::calcSum(lVol);

        double res3 = 0;
        if (lvolSum != 0)
        {
            std::sort(lVol.begin(), lVol.end());
            double mid = 2 * common_utils::MathUtil::calcSortedMedian(lVol);
            double lBigBuyVolSum = 0;
            for (uint64_t itr : buySet)
            {
                double tmp = tradeBuyMap.find(itr)->second;
                if (tmp > mid)
                {
                    lBigBuyVolSum += tmp;
                }
            }
            double lBigSellVolSum = 0;
            for (uint64_t itr : sellSet)
            {
                double tmp = tradeSellMap.find(itr)->second;
                if (tmp > mid)
                {
                    lBigSellVolSum += tmp;
                }
            }
            res3 = (lBigBuyVolSum - lBigSellVolSum) / lvolSum;
        }

        // sundc_t_tran_13
        double sellStd = common_utils::MathUtil::calcStd(tradeSellCountMap, false, false, true);
        double res4 = 0;
        if (isnan(sellStd) == 0 && sellStd != 0)
        {
            res4 = common_utils::MathUtil::calcStd(tradeBuyCountMap, false, false, true);
            res4 = res4 / sellStd;
        }

        UpdateValue(0, isnan(res2) ? 0 : res2);
        UpdateValue(1, isnan(res3) ? 0 : res3);
        UpdateValue(2, isnan(res4) ? 0 : res4);
    }

private:
    std::set<uint64_t> timeSet;
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor