//
// Created by appadmin on 2023/2/8.
//

#ifndef EVENTDRIVENSTRATEGY_STRAIGHT_DIFF_MEAN_H
#define EVENTDRIVENSTRATEGY_STRAIGHT_DIFF_MEAN_H

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Straight_diff_mean : public BaseFactor
{
private:
    std::unordered_map<uint64_t, double> lxjj_time_to_price;
    double price_sum = 0;

public:
    Straight_diff_mean(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"Straight_diff_mean"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        auto it = lxjj_time_to_price.find(fill->timestamp);
        if (it != lxjj_time_to_price.end())
        {
            price_sum -= it->second;
        }
        this->lxjj_time_to_price[fill->timestamp] = fill->price;
        price_sum += fill->price;
    }

    void Calculate() override
    {
        unsigned size = this->marketDataManager->lxjj_time_to_price.size();
        double jhjj_price = this->marketDataManager->jhjj_price;
        double zt_price = this->marketDataManager->high_price;

        double ratio = (zt_price - jhjj_price) / size;
        unsigned cnt = size + 1;
        double sum = jhjj_price * size - price_sum + cnt * ratio * size / 2;
        double val = sum / cnt;
        this->UpdateValue(0, isnan(val) ? 0.2 : val);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_STRAIGHT_DIFF_MEAN_H
