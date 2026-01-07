//
// Created by zhangtian on 2023/5/15.
//

#pragma once

#include "include/factor/base_factor.h"

using namespace vendor::strategy::common_utils;

namespace vendor {
namespace strategy {
namespace eventdriven {

class Sundc_t_tran_16_fix : public BaseFactor
{
private:
    double activeBuyVol = 0;
    double activeSellVol = 0;

    std::set<uint64_t> activeBuySet;
    std::set<uint64_t> activeSellSet;

public:
    Sundc_t_tran_16_fix(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"sundc_t_tran_16_fix"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        if (fill->side == 1)
        {
            activeBuyVol += fill->qty;
            activeBuySet.emplace(fill->buy_no);
        }
        else
        {
            activeSellVol += fill->qty;
            activeSellSet.emplace(fill->sell_no);
        }
    }

    void Calculate() override
    {
        double factorValue = 0;
        double lxjjTotalQty = marketDataManager->lxjj_total_qty;
        if (lxjjTotalQty > 0)
        {
            factorValue = (activeBuyVol / (activeBuySet.size() + 1) - activeSellVol / (activeSellSet.size() + 1)) / lxjjTotalQty;
        }

        if ((std::isinf(factorValue) != 0) || (std::isnan(factorValue) != 0))
        {
            factorValue = 0;
        }

        this->UpdateValue(0, factorValue);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
