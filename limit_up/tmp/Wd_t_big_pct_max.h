//
// Created by zhangtian on 2023/5/15.
//

#pragma once

#include "include/factor/base_factor.h"

using namespace vendor::strategy::common_utils;

namespace vendor {
namespace strategy {
namespace eventdriven {

class Wd_t_big_pct_max : public BaseFactor
{
private:
    uint64_t timePoint = 0;
    double maxPrice = 0;
    double minPrice = DBL_MAX;
    double preMinPrice = -1;

    std::vector<uint64_t> times;
    std::vector<double> ratios;

public:
    Wd_t_big_pct_max(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"wd_t_big_pct_max"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        uint64_t md = fill->timestamp;
        if (timePoint == 0)
        {
            maxPrice = fill->price;
            minPrice = fill->price;
            timePoint = md;
        }
        else if (md == timePoint)
        {
            maxPrice = std::max(maxPrice, fill->price);
            minPrice = std::min(minPrice, fill->price);
        }
        else
        {
            if (preMinPrice != -1)
            {
                double ratio = 0;
                if (marketDataManager->zcz)
                {
                    double preclose = marketDataManager->pre_close;
                    ratio = ((maxPrice / preclose - 1) / 2 + 1) / ((preMinPrice / preclose - 1) / 2 + 1) - 1;
                }
                else
                {
                    ratio = maxPrice / preMinPrice - 1;
                }
                times.push_back(timePoint);
                ratios.push_back(ratio);
            }

            timePoint = md;
            preMinPrice = minPrice;
            maxPrice = fill->price;
            minPrice = fill->price;
        }
    }

    void Calculate() override
    {
        if (preMinPrice == -1)
        {
            this->UpdateValue(0, 0.0065);
            return;
        }

        uint64_t ztTime = marketDataManager->GetZTLocalTime();
        uint64_t startTime = common_utils::TimeUtil::FunGetTime(ztTime, -5);

        double res = DBL_MIN;

        for (int i = times.size() - 1; i >= 0; i--)
        {
            uint64_t time = times[i];
            if (time < startTime)
            {
                break;
            }
            res = std::max(res, ratios[i]);
        }

        if (marketDataManager->zcz)
        {
            double preclose = marketDataManager->pre_close;
            res = std::max(res, ((maxPrice / preclose - 1) / 2 + 1) / ((preMinPrice / preclose - 1) / 2 + 1) - 1);
        }
        else
        {
            res = std::max(res, maxPrice / preMinPrice - 1);
        }
        this->UpdateValue(0, res);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
