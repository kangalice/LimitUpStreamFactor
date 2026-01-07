#pragma once

#include "include/factor/base_factor.h"

using namespace vendor::strategy::common_utils;

namespace vendor {
namespace strategy {
namespace eventdriven {
class Yzhan_hf_b2_38 : public BaseFactor
{
private:
    uint64_t count = 0;
    double sum = 0;
    std::vector<double> filter_bs_flags; // todo init size
    StandardDeviation diff_std{true};

public:
    Yzhan_hf_b2_38(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"yzhan_hf_b2_38"};
        this->update_mode = {2};
    }

    void Update(const Fill *fill) override
    {
        if (fill->ret > 0)
        {
            double bs = fill->side;
            filter_bs_flags.emplace_back(bs);
            sum += bs;
            ++count;
            if (count < 10) {}
            else if (count <= 32)
            {
                diff_std.increment(bs - sum / count);
            }
            else
            {
                sum -= filter_bs_flags[count - 33];
                diff_std.increment(bs - sum / 32);
            }
        }
    }

    void Calculate() override
    {
        double factor_value = filter_bs_flags.size() >= 10 ? diff_std.getResult() : 0;
        this->UpdateValue(0, std::isnan(factor_value) ? 0 : factor_value);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor