//
// Created by appadmin on 2023/4/25.
//

#ifndef BACKTESTFRAMEWORK_YZHAN_HF_AK2_40_H
#define BACKTESTFRAMEWORK_YZHAN_HF_AK2_40_H

#include "include/factor/base_factor.h"

namespace vendor {
    namespace strategy {
        namespace eventdriven {
            class Yzhan_hf_ak2_40 : public BaseFactor {
            public:
                Yzhan_hf_ak2_40(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
                        : BaseFactor(manager, factor_map) {
                    this->factor_names = std::vector < std::string > {"yzhan_hf_ak2_40"};
                }

                void Calculate() override {
                    auto &lxjj_fill_list = this->marketDataManager->lxjj_fill_list;
                    if (lxjj_fill_list.empty()) {
                        this->UpdateValue(0, 0);
                        return;
                    }
                    double trade_qt1_sum = this->marketDataManager->last_fill->cum_trade_qty1;
                    double trade_qty_sum = this->marketDataManager->lxjj_total_qty;
                    double tradeMoneySum = this->marketDataManager->lxjj_total_amt;

                    common_utils::SimpleRegression regression98;
                    common_utils::SimpleRegression regression96;

                    for (int64_t i = lxjj_fill_list.size() - 1; i >= 0; --i) {
                        auto &fill = lxjj_fill_list[i];
                        if (fill->cum_trade_qty1 / trade_qt1_sum > 0.98) {
                            regression98.addData(fill->qty / trade_qty_sum, fill->amt / tradeMoneySum);
                            regression96.addData(fill->qty / trade_qty_sum, fill->amt / tradeMoneySum);
                        } else if (fill->cum_trade_qty1 / trade_qt1_sum > 0.96) {
                            regression96.addData(fill->qty / trade_qty_sum, fill->amt / tradeMoneySum);
                        } else {
                            break;
                        }
                    }

                    double correlation98 = regression98.getR();
                    double laValue = isnan(correlation98) ? 0 : correlation98;

                    double correlation96 = regression96.getR();
                    double lbValue = isnan(correlation96) ? 0 : correlation96;

                    double factorValue = laValue - lbValue;
                    UpdateValue(0, isnan(factorValue) ? 0 : factorValue);
                }
            };
        } // namespace eventdriven
    } // namespace strategy
} // namespace vendor
#endif // BACKTESTFRAMEWORK_YZHAN_HF_AK2_40_H
