//
// Created by appadmin on 3/12/24.
//

#ifndef EVENTDRIVEN_STRATEGY_CPP_BACKTEST_SSS_TO_PRICEDIFFSTD_ALL_S60_H
#define EVENTDRIVEN_STRATEGY_CPP_BACKTEST_SSS_TO_PRICEDIFFSTD_ALL_S60_H

#include "include/factor/base_factor.h"

namespace vendor::strategy::eventdriven {
    class Sss_to_pricediffstd_all_s60 : public BaseFactor
    {
    public:
        Sss_to_pricediffstd_all_s60(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
                : BaseFactor(manager, factor_map)
        {
            this->factor_names = std::vector<std::string>{"sss_to_pricediffstd_all_s60"};
        }

        void Calculate() override
        {
            double nan_val = 0;

            auto time_threshold = std::max(TimeUtil::FunGetTime((int64_t)marketDataManager->ZT_local_time, -60, false), 9'30'00'000L);
            auto curr_timestamp = std::numeric_limits<uint64_t>::max();
            double trade_price = 0;
            auto &fill_list = marketDataManager->fill_list;
            int j = (int)fill_list.size() - 1;
            int64_t order_qty_sum = 0;
            double pricediff_times_order_qty_sum = 0;
            std::vector<double> pricediff_list;
            for (int i = (int)marketDataManager->lxjj_order_list.size() - 1; i >= 0; --i) {
                auto *order = marketDataManager->lxjj_order_list[i];
                if (order->md_time < time_threshold) {
                    break;
                }
                if (order->md_time <= curr_timestamp) { // need to re-calculate the values, otherwise, can re-use the values computed previously
                    while (j >= 0 && (int64_t)fill_list[j]->timestamp >= order->md_time) {
                        --j;
                    }
                    if (j < 0) {
                        trade_price = marketDataManager->pre_close;
                    } else {
                        uint64_t trade_qty_sum = 0;
                        double trade_money_sum = 0;
                        curr_timestamp = fill_list[j]->timestamp;
                        while (j >= 0 && fill_list[j]->timestamp == curr_timestamp) {
                            trade_qty_sum += fill_list[j]->qty;
                            trade_money_sum += fill_list[j]->amt;
                            --j;
                        }
                        trade_price = trade_money_sum / (double) trade_qty_sum;
                    }
                }
                auto pricediff = (order->order_price - trade_price) / marketDataManager->pre_close * 100;
                if (marketDataManager->zcz) {
                    pricediff /= 2;
                }
                pricediff_list.push_back(pricediff);
                pricediff_times_order_qty_sum += pricediff * (double)order->order_qty;
                order_qty_sum += order->order_qty;
            }

            if (order_qty_sum == 0) {
                this->UpdateValue(0, nan_val);
                return;
            }

            auto vwap = pricediff_times_order_qty_sum / (double)order_qty_sum;
            double numerator = 0;
            for (int i = 0; i < pricediff_list.size(); ++i) {
                auto *order = marketDataManager->lxjj_order_list[marketDataManager->lxjj_order_list.size() - 1 - i];
                numerator += (pricediff_list[i] - vwap) * (pricediff_list[i] - vwap) * (double)order->order_qty;
            }
            auto value = std::sqrt(numerator / (double)order_qty_sum);

            this->UpdateValue(0, std::isinf(value) || std::isnan(value) ? nan_val : value);
        }
    };
} // namespace vendor::strategy::eventdriven

#endif //EVENTDRIVEN_STRATEGY_CPP_BACKTEST_SSS_TO_PRICEDIFFSTD_ALL_S60_H
