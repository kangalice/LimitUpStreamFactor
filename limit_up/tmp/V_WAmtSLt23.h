//
// Created by zhangtian on 2023/2/20.
//

#pragma once

#include <climits>
#include <optional>
#include "include/factor/base_factor.h"

namespace vendor {
    namespace strategy {
        namespace eventdriven {
            class V_WAmtSLt23 : public BaseFactor { // 突破前1分钟，卖单成交时长成交金额加权均值
            public:
                V_WAmtSLt23(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
                        : BaseFactor(manager, factor_map) {
                    this->factor_names = std::vector<std::string>{"V_WAmtSLt23"};
                }

                void Calculate() override {
                    // 1. 这个因子就是上一个因子V_APBSLt23步骤4中的分母，卖单成交额加权成交时长均值
                    const std::unordered_map<uint64_t, MarketOrder> &tradeSellMap = marketDataManager->last_1min_trade_sell_map;

                    if (tradeSellMap.empty()) {
                        UpdateValue(0, 0);
                        return;
                    }

                    double totalAmt = 0;
                    double totalTimeMultiAmt = 0;
                    for (const auto &itr: tradeSellMap) {
                        totalAmt += itr.second.amt;
                        totalTimeMultiAmt += itr.second.fill_time_delta * itr.second.amt;
                    }
                    double val = totalAmt == 0 ? 0 : totalTimeMultiAmt / totalAmt;
                    UpdateValue(0, val);
                }
            };
        } // namespace eventdriven
    } // namespace strategy
} // namespace vendor