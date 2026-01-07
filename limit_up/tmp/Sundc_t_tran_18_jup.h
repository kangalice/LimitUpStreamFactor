//
// Created by zhangtian on 2023/5/15.
//

#pragma once

#include "include/factor/base_factor.h"

using namespace vendor::strategy::common_utils;

namespace vendor {
namespace strategy {
namespace eventdriven {

class Sundc_t_tran_18_jup : public BaseFactor
{
private:
    std::set<int64_t> ztBuyNo;
    std::set<int64_t> ztSellNo;
    std::set<int64_t> unztBuyNo;
    std::set<int64_t> unztSellNo;

public:
    Sundc_t_tran_18_jup(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"sundc_t_tran_18"};
        this->update_mode = {2};
    }

    void Update(const Fill *fill) override
    {
        if (fill->price == marketDataManager->high_limit_close)
        {
            ztBuyNo.emplace(fill->buy_no);
            ztSellNo.emplace(fill->sell_no);
        }
        else
        {
            unztBuyNo.emplace(fill->buy_no);
            unztSellNo.emplace(fill->sell_no);
        }
    }

    void Calculate() override
    {
        double res = ztBuyNo.size() / (ztSellNo.size() + 1.0) + unztBuyNo.size() / (unztSellNo.size() + 1.0);
        this->UpdateValue(0, (std::isnan(res) != 0) || (std::isinf(res) != 0) ? 0 : res);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
