//
// Created by zhangtian on 2023/5/15.
//

#pragma once

#include "include/factor/base_factor.h"

using namespace vendor::strategy::common_utils;

namespace vendor {
namespace strategy {
namespace eventdriven {

class V_BSVolRatio2 : public BaseFactor
{
private:
    double totalBuyQty = 0;
    double totalSellQty = 0;

public:
    V_BSVolRatio2(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"V_BSVolRatio2"};
        this->update_mode = {3};
    }

    void Update(const Trade *trade) override
    {
        totalBuyQty += trade->trade_qty;
        totalSellQty += trade->trade_qty;
    }

    void Update(const Cancel *cancel) override
    {
        if (cancel->side == '1')
        {
            totalBuyQty += cancel->order_qty;
        }
        else
        {
            totalSellQty += cancel->order_qty;
        }
    }

    void Calculate() override
    {
        if (totalBuyQty == 0 || totalSellQty == 0)
        {
            this->UpdateValue(0, 0);
        }
        else
        {
            this->UpdateValue(0, totalSellQty / totalBuyQty);
        }
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
