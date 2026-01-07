//
// Created by zhangtian on 2024/1/23.
//

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {

class Factor_xly_t_prup_qty_ratio : public BaseFactor
{
private:
    double pre_close;
    double sum = 0.0;
    double high_sum = 0.0;

public:
    Factor_xly_t_prup_qty_ratio(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"xly_t_prup_qty_ratio"};
        this->update_mode = {1};

        if (this->marketDataManager->zcz)
        {
            this->pre_close = common_utils::MathUtil::RoundHalfUp(this->marketDataManager->pre_close * 1.12, 4);
        }
        else
        {
            this->pre_close = common_utils::MathUtil::RoundHalfUp(this->marketDataManager->pre_close * 1.06, 4);
        }
    }

    void Update(const Fill *fill) override
    {
        this->sum += fill->qty;
        if (fill->price >= this->pre_close)
        {
            this->high_sum += fill->qty;
        }
    }

    void Calculate() override
    {
        double f = 0.0;
        if (this->sum != 0)
        {
            f = this->high_sum / this->sum;
        }
        this->UpdateValue(0, f);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
