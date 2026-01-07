#ifndef EVENTDRIVENSTRATEGY_BOOSTING_AVERAGE_ZB_RATIO_BEFORE_LAST_20
#define EVENTDRIVENSTRATEGY_BOOSTING_AVERAGE_ZB_RATIO_BEFORE_LAST_20

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Boosting_average_zb_ratio_before_last_20 : public BaseFactor
{
private:
    std::unordered_set<uint64_t> filteredNo;
    double totalUpdateQty = 0;

public:
    Boosting_average_zb_ratio_before_last_20(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"Boosting_average_zb_ratio_before_last_20"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        uint64_t buyNo = fill->buy_no;
        if (filteredNo.count(buyNo) != 0)
        {
            totalUpdateQty += fill->qty;
        }
        else
        {
            MarketOrder order = marketDataManager->lxjj_trade_buy_map.find(buyNo)->second;
            if (order.max_price - order.min_price >= 0.01)
            {
                totalUpdateQty += order.qty;
                filteredNo.emplace(buyNo);
            }
        }
    }

    void Calculate() override
    {
        if (marketDataManager->lxjj_trade_buy_map.size() < 50)
        {
            this->UpdateValue(0, 0);
            return;
        }
        double totalQty = totalUpdateQty;
        int count = filteredNo.size();

        int i = 0;

        for (auto iter = this->marketDataManager->lxjj_buy_no_set.rbegin(); iter != this->marketDataManager->lxjj_buy_no_set.rend(); ++iter)
        {
            uint64_t no = *iter;
            if (i >= 20)
            {
                break;
            }
            if (filteredNo.count(no) != 0)
            {
                totalQty -= marketDataManager->lxjj_trade_buy_map.find(no)->second.qty;
                --count;
            }
            ++i;
        }
        if (count == 0)
        {
            this->UpdateValue(0, 0);
        }
        else
        {
            this->UpdateValue(0, (totalQty / marketDataManager->free_float_capital) / count);
        }
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_BOOSTING_AVERAGE_ZB_RATIO_BEFORE_LAST_20
