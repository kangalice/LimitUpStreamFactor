#pragma once

#include "include/factor/base_factor.h"

using namespace vendor::strategy::common_utils;

namespace vendor {
namespace strategy {
namespace eventdriven {
class Yzhan_hf_f1_27 : public BaseFactor
{
private:
    uint64_t count = 0;
    double sum = 0;
    std::vector<double> amt_filter_list; // todo init size
    StandardDeviation amt_std{true};

public:
    Yzhan_hf_f1_27(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"yzhan_hf_f1_27"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        if (fill->side == 2)
        {
            double value = std::log(fill->amt);
            amt_filter_list.emplace_back(value);
            sum += value;
            ++count;
            if (count == 50)
            {
                amt_std.increment(sum / 50);
            }
            else if (count > 50)
            {
                sum -= amt_filter_list[count - 51];
                amt_std.increment(sum / 50);
            }
        }
    }

    void Calculate() override
    {
        if (amt_filter_list.size() >= 50)
        {
            double factor_value = amt_std.getResult();
            this->UpdateValue(0, std::isnan(factor_value) ? 0 : factor_value);
        }
        else
        {
            this->UpdateValue(0, 0);
        }
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor