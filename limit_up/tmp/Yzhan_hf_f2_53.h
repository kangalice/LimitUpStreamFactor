#pragma once

#include "include/factor/base_factor.h"

using namespace vendor::strategy::common_utils;

namespace vendor {
namespace strategy {
namespace eventdriven {
class Yzhan_hf_f2_53 : public BaseFactor
{
private:
    uint64_t vol_count = 0;
    uint64_t vol_filter_count = 0;
    double vol_filter_sum = 0;
    double vol_sum = 0;

    std::vector<double> vol_list;
    std::vector<double> vol_filter_list;

    StandardDeviation vol_std{true};
    StandardDeviation vol_filter_std{true};

public:
    Yzhan_hf_f2_53(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"yzhan_hf_f2_53"};
        this->update_mode = {2};

        this->vol_list.reserve(10000);
        this->vol_filter_list.reserve(10000);
    }

    void Update(const Fill *fill) override
    {
        double vol = std::log(fill->qty);
        vol_list.emplace_back(vol);
        vol_sum += vol;
        ++vol_count;
        if (vol_count == 50)
        {
            vol_std.increment(vol_sum / 50);
        }
        else if (vol_count > 50)
        {
            vol_sum -= vol_list[vol_count - 51];
            vol_std.increment(vol_sum / 50);
        }

        if (fill->ret > 0)
        {
            vol_filter_list.emplace_back(vol);
            vol_filter_sum += vol;
            ++vol_filter_count;
            if (vol_filter_count == 50)
            {
                vol_filter_std.increment(vol_filter_sum / 50);
            }
            else if (vol_filter_count > 50)
            {
                vol_filter_sum -= vol_filter_list[vol_filter_count - 51];
                vol_filter_std.increment(vol_filter_sum / 50);
            }
        }
    }

    void Calculate() override
    {
        if (vol_filter_std.getN() > 1)
        {
            this->UpdateValue(0, vol_filter_std.getResult() - vol_std.getResult());
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