//
// Created by zhangtian on 2023/2/16.
//

#pragma once

#include <climits>
#include <optional>
#include "include/factor/base_factor.h"

namespace vendor {
    namespace strategy {
        namespace eventdriven {
            class V_BLtNzR23 : public BaseFactor {
                //  * 突破前1分钟，买单成交时长非零占比
            public:
                V_BLtNzR23(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
                        : BaseFactor(manager, factor_map) {
                    this->factor_names = std::vector<std::string>{"V_BLtNzR23"};
                }

                void Calculate() override {
                    const std::unordered_map<uint64_t, MarketOrder> &tradeBuyMap = marketDataManager->last_1min_trade_buy_map;
                    if (tradeBuyMap.empty()) {
                        UpdateValue(0, 0);
                        return;
                    }
                    int64_t tradeTimeNotZeroSize = 0;
                    for (const auto &itr: tradeBuyMap) {
                        if (itr.second.last_fill_time > itr.second.first_fill_time) {
                            tradeTimeNotZeroSize++;
                        }
                    }
                    double val = double(tradeTimeNotZeroSize) / tradeBuyMap.size();
                    UpdateValue(0, val);
                }
            };
        } // namespace eventdriven
    } // namespace strategy
} // namespace vendor