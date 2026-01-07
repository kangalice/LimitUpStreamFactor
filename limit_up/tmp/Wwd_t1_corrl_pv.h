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
            class Wwd_t1_corrl_pv : public BaseFactor {
            public:
                Wwd_t1_corrl_pv(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
                        : BaseFactor(manager, factor_map) {
                    this->factor_names = std::vector<std::string>{"wwd_t1_corrl_pv"};
                }

                void Calculate() override {
                    const std::vector<Fill *> &fillList = marketDataManager->last_1min_fill_list;
                    if (fillList.empty()) {
                        UpdateValue(0, 0);
                        return;
                    }

                    std::unordered_map <uint64_t, DataPair> buyNoAmtQty;
                    buyNoAmtQty.reserve(1000);
                    for (const auto &itr: fillList) {
                        merge(buyNoAmtQty, itr->buy_no, itr->amt, itr->qty);
                    }

                    common_utils::SimpleRegression regression;
                    for (auto &itr: buyNoAmtQty) {
                        regression.addData(itr.second.left / itr.second.right, log(itr.second.right));
                    }

                    double factorValue = regression.getR();
                    UpdateValue(0, isnan(factorValue) ? 0 : factorValue);
                }

                static void
                merge(std::unordered_map <uint64_t, DataPair> &mp, uint64_t key, double value, double value2) {
                    auto itr = mp.find(key);
                    if (itr != mp.end()) {
                        (itr->second.left) += value;
                        (itr->second.right) += value2;
                        return;
                    }
                    mp.emplace(key, DataPair{value, value2});
                }
            };
        } // namespace eventdriven
    } // namespace strategy
} // namespace vendor