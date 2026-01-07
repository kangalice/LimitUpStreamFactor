//
// Created by zhangtian on 2023/2/14.
//

#pragma once

#include "include/factor/base_factor.h"
#include <optional>

namespace vendor {
namespace strategy {
namespace eventdriven {
class Sundc_t_tran_14 : public BaseFactor
{

public:
    Sundc_t_tran_14(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"sundc_t_tran_14"};
        this->update_mode = {3};
    }

    void Update(const Trade *trade) override
    {
        int64_t thisDate = trade->md_time;
        if (this->last5TimeList.empty())
        {
            this->last5TimeList.emplace_back(thisDate);
        }
        else
        {
            int64_t last = this->last5TimeList.back();
            if (thisDate > last)
            {
                this->last5TimeList.emplace_back(thisDate);
                if (this->last5TimeList.size() > 5)
                {
                    this->last5TimeList.erase(this->last5TimeList.begin());
                }
            }
        }
    }

    void Update(const Cancel *cancelOrder) override
    {
        int64_t thisDate = cancelOrder->md_time;
        if (this->last5TimeList.empty())
        {
            this->last5TimeList.emplace_back(thisDate);
        }
        else
        {
            int64_t last = this->last5TimeList.back();
            if (thisDate > last)
            {
                this->last5TimeList.emplace_back(thisDate);
                if (this->last5TimeList.size() > 5)
                {
                    this->last5TimeList.erase(this->last5TimeList.begin());
                }
            }
        }
    }

    void Calculate() override
    {
        std::unordered_map<int64_t, double> buyOrderCount(500);
        std::unordered_map<int64_t, double> sellOrderCount(500);

        if (this->last5TimeList.empty())
        {
            this->UpdateValue(0, 0);
            return;
        }

        int64_t startTime = this->last5TimeList.front();
        const std::vector<Fill *> &fillList = this->marketDataManager->fill_list;
        for (int i = fillList.size() - 1; i >= 0; --i)
        {
            const auto &itr = fillList[i];
            if (itr->timestamp < startTime)
            {
                break;
            }
            int64_t buy_no = itr->buy_no;
            int64_t sell_no = itr->sell_no;

            auto buyCnt = buyOrderCount.find(buy_no);
            if (buyCnt != buyOrderCount.end())
            {
                ++(buyCnt->second);
            }
            else
            {
                buyOrderCount.emplace(buy_no, 1);
            }

            auto sellCnt = sellOrderCount.find(sell_no);
            if (sellCnt != sellOrderCount.end())
            {
                ++(sellCnt->second);
            }
            else
            {
                sellOrderCount.emplace(sell_no, 1);
            }
        }

        double res = common_utils::MathUtil::calcSkew(buyOrderCount, false) + common_utils::MathUtil::calcSkew(sellOrderCount, false);
        this->UpdateValue(0, (isinf(res) || isnan(res)) ? 0 : res);
    }

private:
    std::vector<int64_t> last5TimeList;
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
