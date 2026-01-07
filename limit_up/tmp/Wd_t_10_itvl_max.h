//
// Created by zhangtian on 2023/2/20.
//

#pragma once

#include <climits>
#include <optional>
#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Wd_t_10_itvl_max : public BaseFactor
{
public:
    Wd_t_10_itvl_max(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
        , timeSet()
    {
        this->factor_names = std::vector<std::string>{"wd_t_10_itvl_max"};
        this->update_mode = {1};
        this->timeSet.insert(93000000);
    }

    void Update(const Fill *fill) override
    {
        timeSet.emplace(fill->timestamp);
    }

    void Calculate() override
    {
        int64_t ztTime = marketDataManager->GetZTLocalTime();
        int64_t startTime = common_utils::TimeUtil::FunGetTime(ztTime, -10);

        int64_t timeDeltaMax = 0;
        optional<int64_t> nextMd;
        // set 本身是有序的，倒序
        for (auto itr = timeSet.rbegin(); itr != timeSet.rend(); ++itr)
        {
            if (nextMd)
            {
                timeDeltaMax = max(timeDeltaMax, common_utils::TimeUtil::CalTimeDelta(*itr, *nextMd));
            }
            if (*itr < startTime)
            {
                break;
            }
            nextMd = *itr;
        }

        double val = 1200;
        if (timeDeltaMax != 0 && timeDeltaMax <= 100000)
        {
            val = timeDeltaMax;
        }

        UpdateValue(0, val);
    }

private:
    std::set<int64_t> timeSet;
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor