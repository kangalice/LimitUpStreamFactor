//
// Created by zhangtian on 2024/1/23.
//

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {

class Zwh_20230824_004 : public BaseFactor
{
private:
    double thres_price1 = 0.0;
    double thres_price2 = 0.0;

    double total_sum = 0.0;
    double tp1_sum = 0.0;
    double tp2_sum = 0.0;
    std::vector<double> tp1_list;

public:
    Zwh_20230824_004(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
        , tp1_list()
    {
        this->factor_names = std::vector<std::string>{"zwh_20230824_004"};
        this->update_mode = {1};

        if (this->marketDataManager->zcz)
        {
            this->thres_price1 = std::floor(this->marketDataManager->pre_close * 100 * 1.16 + 0.5) / 100;
            this->thres_price2 = std::floor(this->marketDataManager->pre_close * 100 * 0.88 + 0.5) / 100;
        }
        else
        {
            this->thres_price1 = std::floor(this->marketDataManager->pre_close * 100 * 1.08 + 0.5) / 100;
            this->thres_price2 = std::floor(this->marketDataManager->pre_close * 100 * 0.94 + 0.5) / 100;
        }
    }

    void Update(const Fill *fill) override
    {
        if (fill->price >= this->thres_price1)
        {
            this->tp1_sum += fill->amt;
            this->tp1_list.push_back(fill->amt);
        }

        if (fill->price <= this->thres_price2)
        {
            this->tp2_sum += fill->amt;
        }

        this->total_sum += fill->amt;
    }

    void Calculate() override
    {
        double f = 0.0;
        if (!tp1_list.empty())
        {
            f = (tp1_sum + 100 * common_utils::MathUtil::calcStd(tp1_list, true)) / (0.1 + total_sum);
            f = common_utils::MathUtil::RoundHalfUp(f, 5);
        }
        this->UpdateValue(0, f);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
