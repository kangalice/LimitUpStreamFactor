//
// Created by zhangtian on 2024/1/23.
//

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {

class Factor_xly_t_prupt_uni_ratio : public BaseFactor
{
private:
    double pre_close;
    bool first_reach = false;
    std::set<double> trade_prices;
    std::set<double> high_trade_prices;
    std::set<double> tmp_trade_prices;
    int64_t pre_md_time = -1;

public:
    Factor_xly_t_prupt_uni_ratio(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
        , trade_prices()
        , high_trade_prices()
        , tmp_trade_prices()
    {
        this->factor_names = std::vector<std::string>{"xly_t_prupt_uni_ratio"};
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
        double price = common_utils::MathUtil::RoundHalfUp(fill->price, 4);
        this->trade_prices.emplace(price);

        // 未到触发条件时，要把同一个mdtime的price都记录下来
        if (!this->first_reach)
        {
            if (fill->timestamp > this->pre_md_time)
            {
                tmp_trade_prices.clear();
                tmp_trade_prices.emplace(price);
            }
            else
            {
                tmp_trade_prices.emplace(price);
            }

            if (price >= this->pre_close)
            {
                this->first_reach = true;
                this->high_trade_prices.insert(this->tmp_trade_prices.begin(), this->tmp_trade_prices.end());
            }

            this->pre_md_time = fill->timestamp;
        }
        else
        {
            this->high_trade_prices.emplace(price);
        }
    }

    void Calculate() override
    {
        double f = 0.0;
        if (!this->trade_prices.empty())
        {
            f = this->high_trade_prices.size() * 1.0 / this->trade_prices.size();
        }
        this->UpdateValue(0, f);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
