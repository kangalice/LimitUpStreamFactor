
#ifndef EVENTDRIVENSTRATEGY_YZHAN_HF_C0_1_H
#define EVENTDRIVENSTRATEGY_YZHAN_HF_C0_1_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Yzhan_hf_c0_1 : public BaseFactor
{
    bool isSame = true;
    double lastRet = NAN;
    double sum = 0;
    std::vector<double> rtnList;

public:
    Yzhan_hf_c0_1(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"yzhan_hf_c0_1"};
        this->update_mode = {2};
        rtnList.reserve(10000);
    }

    void Update(const Fill *fill) override
    {
        auto &ret = fill->ret;
        if (!std::isnan(ret))
        {
            rtnList.push_back(ret);
            sum += ret;
            if (isSame)
            {
                if (!std::isnan(lastRet))
                {
                    isSame = ret == lastRet;
                }
                lastRet = ret;
            }
        }
    }

    void Calculate() override
    {
        int size = rtnList.size();
        if (size <= 2 || isSame)
        {
            this->UpdateValue(0, 0);
            return;
        }
        double mean = sum / size;
        double m3 = 0;
        double m2 = 0;
        for (auto i : rtnList)
        {
            auto diff = i - mean;
            auto square = diff * diff;
            m2 += square;
            m3 += square * diff;
        }
        m3 /= size;
        m2 /= size;
        double factorVal = m3 / pow(m2, 1.5) * sqrt(size) * sqrt(size - 1) / (size - 2);
        this->UpdateValue(0, factorVal);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_YZHAN_HF_C0_1_H
