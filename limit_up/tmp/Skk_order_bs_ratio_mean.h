//
// Created by appadmin on 2/26/24.
//

#ifndef LEDASTRATEGY_SKK_ORDER_BS_RATIO_MEAN_H
#define LEDASTRATEGY_SKK_ORDER_BS_RATIO_MEAN_H

#include "include/factor/base_factor.h"

namespace vendor {
    namespace strategy {
        namespace eventdriven {
            class Skk_order_bs_ratio_mean : public BaseFactor
            {
            public:
                Skk_order_bs_ratio_mean(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
                        : BaseFactor(manager, factor_map)
                {
                    this->factor_names = std::vector<std::string>{"skk_order_bs_ratio_mean"};
                }

                void Calculate() override
                {
                    double nan_val = 2.2;
                    auto &order_list = marketDataManager->lxjj_order_list;
                    if (order_list.empty()) {
                        this->UpdateValue(0, nan_val);
                        return;
                    }

                    int64_t end_time = 0;
                    for (int i = (int)order_list.size() - 1; i >= 0; --i) {
                        if (order_list[i]->order_type == '2') {
                            end_time = order_list[i]->md_time;
                            break;
                        }
                    }
                    auto start_time = std::max(TimeUtil::FunGetTime(end_time, -30, false), (LocalTime)9'30'00'000);

                    int cnt[2] = {0, 0}; // 0 -> buy, 1 -> sell
                    int64_t qty_sum[2] = {0, 0}; // 0 -> buy, 1 -> sell

                    for (int i = (int)order_list.size() - 1; i >= 0; --i) {
                        auto *order = order_list[i];
                        if (order->md_time < start_time) {
                            break;
                        }
                        if (order->order_type != '2') {
                            continue;
                        }
                        int side = order->side - '1'; // 0 -> buy, 1 -> sell
                        qty_sum[side] += order->order_qty;
                        ++cnt[side];
                    }
                    double amt1 = (double)qty_sum[0] / (double)cnt[0];
                    double amt2 = (double)qty_sum[1] / (double)cnt[1];
                    double value = std::min(amt1 / amt2, 100.0);

                    this->UpdateValue(0, std::isinf(value) || std::isnan(value) ? nan_val : value);
                }
            };
        } // namespace eventdriven
    } // namespace strategy
} // namespace vendor


#endif //LEDASTRATEGY_SKK_ORDER_BS_RATIO_MEAN_H
