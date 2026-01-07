#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Yzhan_hf_d6_46_fix : public BaseFactor
{
private:
    double vol0_sum = 0;
    double vol1_sum = 0;
    uint64_t cnt1 = 0;
    uint64_t length = 0;

public:
    Yzhan_hf_d6_46_fix(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"yzhan_hf_d6_46_fix"};
        this->update_mode = {2};
    }

    void Update(const Fill *fill) override
    {
        double vol = std::log(fill->qty);
        vol0_sum += vol;
        if (length >= 128 && (marketDataManager->fill_list[length - 128]->price / fill->price - 1) > 0)
        {
            vol1_sum += vol;
            ++cnt1;
        }
        ++length;
    }

    void Calculate() override
    {
        double factor_value = cnt1 > 0 ? vol1_sum / cnt1 - vol0_sum / length : NAN;
        this->UpdateValue(0, std::isnan(factor_value) ? 0 : factor_value);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor